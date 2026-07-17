// =============================================================================
//  Real-time safety tests
//
//  1. PerformanceApvtsReader::ParamCache reads match the parameter values
//     stored in the APVTS (correctness of the cached-pointer path).
//  2. ToneShaper's in-place coefficient update matches JUCE's allocating
//     Coefficients::make* reference implementation sample-for-sample.
//  3. An allocation guard (global operator new override, armed per-scope)
//     proves the per-block paths perform zero heap allocations.
// =============================================================================

#include <juce_core/juce_core.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

#include "State/ParameterLayout.h"
#include "State/StateSchema.h"
#include "DSP/Performance/PerformanceApvtsReader.h"
#include "DSP/Performance/MacroMapper.h"
#include "DSP/Performance/PhraseChopper.h"
#include "DSP/Performance/MotionEngine.h"
#include "DSP/ToneShaper.h"
#include "DSP/SamplerEngine.h"
#include "State/SampleLibrary.h"

#include <atomic>
#include <cstdlib>
#include <new>

// ---------------------------------------------------------------------------
// Allocation guard: counts heap allocations while armed on the calling thread.
// Defined once in the test binary; replaces the global (non-aligned) forms.
// ---------------------------------------------------------------------------
namespace AllocationGuard
{
static std::atomic<bool> armed { false };
static std::atomic<int>  count { 0 };

struct Scope
{
    Scope()  { count.store (0); armed.store (true); }
    ~Scope() { armed.store (false); }
};
} // namespace AllocationGuard

void* operator new (std::size_t size)
{
    if (AllocationGuard::armed.load (std::memory_order_relaxed))
        AllocationGuard::count.fetch_add (1, std::memory_order_relaxed);
    if (void* p = std::malloc (size > 0 ? size : 1))
        return p;
    throw std::bad_alloc();
}

void* operator new[] (std::size_t size) { return operator new (size); }
void operator delete (void* p) noexcept { std::free (p); }
void operator delete[] (void* p) noexcept { std::free (p); }
void operator delete (void* p, std::size_t) noexcept { std::free (p); }
void operator delete[] (void* p, std::size_t) noexcept { std::free (p); }

namespace
{
struct RtTestProcessor : juce::AudioProcessor
{
    RtTestProcessor()
        : AudioProcessor (BusesProperties()),
          apvts (*this, nullptr, "AviatorKeyzState", AviatorKeyz::createParameterLayout())
    {}

    const juce::String getName() const override { return "RtTest"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}
    void prepareToPlay (double, int) override {}
    void releaseResources() override {}
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override {}
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    bool hasEditor() const override { return false; }
    void getStateInformation (juce::MemoryBlock&) override {}
    void setStateInformation (const void*, int) override {}

    juce::AudioProcessorValueTreeState apvts;
};

void setParam (juce::AudioProcessorValueTreeState& apvts, const juce::String& id, float value)
{
    auto* p = apvts.getParameter (id);
    if (p != nullptr)
        p->setValueNotifyingHost (p->convertTo0to1 (value));
}
} // namespace

class RealtimeSafetyTests : public juce::UnitTest
{
public:
    RealtimeSafetyTests() : juce::UnitTest ("RealtimeSafety", "AviatorKeyz") {}

