// =============================================================================
//  SamplerEngine unit tests
//
//  API: setSampleSnapshot(const SampleLibrary::AudioSnapshot*)
//
//  Tests cover:
//    - noteOn with sample data produces non-zero audio
//    - Pitch transposition: note above root drives faster read position
//    - Reverse mode produces different waveform than forward
//    - allSoundOff silences immediately
//    - noteOff: voice persists during release phase
//    - 16 simultaneous voices: no crash
//    - Voice stealing: 17th noteOn: no crash
//    - Glide at 0ms / 100ms: no crash, produces audio
//    - Sine fallback (no sample table): produces non-zero audio
// =============================================================================

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include "DSP/SamplerEngine.h"
#include "DSP/Performance/PerformanceTypes.h"
#include "State/CategorySoundPolicy.h"
#include "State/SampleLibrary.h"

// ---------------------------------------------------------------------------
// Static sine-wave sample buffer (shared across tests to avoid repeated alloc)
// ---------------------------------------------------------------------------
namespace TestSamples
{
    static constexpr int kFrames = 4096;
    static float sine4096[kFrames] = {};

    static void init()
    {
        static bool done = false;
        if (done) return;
        done = true;
        for (int i = 0; i < kFrames; ++i)
            sine4096[i] = std::sin (juce::MathConstants<float>::twoPi
                                    * 440.0f * static_cast<float> (i) / 44100.0f);
    }
}

struct TestSampleSnapshot
{
    SampleLibrary::AudioSnapshot snapshot;
    SampleLibrary::AudioRegion   region;

    void setMono (const float* data, int numFrames, int rootNote = 60) noexcept
    {
        region.data        = data;
        region.numFrames   = numFrames;
        region.rootNote    = rootNote;
        region.noteMin     = 0;
        region.noteMax     = 127;
        region.velocityMin = 0.0f;
        region.velocityMax = 1.0f;
        snapshot.regions.clear();
        if (data != nullptr && numFrames > 0)
            snapshot.regions.push_back (region);
    }
};

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

class SamplerEnginePitchTests : public juce::UnitTest
{
public:
    SamplerEnginePitchTests() : juce::UnitTest ("SamplerEngine_Pitch", "AviatorKeyz") {}

    void runTest() override
    {
        TestSamples::init();

        beginTest ("noteOn with sample table produces non-zero audio");
        {
            SamplerEngine engine;
            juce::dsp::ProcessSpec spec { 44100.0, 512, 2 };
            engine.prepare (spec);
            TestSampleSnapshot snap;
            snap.setMono (TestSamples::sine4096, TestSamples::kFrames, 60);
            engine.setSampleSnapshot (&snap.snapshot);
            engine.setEnvelopeTimesMs (0.5f, 300.f, 1.f, 50.0f);
            engine.noteOn (60, 1.0f, false, 0.0f);

            juce::AudioBuffer<float> buf (2, 512);
            buf.clear();
            engine.process (buf);

            float maxAbs = 0.0f;
            for (int i = 0; i < 512; ++i)
                maxAbs = std::max (maxAbs, std::abs (buf.getSample (0, i)));

            expect (maxAbs > 0.0f,
                    "noteOn with valid sample should produce non-zero audio");
            engine.allSoundOff();
        }

        beginTest ("Both root note and octave-up note produce audio");
        {
            SamplerEngine engine;
            juce::dsp::ProcessSpec spec { 44100.0, 256, 2 };
            engine.prepare (spec);
            TestSampleSnapshot snap;
            snap.setMono (TestSamples::sine4096, TestSamples::kFrames, 60);
            engine.setSampleSnapshot (&snap.snapshot);
            engine.setEnvelopeTimesMs (0.5f, 300.f, 1.f, 50.0f);

            juce::AudioBuffer<float> buf (2, 256);

            engine.noteOn (60, 1.0f, false, 0.0f);
            buf.clear(); engine.process (buf);
            float maxRoot = 0.0f;
            for (int i = 0; i < 256; ++i) maxRoot = std::max (maxRoot, std::abs (buf.getSample (0, i)));
            engine.allSoundOff();

            engine.noteOn (72, 1.0f, false, 0.0f);  // one octave up
            buf.clear(); engine.process (buf);
            float maxHigh = 0.0f;
            for (int i = 0; i < 256; ++i) maxHigh = std::max (maxHigh, std::abs (buf.getSample (0, i)));
            engine.allSoundOff();

            expect (maxRoot > 0.0f, "Root note must produce audio");
            expect (maxHigh > 0.0f, "Octave-up note must produce audio");
        }
    }
};

