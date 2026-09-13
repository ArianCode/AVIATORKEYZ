// =============================================================================
//  LoopPointTests — sustain loop points (src_loop_start / src_loop_end):
//  the engine keeps a held note alive past the sample end, the seam is
//  crossfaded (no step), and auto-detection lands on matching zero crossings.
// =============================================================================

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include "DSP/SamplerEngine.h"
#include "State/SampleAnalysis.h"
#include "State/SampleLibrary.h"

namespace
{
constexpr int kFrames = 1 << 16;
float loopSine[kFrames];

struct LoopRig
{
    SampleLibrary::AudioSnapshot snapshot;
    SamplerEngine engine;

    LoopRig()
    {
        for (int i = 0; i < kFrames; ++i)
            loopSine[i] = std::sin (juce::MathConstants<float>::twoPi * static_cast<float> (i) / 100.f);

        SampleLibrary::AudioRegion region;
        region.data = loopSine;
        region.numFrames = kFrames;
        region.rootNote = 60;
        region.fileSampleRate = 44100.0;
        snapshot.regions.push_back (region);

        const juce::dsp::ProcessSpec spec { 44100.0, 256, 2 };
        engine.prepare (spec);
        engine.setSampleSnapshot (&snapshot);
        engine.setVelocitySensitivity (0.f);
        engine.setEnvelopeTimesMs (0.f, 0.f, 1.f, 20.f);
    }

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

float maxStepFrom (const std::vector<float>& v, size_t from)
{
    float m = 0.f;
    for (size_t i = std::max<size_t> (1, from); i < v.size(); ++i)
        m = juce::jmax (m, std::abs (v[i] - v[i - 1]));
    return m;
}
} // namespace

class LoopPointTests : public juce::UnitTest
{
public:
    LoopPointTests() : juce::UnitTest ("LoopPoints", "AviatorKeyz") {}

    void runTest() override
    {
        // Natural per-sample slope of the test sine is 2*pi/100 ≈ 0.063.
        constexpr float kSmoothBound = 0.2f;

        beginTest ("Sustain loop holds a note past the sample end and never steps at the seam");
        {
            LoopRig rig;
            SourceSettings s;
            s.playbackMode = SamplePlaybackMode::ChromaticResample;
            s.keytrack = true;
            s.loopMode = LoopMode::Loop;
            s.bpmSync = false;
            // 655-frame loop = 6.55 sine periods: the raw wrap would jump half a cycle.
            s.loopStart = 20000.f / (float) (kFrames - 1);
            s.loopEnd   = 20655.f / (float) (kFrames - 1);
            rig.engine.setSourceSettings (s, 120.0);

            std::vector<float> out;
            rig.engine.noteOn (60, 1.f, false, 0.f);
            rig.render (out, kFrames + 20000); // well past the 65536-frame sample

            expect (rig.engine.hasActiveVoices(), "held note must still be sounding after the sample length");
            float tailPeak = 0.f;
            for (size_t i = out.size() - 4000; i < out.size(); ++i)
                tailPeak = juce::jmax (tailPeak, std::abs (out[i]));
            expect (tailPeak > 0.5f, "loop keeps producing signal: " + juce::String (tailPeak, 3));

            const float step = maxStepFrom (out, 20000);
            expect (step < kSmoothBound, "loop seam discontinuity: " + juce::String (step, 4));
        }

        beginTest ("Default loop points (0..1) reproduce the whole-window loop");
        {
            LoopRig rig;
            SourceSettings s;
            s.playbackMode = SamplePlaybackMode::ChromaticResample;
            s.keytrack = true;
            s.loopMode = LoopMode::Loop;
            s.bpmSync = false;
            rig.engine.setSourceSettings (s, 120.0);

            std::vector<float> out;
            rig.engine.noteOn (60, 1.f, false, 0.f);
            rig.render (out, kFrames + 5000);
            expect (rig.engine.hasActiveVoices(), "whole-window loop still sounding");
        }

        beginTest ("Loop points shorter than the minimum fall back to the window (no stall)");
        {
            LoopRig rig;
            SourceSettings s;
            s.playbackMode = SamplePlaybackMode::ChromaticResample;
            s.keytrack = true;
            s.loopMode = LoopMode::Loop;
            s.bpmSync = false;
            s.loopStart = 0.5f;
            s.loopEnd = 0.5f + 8.f / (float) kFrames; // 8 frames
            rig.engine.setSourceSettings (s, 120.0);

            std::vector<float> out;
            rig.engine.noteOn (60, 1.f, false, 0.f);
            rig.render (out, 40000);
            float peak = 0.f;
            for (size_t i = 30000; i < out.size(); ++i)
                peak = juce::jmax (peak, std::abs (out[i]));
            expect (peak > 0.5f, "voice keeps playing through the window: " + juce::String (peak, 3));
        }

        beginTest ("findSustainLoop picks rising zero crossings inside the sustain of a decaying tone");
        {
            constexpr double sr = 44100.0;
            constexpr int n = 44100; // 1 s
            std::vector<float> tone ((size_t) n);
            for (int i = 0; i < n; ++i)
            {
                const float env = std::exp (-2.5f * (float) i / (float) n);
                tone[(size_t) i] = env * std::sin (juce::MathConstants<float>::twoPi * 220.f * (float) i / (float) sr);
            }

            const auto lp = SampleAnalysis::findSustainLoop (tone.data(), n, sr, 0.f, 1.f);
            expect (lp.found);
            expect (lp.startNorm >= 0.39f && lp.startNorm <= 0.5f, "start in the sustain: " + juce::String (lp.startNorm, 3));
            expect (lp.endNorm > lp.startNorm + 0.04f && lp.endNorm <= 0.96f, "end later, before the tail: " + juce::String (lp.endNorm, 3));

            const int a = (int) (lp.startNorm * (float) (n - 1));
            const int b = (int) (lp.endNorm * (float) (n - 1));
            expect (tone[(size_t) a - 1] <= 0.f && tone[(size_t) a] > 0.f, "start is a rising zero crossing");
            expect (tone[(size_t) b - 1] <= 0.f && tone[(size_t) b] > 0.f, "end is a rising zero crossing");

            // Following waveforms line up in phase: the seam error is small relative to the level there.
            float err = 0.f, level = 0.f;
            for (int k = 0; k < 128; ++k)
            {
                err = juce::jmax (err, std::abs (tone[(size_t) (b + k)] - tone[(size_t) (a + k)]));
                level = juce::jmax (level, std::abs (tone[(size_t) (a + k)]));
            }
            expect (err < level * 0.5f, "seam mismatch " + juce::String (err, 3) + " vs level " + juce::String (level, 3));
        }

        beginTest ("findSustainLoop on silence still returns a usable sustain region");
        {
            std::vector<float> silence (44100, 0.f);
            const auto lp = SampleAnalysis::findSustainLoop (silence.data(), (int) silence.size(), 44100.0, 0.f, 1.f);
            expect (lp.found);
            expect (lp.endNorm > lp.startNorm + 0.1f);
        }
    }
};

static LoopPointTests loopPointTests;
