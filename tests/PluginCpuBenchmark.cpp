// =============================================================================
//  PluginCpuBenchmark — whole-plugin processBlock cost, not a single engine.
//
//  PerformanceBenchmarkTests times SamplerEngine in isolation, which is why it
//  reports ~0.6% of the callback budget. That number does not describe what a
//  host sees: the real callback also pays for the APVTS read, the macro layer,
//  the chopper, the MFX rack, the reverb and the FX chain. This harness drives
//  the real AviatorKeyzProcessor through processBlock so the percentages are
//  comparable to what a DAW's CPU meter shows.
//
//  Informational only — it logs, it never fails a build.
// =============================================================================

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

#include "PluginProcessor.h"
#include "State/PresetManager.h"
#include "State/StateSchema.h"
#include "DSP/Mfx/MfxDescriptors.h"
#include "DSP/Mfx/MfxRack.h"

#include <algorithm>
#include <chrono>
#include <vector>
#include <mach/mach.h>

namespace
{
constexpr double kSampleRate = 48000.0;

void setParam (AviatorKeyzProcessor& proc, const juce::String& id, float realValue)
{
    if (auto* p = proc.getAPVTS().getParameter (id))
        p->setValueNotifyingHost (p->convertTo0to1 (realValue));
}

bool loadFirstPresetWithSample (AviatorKeyzProcessor& proc)
{
    auto& pm = proc.getPresetManager();
    for (const auto& cat : { AviatorKeyz::Category::LEADS, AviatorKeyz::Category::PADS })
        for (const auto& name : pm.getPresetsForCategory (cat))
            if (pm.loadPreset (cat, name) && pm.getCurrentSampleId().isNotEmpty())
                return true;
    return false;
}

struct Result
{
    double medianUs = 0.0;
    double p95Us    = 0.0;
    double worstUs  = 0.0;
};

/** Times processBlock over `blocks` callbacks with `numNotes` voices held. */
Result timeBlocks (AviatorKeyzProcessor& proc, int blockSize, int blocks, int numNotes)
{
    juce::AudioBuffer<float> buffer (2, blockSize);
    std::vector<double> samples;
    samples.reserve ((size_t) blocks);

    // Warm up: let the voices start, the smoothers settle and the caches fill.
    for (int b = 0; b < 64; ++b)
    {
        juce::MidiBuffer midi;
        if (b == 0)
            for (int v = 0; v < numNotes; ++v)
                midi.addEvent (juce::MidiMessage::noteOn (1, 48 + (v % 24), (juce::uint8) 100), 0);
        buffer.clear();
        proc.processBlock (buffer, midi);
    }

    for (int b = 0; b < blocks; ++b)
    {
        juce::MidiBuffer midi;
        buffer.clear();
        const auto t0 = std::chrono::steady_clock::now();
        proc.processBlock (buffer, midi);
        const auto t1 = std::chrono::steady_clock::now();
        samples.push_back (std::chrono::duration<double, std::micro> (t1 - t0).count());
    }

    std::sort (samples.begin(), samples.end());
    Result r;
    r.medianUs = samples[samples.size() / 2];
    r.p95Us    = samples[(size_t) ((double) samples.size() * 0.95)];
    r.worstUs  = samples.back();
    return r;
}
} // namespace

class PluginCpuBenchmark final : public juce::UnitTest
{
public:
    PluginCpuBenchmark() : juce::UnitTest ("PluginCpuBenchmark", "Benchmark") {}

    void runTest() override
    {
        for (const int blockSize : { 64, 128, 256, 512 })
            runScenarioSet (blockSize);

        beginTest ("Plugin surface");
        {
            AviatorKeyzProcessor proc;
            logMessage ("host-visible parameters: " + juce::String (proc.getParameters().size()));
            logMessage ("latency samples: " + juce::String (proc.getLatencySamples()));
            logMessage ("tail length seconds: " + juce::String (proc.getTailLengthSeconds(), 1));
        }

        beginTest ("Memory footprint of one instance");
        logMessage ("sizeof(AviatorKeyzProcessor) = "
                    + juce::String ((int) sizeof (AviatorKeyzProcessor)) + " bytes");

        auto residentMb = []
        {
            mach_task_basic_info info {};
            mach_msg_type_number_t count = MACH_TASK_BASIC_INFO_COUNT;
            task_info (mach_task_self(), MACH_TASK_BASIC_INFO, (task_info_t) &info, &count);
            return (double) info.resident_size / (1024.0 * 1024.0);
        };

        const double before = residentMb();
        {
            std::vector<std::unique_ptr<AviatorKeyzProcessor>> instances;
            for (int i = 0; i < 4; ++i)
            {
                instances.push_back (std::make_unique<AviatorKeyzProcessor>());
                loadFirstPresetWithSample (*instances.back());
                instances.back()->prepareToPlay (kSampleRate, 512);
            }
            const double after = residentMb();
            logMessage ("resident before 4 instances: " + juce::String (before, 1) + " MB");
            logMessage ("resident after  4 instances: " + juce::String (after, 1) + " MB");
            logMessage ("=> approx per-instance heap: " + juce::String ((after - before) / 4.0, 1) + " MB");
        }

        // Attribution: how much of that is the MFX rack preallocating all 10
        // effects in both slots, whether or not they are ever selected?
        {
            const double b2 = residentMb();
            std::vector<std::unique_ptr<MfxRack>> racks;
            juce::dsp::ProcessSpec spec { kSampleRate, 512, 2 };
            for (int i = 0; i < 4; ++i)
            {
                racks.push_back (std::make_unique<MfxRack>());
                racks.back()->prepare (spec);
            }
            const double a2 = residentMb();
            logMessage ("=> of which MfxRack (2 slots x 10 effects): "
                        + juce::String ((a2 - b2) / 4.0, 1) + " MB per instance");
        }
    }

private:
    void report (const juce::String& label, const Result& r, int blockSize)
    {
        const double budgetUs = 1.0e6 * (double) blockSize / kSampleRate;
        auto pct = [budgetUs] (double us) { return juce::String (100.0 * us / budgetUs, 2); };
        logMessage ("  " + label.paddedRight (' ', 34)
                    + " median " + juce::String (r.medianUs, 1) + " us (" + pct (r.medianUs) + "%)"
                    + "   p95 "  + juce::String (r.p95Us, 1)  + " us (" + pct (r.p95Us)  + "%)"
                    + "   max "  + juce::String (r.worstUs, 1) + " us (" + pct (r.worstUs) + "%)");
    }