class SamplerEngineVoiceTests : public juce::UnitTest
{
public:
    SamplerEngineVoiceTests() : juce::UnitTest ("SamplerEngine_Voices", "AviatorKeyz") {}

    void runTest() override
    {
        TestSamples::init();

        beginTest ("allSoundOff silences all voices immediately");
        {
            SamplerEngine engine;
            juce::dsp::ProcessSpec spec { 44100.0, 512, 2 };
            engine.prepare (spec);
            TestSampleSnapshot snap;
            snap.setMono (TestSamples::sine4096, TestSamples::kFrames, 60);
            engine.setSampleSnapshot (&snap.snapshot);
            engine.setEnvelopeTimesMs (0.5f, 300.f, 1.f, 500.0f);

            for (int n = 60; n < 68; ++n)
                engine.noteOn (n, 0.8f, false, 0.0f);

            engine.allSoundOff();

            juce::AudioBuffer<float> buf (2, 512);
            buf.clear();
            engine.process (buf);

            float maxAbs = 0.0f;
            for (int i = 0; i < 512; ++i)
                maxAbs = std::max (maxAbs, std::abs (buf.getSample (0, i)));

            expect (maxAbs < 1e-6f,
                    "allSoundOff must silence output immediately (got max=" +
                    juce::String (maxAbs) + ")");
        }

        beginTest ("noteOff: voice still active during release phase");
        {
            SamplerEngine engine;
            juce::dsp::ProcessSpec spec { 44100.0, 512, 2 };
            engine.prepare (spec);
            TestSampleSnapshot snap;
            snap.setMono (TestSamples::sine4096, TestSamples::kFrames, 60);
            engine.setSampleSnapshot (&snap.snapshot);
            engine.setEnvelopeTimesMs (0.5f, 300.f, 1.f, 200.0f);  // 200ms release

            engine.noteOn (60, 1.0f, false, 0.0f);

            juce::AudioBuffer<float> buf (2, 512);
            buf.clear();
            engine.process (buf);

            engine.noteOff (60);

            buf.clear();
            engine.process (buf);  // should still have audio in release

            float maxAbs = 0.0f;
            for (int i = 0; i < 512; ++i)
                maxAbs = std::max (maxAbs, std::abs (buf.getSample (0, i)));

            expect (maxAbs > 0.0f,
                    "During 200ms release, voice must still produce audio immediately after noteOff");
        }

        beginTest ("16 simultaneous voices — no crash");
        {
            SamplerEngine engine;
            juce::dsp::ProcessSpec spec { 44100.0, 256, 2 };
            engine.prepare (spec);
            TestSampleSnapshot snap;
            snap.setMono (TestSamples::sine4096, TestSamples::kFrames, 60);
            engine.setSampleSnapshot (&snap.snapshot);
            engine.setEnvelopeTimesMs (0.5f, 300.f, 1.f, 100.0f);

            for (int n = 48; n < 64; ++n)  // 16 = kMaxVoices
                engine.noteOn (n, 0.5f, false, 0.0f);

            juce::AudioBuffer<float> buf (2, 256);
            buf.clear();
            engine.process (buf);

            expect (true, "16 simultaneous voices should not crash");
        }

        beginTest ("17th noteOn triggers voice steal — no crash");
        {
            SamplerEngine engine;
            juce::dsp::ProcessSpec spec { 44100.0, 256, 2 };
            engine.prepare (spec);
            TestSampleSnapshot snap;
            snap.setMono (TestSamples::sine4096, TestSamples::kFrames, 60);
            engine.setSampleSnapshot (&snap.snapshot);
            engine.setEnvelopeTimesMs (0.5f, 300.f, 1.f, 100.0f);

            for (int n = 40; n < 57; ++n)  // 17 notes — steals one
                engine.noteOn (n, 0.5f, false, 0.0f);

            juce::AudioBuffer<float> buf (2, 256);
            buf.clear();
            engine.process (buf);

            expect (true, "Voice stealing with 17 notes must not crash");
        }
    }
};

