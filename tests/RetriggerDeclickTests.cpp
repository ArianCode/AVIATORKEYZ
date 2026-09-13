// =============================================================================
//  Retrigger / voice-steal de-click tests
//
//  Uses a sine-wave sample (period 100 samples → max slope ~0.063/sample)
//  so a hard cut or hard restart shows up as a first-difference step far
//  above the natural waveform slope.
// =============================================================================

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include "DSP/SamplerEngine.h"
#include "State/SampleLibrary.h"

namespace
{
constexpr int kFrames = 1 << 16;
float sineSample[kFrames];

struct DeclickRig
{
    SampleLibrary::AudioSnapshot snapshot;
    SamplerEngine engine;

    DeclickRig()
    {
        for (int i = 0; i < kFrames; ++i)
            sineSample[i] = std::sin (juce::MathConstants<float>::twoPi
                                      * static_cast<float> (i) / 100.f);

        SampleLibrary::AudioRegion region;
        region.data = sineSample;
        region.numFrames = kFrames;
        region.rootNote = 60;
        region.noteMin = 0;
        region.noteMax = 127;
        region.fileSampleRate = 44100.0;
        snapshot.regions.push_back (region);

        const juce::dsp::ProcessSpec spec { 44100.0, 256, 2 };
        engine.prepare (spec);
        engine.setSampleSnapshot (&snapshot);
        engine.setVelocitySensitivity (0.f);
        engine.setEnvelopeTimesMs (0.f, 0.f, 1.f, 60.f);
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

    /** Render one sample at a time until |value| exceeds the threshold. */
    void renderUntilLoud (std::vector<float>& out, float threshold, int maxSamples = 4000)
    {
        for (int i = 0; i < maxSamples; ++i)
        {
            render (out, 1);
            if (std::abs (out.back()) > threshold)
                return;
        }
    }
};

float maxStep (const std::vector<float>& v, size_t from)
{
    float m = 0.f;
    for (size_t i = std::max<size_t> (1, from); i < v.size(); ++i)
        m = juce::jmax (m, std::abs (v[i] - v[i - 1]));
    return m;
}
} // namespace

class RetriggerDeclickTests : public juce::UnitTest
{
public:
    RetriggerDeclickTests() : juce::UnitTest ("RetriggerDeclick", "AviatorKeyz") {}