    void runTest() override
    {
        namespace P = AviatorKeyz::ParamID;

        beginTest ("ParamCache reads match APVTS values");
        {
            RtTestProcessor proc;
            setParam (proc.apvts, P::SRC_START, 0.2f);
            setParam (proc.apvts, P::SRC_END, 0.9f);
            setParam (proc.apvts, P::SRC_TUNE, -7.f);
            setParam (proc.apvts, P::SRC_SPEED, 1.5f);
            setParam (proc.apvts, P::CHOP_ON, 1.f);
            setParam (proc.apvts, P::CHOP_AMOUNT, 0.6f);
            setParam (proc.apvts, P::PTEX_MIX, 0.4f);
            setParam (proc.apvts, P::PERF_MACRO_2, 0.8f);
            setParam (proc.apvts, P::chopStepParamId (3, "vol"), 0.25f);
            setParam (proc.apvts, P::chopStepParamId (3, "rev"), 1.f);
            setParam (proc.apvts, P::chopStepParamId (15, "pitch"), -5.f);

            PerformanceApvtsReader::ParamCache cache;
            cache.init (proc.apvts);

            const auto s = PerformanceApvtsReader::readBaseState (cache);
            expectWithinAbsoluteError (s.source.start, 0.2f, 1.0e-4f);
            expectWithinAbsoluteError (s.source.end, 0.9f, 1.0e-4f);
            expectWithinAbsoluteError (s.source.tune, -7.f, 1.0e-3f);
            expectWithinAbsoluteError (s.source.speed, 1.5f, 1.0e-3f);
            expect (s.chop.enabled);
            expectWithinAbsoluteError (s.chop.amount, 0.6f, 1.0e-4f);
            expectWithinAbsoluteError (s.texture.mix, 0.4f, 1.0e-4f);
            expectWithinAbsoluteError (s.chop.steps[3].volume, 0.25f, 1.0e-4f);
            expect (s.chop.steps[3].reverse);
            expectEquals (s.chop.steps[15].pitchOffset, -5);
            expectWithinAbsoluteError (PerformanceApvtsReader::readMacroValue (cache, 1), 0.8f, 1.0e-4f);
        }

        beginTest ("ToneShaper in-place coefficients match JUCE reference filters");
        {
            const juce::dsp::ProcessSpec spec { 48000.0, 512, 2 };

            ToneShaper shaper;
            shaper.prepare (spec);

            // Reference: the original allocating construction.
            const float tone = 0.7f;
            const float lowG  = juce::Decibels::decibelsToGain (-tone * 5.f);
            const float highG = juce::Decibels::decibelsToGain (tone * 5.f);
            juce::dsp::IIR::Filter<float> refLowL, refLowR, refHighL, refHighR;
            refLowL.coefficients  = juce::dsp::IIR::Coefficients<float>::makeLowShelf (spec.sampleRate, 180.0f, 0.707f, lowG);
            refLowR.coefficients  = refLowL.coefficients;
            refHighL.coefficients = juce::dsp::IIR::Coefficients<float>::makeHighShelf (spec.sampleRate, 6500.0f, 0.707f, highG);
            refHighR.coefficients = refHighL.coefficients;

            juce::Random rng (42);
            juce::AudioBuffer<float> a (2, 512), b (2, 512);
            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < 512; ++i)
                {
                    const float v = rng.nextFloat() * 2.f - 1.f;
                    a.setSample (ch, i, v);
                    b.setSample (ch, i, v);
                }

            shaper.process (a, tone);

            for (int i = 0; i < 512; ++i)
            {
                float l = refLowL.processSample (b.getSample (0, i));
                l = refHighL.processSample (l);
                float r = refLowR.processSample (b.getSample (1, i));
                r = refHighR.processSample (r);
                b.setSample (0, i, l);
                b.setSample (1, i, r);
            }

            float maxDiff = 0.f;
            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < 512; ++i)
                    maxDiff = juce::jmax (maxDiff, std::abs (a.getSample (ch, i) - b.getSample (ch, i)));
            expect (maxDiff < 1.0e-6f, "ToneShaper output diverges from reference: " + juce::String (maxDiff, 9));
        }

        beginTest ("Per-block paths perform zero heap allocations");
        {
            RtTestProcessor proc;
            setParam (proc.apvts, P::CHOP_ON, 1.f);
            setParam (proc.apvts, P::CHOP_AMOUNT, 0.5f);

            PerformanceApvtsReader::ParamCache cache;
            cache.init (proc.apvts);

            auto macros = MacroMapper::defaultsForCategory (AviatorKeyz::Category::LEADS);

            const juce::dsp::ProcessSpec spec { 48000.0, 256, 2 };
            ToneShaper shaper;
            shaper.prepare (spec);
            PhraseChopper chopper;
            chopper.prepare (spec);
            MotionEngine motion;
            motion.prepare (spec);

            // Static sample data → SamplerEngine voice rendering.
            static float sampleData[4096];
            for (int i = 0; i < 4096; ++i)
                sampleData[i] = std::sin (0.05f * static_cast<float> (i));
            SampleLibrary::AudioSnapshot snapshot;
            SampleLibrary::AudioRegion region;
            region.data = sampleData;
            region.numFrames = 4096;
            region.rootNote = 60;
            region.noteMin = 0;
            region.noteMax = 127;
            region.fileSampleRate = 48000.0;
            snapshot.regions.push_back (region);

            SamplerEngine sampler;
            sampler.prepare (spec);
            sampler.setSampleSnapshot (&snapshot);
            sampler.setEnvelopeTimesMs (5.f, 0.f, 1.f, 50.f);
            sampler.noteOn (60, 0.8f, false, 0.f);
            sampler.noteOn (64, 0.8f, false, 0.f);

            juce::AudioBuffer<float> buffer (2, 256);

            int allocations = 0;
            {
                AllocationGuard::Scope guard;
                for (int block = 0; block < 200; ++block)
                {
                    buffer.clear();

                    EngineState base = PerformanceApvtsReader::readBaseState (cache);
                    EngineState engineState = MacroMapper::applyMacros (base, macros, cache);

                    sampler.setSourceSettings (engineState.source, 120.0);
                    sampler.process (buffer);

                    chopper.updatePlayback (engineState, region.numFrames, 120.0);
                    chopper.process (buffer, engineState, 120.0);
                    motion.process (buffer, engineState, 120.0);

                    // Sweep tone so coefficients update every block.
                    shaper.process (buffer, 0.1f + 0.004f * static_cast<float> (block));
                }
                allocations = AllocationGuard::count.load();
            }

            expectEquals (allocations, 0,
                          "audio-thread path allocated " + juce::String (allocations) + " time(s)");
        }
    }
};

static RealtimeSafetyTests realtimeSafetyTests;