class SamplerEngineReverseTests : public juce::UnitTest
{
public:
    SamplerEngineReverseTests() : juce::UnitTest ("SamplerEngine_Reverse", "AviatorKeyz") {}

    void runTest() override
    {
        TestSamples::init();

        beginTest ("Reverse mode produces non-zero audio");
        {
            SamplerEngine engine;
            juce::dsp::ProcessSpec spec { 44100.0, 512, 2 };
            engine.prepare (spec);
            TestSampleSnapshot snap;
            snap.setMono (TestSamples::sine4096, TestSamples::kFrames, 60);
            engine.setSampleSnapshot (&snap.snapshot);
            engine.setEnvelopeTimesMs (0.5f, 300.f, 1.f, 50.0f);

            engine.noteOn (60, 1.0f, true, 0.0f);  // reverse=true

            juce::AudioBuffer<float> buf (2, 512);
            buf.clear();
            engine.process (buf);

            float maxAbs = 0.0f;
            for (int i = 0; i < 512; ++i)
                maxAbs = std::max (maxAbs, std::abs (buf.getSample (0, i)));

            expect (maxAbs > 0.0f, "Reverse mode must produce non-zero audio");
            engine.allSoundOff();
        }

        beginTest ("Reverse and forward produce different waveforms from same sample");
        {
            juce::AudioBuffer<float> fwdBuf (2, 512), revBuf (2, 512);

            {
                SamplerEngine engine;
                juce::dsp::ProcessSpec spec { 44100.0, 512, 2 };
                engine.prepare (spec);
                TestSampleSnapshot snap;
            snap.setMono (TestSamples::sine4096, TestSamples::kFrames, 60);
            engine.setSampleSnapshot (&snap.snapshot);
                engine.setEnvelopeTimesMs (0.5f, 300.f, 1.f, 50.0f);
                engine.noteOn (60, 1.0f, false, 0.0f);
                fwdBuf.clear();
                engine.process (fwdBuf);
            }

            {
                SamplerEngine engine;
                juce::dsp::ProcessSpec spec { 44100.0, 512, 2 };
                engine.prepare (spec);
                TestSampleSnapshot snap;
            snap.setMono (TestSamples::sine4096, TestSamples::kFrames, 60);
            engine.setSampleSnapshot (&snap.snapshot);
                engine.setEnvelopeTimesMs (0.5f, 300.f, 1.f, 50.0f);
                engine.noteOn (60, 1.0f, true, 0.0f);  // reversed
                revBuf.clear();
                engine.process (revBuf);
            }

            float sumDiff = 0.0f;
            for (int i = 0; i < 512; ++i)
                sumDiff += std::abs (fwdBuf.getSample (0, i) - revBuf.getSample (0, i));

            expect (sumDiff > 0.01f,
                    "Reversed playback must differ from forward playback "
                    "(sumDiff=" + juce::String (sumDiff) + ")");
        }
    }
};

class SamplerEngineGlideTests : public juce::UnitTest
{
public:
    SamplerEngineGlideTests() : juce::UnitTest ("SamplerEngine_Glide", "AviatorKeyz") {}