    /** Builds a fresh processor with a sample loaded and everything off. */
    static void configureBaseline (AviatorKeyzProcessor& proc, int blockSize)
    {
        namespace P = AviatorKeyz::ParamID;
        loadFirstPresetWithSample (proc);
        proc.prepareToPlay (kSampleRate, blockSize);
        setParam (proc, P::FX_REVERB_ON, 0.f);
        setParam (proc, P::FX_DELAY_ON,  0.f);
        setParam (proc, P::FX_CHORUS_ON, 0.f);
        setParam (proc, P::FX_LOFI_ON,   0.f);
        setParam (proc, P::FX_DIST_ON,   0.f);
        setParam (proc, P::TEX_ENABLED,  0.f);
        setParam (proc, P::FILTER_ENABLED, 0.f);
        setParam (proc, Mfx::onId (0), 0.f);
        setParam (proc, Mfx::onId (1), 0.f);
    }

    void runScenarioSet (int blockSize)
    {
        namespace P = AviatorKeyz::ParamID;
        const double budgetUs = 1.0e6 * (double) blockSize / kSampleRate;
        beginTest ("Whole-plugin processBlock @ 48 kHz / " + juce::String (blockSize)
                   + " samples (budget " + juce::String (budgetUs, 0) + " us)");
        const int blocks = 2000;

        {
            AviatorKeyzProcessor proc;
            configureBaseline (proc, blockSize);
            report ("idle, all FX off", timeBlocks (proc, blockSize, blocks, 0), blockSize);
        }
        {
            AviatorKeyzProcessor proc;
            configureBaseline (proc, blockSize);
            report ("1 voice, all FX off", timeBlocks (proc, blockSize, blocks, 1), blockSize);
        }
        {
            AviatorKeyzProcessor proc;
            configureBaseline (proc, blockSize);
            report ("8 voices, all FX off", timeBlocks (proc, blockSize, blocks, 8), blockSize);
        }
        {
            AviatorKeyzProcessor proc;
            configureBaseline (proc, blockSize);
            report ("16 voices, all FX off", timeBlocks (proc, blockSize, blocks, 16), blockSize);
        }
        {
            AviatorKeyzProcessor proc;
            configureBaseline (proc, blockSize);
            setParam (proc, P::FX_REVERB_ON, 1.f);
            setParam (proc, P::REVERB_AMOUNT, 0.5f);
            report ("8 voices + reverb", timeBlocks (proc, blockSize, blocks, 8), blockSize);
        }
        {
            AviatorKeyzProcessor proc;
            configureBaseline (proc, blockSize);
            setParam (proc, P::TEX_ENABLED, 1.f);
            setParam (proc, P::TEX_AMOUNT, 0.5f);
            report ("8 voices + granular TEX", timeBlocks (proc, blockSize, blocks, 8), blockSize);
        }
        {
            AviatorKeyzProcessor proc;
            configureBaseline (proc, blockSize);
            setParam (proc, Mfx::onId (0), 1.f);
            setParam (proc, Mfx::effectId (0), (float) (int) Mfx::Effect::grainCloud);
            setParam (proc, Mfx::onId (1), 1.f);
            setParam (proc, Mfx::effectId (1), (float) (int) Mfx::Effect::aviationDelay);
            setParam (proc, Mfx::sendId (1), 0.4f);
            report ("8 voices + MFX grain->delay+send", timeBlocks (proc, blockSize, blocks, 8), blockSize);
        }
        {
            AviatorKeyzProcessor proc;
            configureBaseline (proc, blockSize);
            setParam (proc, P::FX_REVERB_ON, 1.f);
            setParam (proc, P::REVERB_AMOUNT, 0.5f);
            setParam (proc, P::FX_DELAY_ON, 1.f);
            setParam (proc, P::FX_CHORUS_ON, 1.f);
            setParam (proc, P::FX_LOFI_ON, 1.f);
            setParam (proc, P::FX_DIST_ON, 1.f);
            setParam (proc, P::FILTER_ENABLED, 1.f);
            setParam (proc, P::TEX_ENABLED, 1.f);
            setParam (proc, P::TEX_AMOUNT, 0.5f);
            setParam (proc, Mfx::onId (0), 1.f);
            setParam (proc, Mfx::effectId (0), (float) (int) Mfx::Effect::grainCloud);
            setParam (proc, Mfx::onId (1), 1.f);
            setParam (proc, Mfx::effectId (1), (float) (int) Mfx::Effect::aviationDelay);
            setParam (proc, Mfx::sendId (1), 0.4f);
            report ("16 voices, EVERYTHING on", timeBlocks (proc, blockSize, blocks, 16), blockSize);
        }
    }
};

static PluginCpuBenchmark pluginCpuBenchmark;
