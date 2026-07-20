// =============================================================================
//  Envelope determinism tests
//
//  Uses a constant-1.0 sample so the rendered output equals the amp-envelope
//  level directly (velocity sensitivity 0 → velocity gain 1).
// =============================================================================

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include "DSP/SamplerEngine.h"
#include "State/SampleLibrary.h"

namespace
{
constexpr int kFrames = 1 << 16; // ~1.5 s at 44.1k — long enough for all cases
float constantSample[kFrames];

struct EnvTestRig
{
    SampleLibrary::AudioSnapshot snapshot;
    SamplerEngine engine;

    EnvTestRig()
    {
        for (int i = 0; i < kFrames; ++i)
            constantSample[i] = 1.f;

        SampleLibrary::AudioRegion region;
        region.data = constantSample;
        region.numFrames = kFrames;
        region.rootNote = 60;
        region.noteMin = 0;
        region.noteMax = 127;
        region.fileSampleRate = 44100.0;
        snapshot.regions.push_back (region);

        const juce::dsp::ProcessSpec spec { 44100.0, 512, 2 };
        engine.prepare (spec);
        engine.setSampleSnapshot (&snapshot);
        engine.setVelocitySensitivity (0.f);
    }

    /** Render n samples of channel 0 into out. */
    void render (std::vector<float>& out, int n)
    {
        juce::AudioBuffer<float> buf (2, 256);
        int done = 0;
        while (done < n)
        {
            const int len = juce::jmin (256, n - done);
            buf.clear();
            juce::AudioBuffer<float> view (buf.getArrayOfWritePointers(), 2, len);
            engine.process (view);
            for (int i = 0; i < len; ++i)
                out.push_back (buf.getSample (0, i));
            done += len;
        }
    }
};

float maxStep (const std::vector<float>& v, size_t from = 1)
{
    float m = 0.f;
    for (size_t i = std::max<size_t> (1, from); i < v.size(); ++i)
        m = juce::jmax (m, std::abs (v[i] - v[i - 1]));
    return m;
}
} // namespace

class EnvelopeTests : public juce::UnitTest
{
public:
    EnvelopeTests() : juce::UnitTest ("Envelope", "AviatorKeyz") {}

    void runTest() override
    {
        beginTest ("Zero attack + decay: level starts at 1 and decays smoothly (no jump to sustain)");
        {
            EnvTestRig rig;
            rig.engine.setEnvelopeTimesMs (0.f, 100.f, 0.5f, 50.f);
            rig.engine.noteOn (60, 1.f, false, 0.f);

            std::vector<float> out;
            rig.render (out, 2000);

            expectWithinAbsoluteError (out[0], 1.f, 1.0e-4f);
            // Decay over 100 ms at 44.1 kHz: per-sample step = 0.5/4410 ≈ 1.1e-4.
            // The old bug jumped 1.0 → 0.5 in a single sample.
            expect (maxStep (out) < 0.001f,
                    "envelope discontinuity: max step = " + juce::String (maxStep (out), 6));
            // After ~45 ms it should be roughly halfway between 1.0 and sustain.
            expectWithinAbsoluteError (out[1984], 1.f - 0.5f * (1984.f / 4410.f), 0.02f);
        }

        beginTest ("Zero attack + zero decay: constant level while held");
        {
            EnvTestRig rig;
            rig.engine.setEnvelopeTimesMs (0.f, 0.f, 1.f, 50.f);
            rig.engine.noteOn (60, 1.f, false, 0.f);

            std::vector<float> out;
            rig.render (out, 1000);
            for (size_t i = 0; i < out.size(); ++i)
                expectWithinAbsoluteError (out[i], 1.f, 1.0e-4f);
        }

        beginTest ("Short attack ramps up without overshoot");
        {
            EnvTestRig rig;
            rig.engine.setEnvelopeTimesMs (2.f, 0.f, 1.f, 50.f);
            rig.engine.noteOn (60, 1.f, false, 0.f);

            std::vector<float> out;
            rig.render (out, 500);

            float prev = -1.f;
            for (size_t i = 0; i < 88; ++i) // 2 ms ≈ 88 samples
            {
                expect (out[i] >= prev - 1.0e-5f, "attack must be monotonic");
                prev = out[i];
            }
            for (size_t i = 100; i < out.size(); ++i)
                expect (out[i] <= 1.f + 1.0e-4f, "attack must not overshoot");
        }

        beginTest ("Minimum release fades out and frees the voice");
        {
            EnvTestRig rig;
            rig.engine.setEnvelopeTimesMs (0.f, 0.f, 1.f, 0.01f);
            rig.engine.noteOn (60, 1.f, false, 0.f);
            std::vector<float> out;
            rig.render (out, 100);
            rig.engine.noteOff (60);
            rig.render (out, 400);
            expect (! rig.engine.hasActiveVoices(), "voice must be freed after minimum release");
            expectWithinAbsoluteError (out.back(), 0.f, 1.0e-4f);
        }

        beginTest ("Retriggered note has an identical envelope trajectory (no stale segment state)");
        {
            EnvTestRig rig;
            rig.engine.setEnvelopeTimesMs (0.f, 80.f, 0.6f, 30.f);

            rig.engine.noteOn (60, 1.f, false, 0.f);
            std::vector<float> first;
            rig.render (first, 800);
            rig.engine.noteOff (60);
            std::vector<float> tail;
            rig.render (tail, 4000); // let release finish fully
            expect (! rig.engine.hasActiveVoices());

            rig.engine.noteOn (60, 1.f, false, 0.f);
            std::vector<float> second;
            rig.render (second, 800);

            float maxDiff = 0.f;
            for (size_t i = 0; i < first.size(); ++i)
                maxDiff = juce::jmax (maxDiff, std::abs (first[i] - second[i]));
            expect (maxDiff < 1.0e-4f,
                    "retriggered envelope diverges: " + juce::String (maxDiff, 6));
        }

        beginTest ("Stolen voice with zero attack does not inherit the old voice's ramp");
        {
            EnvTestRig rig;
            rig.engine.setEnvelopeTimesMs (0.f, 0.f, 1.f, 200.f);
            rig.engine.setPolyphony (1);

            rig.engine.noteOn (60, 1.f, false, 0.f);
            std::vector<float> out;
            rig.render (out, 500);

            rig.engine.noteOn (62, 1.f, false, 0.f); // steals the only voice
            std::vector<float> after;
            rig.render (after, 500);

            // With decay 0 / sustain 1 the stolen voice must hold full level.
            // The old bug re-used the choke ramp and decayed toward silence.
            expect (after[300] > 0.9f,
                    "stolen voice lost level: " + juce::String (after[300], 4));
        }
    }
};

static EnvelopeTests envelopeTests;