    void runTest() override
    {
        TestSamples::init();

        beginTest ("Glide at 0ms: instant note transition, no crash");
        {
            SamplerEngine engine;
            juce::dsp::ProcessSpec spec { 44100.0, 256, 2 };
            engine.prepare (spec);
            TestSampleSnapshot snap;
            snap.setMono (TestSamples::sine4096, TestSamples::kFrames, 60);
            engine.setSampleSnapshot (&snap.snapshot);
            engine.setEnvelopeTimesMs (0.5f, 300.f, 1.f, 50.0f);

            engine.noteOn (60, 1.0f, false, 0.0f);

            juce::AudioBuffer<float> buf (2, 256);
            buf.clear();
            engine.process (buf);

            engine.noteOn (72, 1.0f, false, 0.0f);  // instant: glide=0
            buf.clear();
            engine.process (buf);

            float maxAbs = 0.0f;
            for (int i = 0; i < 256; ++i) maxAbs = std::max (maxAbs, std::abs (buf.getSample (0, i)));
            expect (maxAbs > 0.0f, "After instant noteOn (glide=0), engine must produce audio");
            engine.allSoundOff();
        }

        beginTest ("Glide at 100ms: pitch ramp without setGlideMode");
        {
            SamplerEngine engine;
            juce::dsp::ProcessSpec spec { 44100.0, 256, 2 };
            engine.prepare (spec);
            engine.setPlayMode (2); // legato — single voice for measurable pitch
            engine.setEnvelopeTimesMs (0.5f, 300.f, 1.f, 500.0f);

            engine.noteOn (60, 1.0f, false, 0.0f);
            juce::AudioBuffer<float> buf (2, 256);
            buf.clear(); engine.process (buf);

            engine.noteOn (72, 1.0f, false, 100.0f);

            auto estimateZeroCrossingHz = [] (const juce::AudioBuffer<float>& buffer, int numSamples) -> float
            {
                int crossings = 0;
                const float* data = buffer.getReadPointer (0);
                for (int i = 1; i < numSamples; ++i)
                {
                    if ((data[i - 1] >= 0.f && data[i] < 0.f)
                        || (data[i - 1] < 0.f && data[i] >= 0.f))
                        ++crossings;
                }
                return static_cast<float> (crossings) * 0.5f * 44100.f / static_cast<float> (numSamples);
            };

            buf.clear();
            engine.process (buf);
            const float earlyHz = estimateZeroCrossingHz (buf, buf.getNumSamples());

            for (int block = 0; block < 6; ++block)
            {
                buf.clear();
                engine.process (buf);
            }
            const float midGlideHz = estimateZeroCrossingHz (buf, buf.getNumSamples());

            expect (midGlideHz > earlyHz + 15.f,
                    "Glide must raise pitch over time (early="
                    + juce::String (earlyHz, 1)
                    + " Hz, mid="
                    + juce::String (midGlideHz, 1)
                    + " Hz)");
            engine.allSoundOff();
        }
    }
};

class SamplerEngineSineFallbackTests : public juce::UnitTest
{
public:
    SamplerEngineSineFallbackTests() : juce::UnitTest ("SamplerEngine_SineFallback", "AviatorKeyz") {}

    void runTest() override
    {
        beginTest ("No sample table: sine fallback produces non-zero audio");
        {
            SamplerEngine engine;
            juce::dsp::ProcessSpec spec { 44100.0, 512, 2 };
            engine.prepare (spec);
            // Deliberately do NOT set a sample table
            engine.setEnvelopeTimesMs (0.5f, 300.f, 1.f, 50.0f);
            engine.noteOn (60, 1.0f, false, 0.0f);

            juce::AudioBuffer<float> buf (2, 512);
            buf.clear();
            engine.process (buf);

            float maxAbs = 0.0f;
            for (int i = 0; i < 512; ++i)
                maxAbs = std::max (maxAbs, std::abs (buf.getSample (0, i)));

            expect (maxAbs > 0.0f,
                    "Sine fallback must produce non-zero audio when no sample is set");
            engine.allSoundOff();
        }

        beginTest ("empty sample snapshot: sine fallback active");
        {
            SamplerEngine engine;
            juce::dsp::ProcessSpec spec { 44100.0, 256, 2 };
            engine.prepare (spec);
            TestSampleSnapshot snap;
            snap.setMono (nullptr, 0, 60);
            engine.setSampleSnapshot (&snap.snapshot);
            engine.setEnvelopeTimesMs (0.5f, 300.f, 1.f, 50.0f);
            engine.noteOn (69, 1.0f, false, 0.0f);  // A4

            juce::AudioBuffer<float> buf (2, 256);
            buf.clear();
            engine.process (buf);

            float maxAbs = 0.0f;
            for (int i = 0; i < 256; ++i)
                maxAbs = std::max (maxAbs, std::abs (buf.getSample (0, i)));

            expect (maxAbs > 0.0f, "Empty sample snapshot must activate sine fallback");
            engine.allSoundOff();
        }
    }
};

class SamplerEnginePlaybackModeTests : public juce::UnitTest
{
public:
    SamplerEnginePlaybackModeTests() : juce::UnitTest ("SamplerEngine_PlaybackModes", "AviatorKeyz") {}