    void runTest() override
    {
        // Natural per-sample slope of the test sine is 2*pi/100 ≈ 0.063.
        // During a 5 ms choke + 1.5 ms fade-in, two voices overlap, so allow
        // roughly the summed slopes plus fade derivatives.
        constexpr float kSmoothBound = 0.2f;

        beginTest ("Mono retrigger crossfades instead of hard-cutting the old note");
        {
            DeclickRig rig;
            rig.engine.setPlayMode (1); // mono

            std::vector<float> out;
            rig.engine.noteOn (60, 1.f, false, 0.f);
            rig.render (out, 500);
            rig.renderUntilLoud (out, 0.7f); // retrigger away from a zero crossing

            const size_t boundary = out.size();
            rig.engine.noteOn (62, 1.f, false, 0.f);
            rig.render (out, 800);

            const float step = maxStep (out, boundary - 1);
            expect (step < kSmoothBound,
                    "mono retrigger discontinuity: " + juce::String (step, 4));
        }

        beginTest ("Legato retrigger is also smooth");
        {
            DeclickRig rig;
            rig.engine.setPlayMode (2); // legato

            std::vector<float> out;
            rig.engine.noteOn (60, 1.f, false, 0.f);
            rig.render (out, 500);
            rig.renderUntilLoud (out, 0.7f);

            const size_t boundary = out.size();
            rig.engine.noteOn (65, 1.f, false, 0.f);
            rig.render (out, 800);

            const float step = maxStep (out, boundary - 1);
            expect (step < kSmoothBound,
                    "legato retrigger discontinuity: " + juce::String (step, 4));
        }

        beginTest ("Mono note-off releases the note (was previously ignored)");
        {
            DeclickRig rig;
            rig.engine.setPlayMode (1);

            std::vector<float> out;
            rig.engine.noteOn (60, 1.f, false, 0.f);
            rig.render (out, 300);
            expect (rig.engine.hasActiveVoices());

            rig.engine.noteOff (60);
            rig.render (out, 44100 / 2); // release is 60 ms
            expect (! rig.engine.hasActiveVoices(), "mono voice must release on note-off");
        }

        beginTest ("Stolen voice fades its new note in (soft onset)");
        {
            DeclickRig rig;
            rig.engine.setPolyphony (1); // force stealing on the second note

            std::vector<float> out;
            rig.engine.noteOn (60, 1.f, false, 0.f);
            rig.render (out, 400);

            std::vector<float> after;
            rig.engine.noteOn (67, 1.f, false, 0.f);
            rig.render (after, 400);

            // First samples after the steal must be near-silent (fade-in),
            // then the note must reach normal level.
            for (size_t i = 0; i < 3; ++i)
                expect (std::abs (after[i]) < 0.1f, "steal onset not faded");

            float peak = 0.f;
            for (size_t i = 100; i < after.size(); ++i)
                peak = juce::jmax (peak, std::abs (after[i]));
            expect (peak > 0.5f, "stolen voice did not reach level");
        }

        beginTest ("Stolen voice's old waveform decays out instead of stepping to zero");
        {
            DeclickRig rig;
            rig.engine.setPolyphony (1);

            std::vector<float> out;
            rig.engine.noteOn (60, 1.f, false, 0.f);
            rig.render (out, 500);
            rig.renderUntilLoud (out, 0.7f); // steal away from a zero crossing

            const size_t boundary = out.size();
            rig.engine.noteOn (67, 1.f, false, 0.f);
            rig.render (out, 800);

            const float step = maxStep (out, boundary - 1);
            expect (step < kSmoothBound, "voice steal discontinuity: " + juce::String (step, 4));
        }

        beginTest ("CHOP FADE stretches the fade at a slice edge (manual chop repair)");
        {
            // Frame 25 is the sine's peak, so this slice starts on a hard step.
            // With a 10 ms chop fade the onset must take 10 ms to arrive, not
            // the 1.5 ms built-in minimum.
            DeclickRig rig;
            rig.engine.setSliceCrossfadeMs (10.f);
            rig.engine.setNextNoteSliceWindow (25, 8025);

            std::vector<float> out;
            out.push_back (0.f);     // silence before the note
            rig.engine.noteOn (60, 1.f, false, 0.f);
            rig.render (out, 1200);

            expect (maxStep (out, 1) < kSmoothBound,
                    "chop onset discontinuity: " + juce::String (maxStep (out, 1), 4));

            // 10 ms at 44.1 kHz is 441 samples, so a quarter of the way in the
            // ramp is still well below full level; by 600 it has arrived.
            float earlyPeak = 0.f;
            for (size_t i = 0; i < 110 && i < out.size(); ++i)
                earlyPeak = juce::jmax (earlyPeak, std::abs (out[i]));
            float latePeak = 0.f;
            for (size_t i = 600; i < out.size(); ++i)
                latePeak = juce::jmax (latePeak, std::abs (out[i]));
            expect (earlyPeak < 0.45f, "10 ms fade must still be ramping at 2.5 ms: " + juce::String (earlyPeak, 3));
            expect (latePeak > 0.7f, "the slice must reach full level after the fade: " + juce::String (latePeak, 3));
        }

        beginTest ("Slice pad starting mid-waveform fades in (chop onset click)");
        {
            DeclickRig rig;
            // Frame 25 is the sine's positive peak: a hard start would open at 1.0.
            rig.engine.setNextNoteSliceWindow (25, 8025);

            std::vector<float> out;
            rig.engine.noteOn (60, 1.f, false, 0.f);
            rig.render (out, 600);

            for (size_t i = 0; i < 3; ++i)
                expect (std::abs (out[i]) < 0.1f, "slice onset not faded: " + juce::String (out[i], 3));
            expect (maxStep (out, 1) < kSmoothBound, "slice onset step: " + juce::String (maxStep (out, 1), 4));

            float peak = 0.f;
            for (size_t i = 200; i < out.size(); ++i)
                peak = juce::jmax (peak, std::abs (out[i]));
            expect (peak > 0.5f, "slice did not reach level");
        }

        beginTest ("Non-looping sample end fades out even with no envelope release (slice end click)");
        {
            DeclickRig rig;
            // 2025 frames ends the sine on its positive peak; release 0 used to hard-cut there.
            SampleLibrary::AudioSnapshot shortSnap;
            SampleLibrary::AudioRegion region;
            region.data = sineSample;
            region.numFrames = 2025;
            region.rootNote = 60;
            region.fileSampleRate = 44100.0;
            shortSnap.regions.push_back (region);
            rig.engine.setSampleSnapshot (&shortSnap);
            rig.engine.setEnvelopeTimesMs (0.f, 0.f, 1.f, 0.f);

            std::vector<float> out;
            rig.engine.noteOn (60, 1.f, false, 0.f);
            rig.render (out, 2600);

            expect (! rig.engine.hasActiveVoices(), "one-shot ran to its end and freed the voice");
            const float step = maxStep (out, 1);
            expect (step < kSmoothBound, "sample-end discontinuity: " + juce::String (step, 4));
            expect (std::abs (out[2024]) < 0.1f, "last sample of the window is faded: " + juce::String (out[2024], 3));
        }

        beginTest ("Rapid alternating retriggers stay bounded and end silent");
        {
            DeclickRig rig;
            rig.engine.setPlayMode (1);

            std::vector<float> out;
            for (int i = 0; i < 40; ++i)
            {
                rig.engine.noteOn (48 + (i % 5) * 3, 1.f, false, 0.f);
                rig.render (out, 64); // ~1.5 ms between notes
            }

            float peak = 0.f;
            for (float v : out)
                peak = juce::jmax (peak, std::abs (v));
            expect (peak < 4.f, "retrigger pile-up: peak = " + juce::String (peak, 3));

            rig.engine.allNotesOff();
            std::vector<float> tail;
            rig.render (tail, 44100);
            expect (! rig.engine.hasActiveVoices(), "voices must all release");
        }
    }
};

static RetriggerDeclickTests retriggerDeclickTests;