    void runTest() override
    {
        TestSamples::init();

        auto estimateZeroCrossingHz = [] (const juce::AudioBuffer<float>& buffer, int numSamples) -> float
        {
            int crossings = 0;
            const float* data = buffer.getReadPointer (0);
            for (int i = 1; i < numSamples; ++i)
            {
                if ((data[i - 1] >= 0.f && data[i] < 0.f)
                    || (data[i - 1] < 0.f && data[i] >= 0.f))
                    ++crossings;
            }
            return static_cast<float> (crossings) * 0.5f * 44100.f / static_cast<float> (numSamples);
        };

        beginTest ("ChromaticResample: higher MIDI note raises output pitch");
        {
            SamplerEngine engine;
            juce::dsp::ProcessSpec spec { 44100.0, 512, 2 };
            engine.prepare (spec);
            TestSampleSnapshot snap;
            snap.setMono (TestSamples::sine4096, TestSamples::kFrames, 60);
            engine.setSampleSnapshot (&snap.snapshot);
            engine.setEnvelopeTimesMs (0.5f, 300.f, 1.f, 50.0f);

            SourceSettings settings;
            settings.playbackMode = SamplePlaybackMode::ChromaticResample;
            settings.keytrack = false;
            settings.bpmSync = false;
            engine.setSourceSettings (settings, 120.0);
            engine.setPlaybackContext (AviatorKeyz::SoundType::OneShot, AviatorKeyz::Category::LEADS);

            juce::AudioBuffer<float> buf (2, 512);

            engine.noteOn (60, 1.0f, false, 0.0f);
            buf.clear(); engine.process (buf);
            const float rootHz = estimateZeroCrossingHz (buf, buf.getNumSamples());
            engine.allSoundOff();

            engine.noteOn (72, 1.0f, false, 0.0f);
            buf.clear(); engine.process (buf);
            const float highHz = estimateZeroCrossingHz (buf, buf.getNumSamples());
            engine.allSoundOff();

            expect (highHz > rootHz * 1.8f,
                    "Chromatic mode must transpose pitch with MIDI note");
        }

        beginTest ("PhraseOriginal: MIDI octave does not change output pitch");
        {
            SamplerEngine engine;
            juce::dsp::ProcessSpec spec { 44100.0, 512, 2 };
            engine.prepare (spec);
            TestSampleSnapshot snap;
            snap.setMono (TestSamples::sine4096, TestSamples::kFrames, 60);
            engine.setSampleSnapshot (&snap.snapshot);
            engine.setEnvelopeTimesMs (0.5f, 300.f, 1.f, 50.0f);

            SourceSettings settings;
            settings.playbackMode = SamplePlaybackMode::PhraseOriginal;
            settings.keytrack = false;
            settings.bpmSync = false;
            engine.setSourceSettings (settings, 120.0);
            engine.setPlaybackContext (AviatorKeyz::SoundType::Phrase, AviatorKeyz::Category::ARPS);

            juce::AudioBuffer<float> buf (2, 512);

            engine.noteOn (60, 1.0f, false, 0.0f);
            buf.clear(); engine.process (buf);
            const float rootHz = estimateZeroCrossingHz (buf, buf.getNumSamples());
            engine.allSoundOff();

            engine.noteOn (72, 1.0f, false, 0.0f);
            buf.clear(); engine.process (buf);
            const float highHz = estimateZeroCrossingHz (buf, buf.getNumSamples());
            engine.allSoundOff();

            expect (std::abs (highHz - rootHz) < rootHz * 0.15f,
                    "Phrase mode must ignore MIDI pitch (root="
                    + juce::String (rootHz, 1)
                    + " Hz, high="
                    + juce::String (highHz, 1)
                    + " Hz)");
        }
    }
};

// Register
static SamplerEnginePitchTests        samplerPitchTests;
static SamplerEngineVoiceTests        samplerVoiceTests;
static SamplerEngineReverseTests      samplerReverseTests;
static SamplerEngineGlideTests        samplerGlideTests;
static SamplerEngineSineFallbackTests samplerSineTests;
static SamplerEnginePlaybackModeTests samplerPlaybackModeTests;
