// =============================================================================
//  Pitch alignment — engine root mapping and cross-instance parity
// =============================================================================

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

#include "DSP/SamplerEngine.h"
#include "DSP/PitchProbe.h"
#include "State/SampleLibrary.h"

namespace
{
constexpr int kTestFrames = 4096;
constexpr double kTestSr = 44100.0;

void fillSineAtMidi (float* dst, int frames, float midiNote) noexcept
{
    const float hz = 440.f * std::pow (2.f, (midiNote - 69.f) / 12.f);
    for (int i = 0; i < frames; ++i)
        dst[i] = std::sin (juce::MathConstants<float>::twoPi * hz
                           * static_cast<float> (i) / static_cast<float> (kTestSr));
}

float peakFrequencyHz (const float* mono, int numSamples, int targetMidi = 69) noexcept
{
    const auto probe = PitchProbe::analyzeMono (mono, numSamples, kTestSr, targetMidi);
    return 440.f * std::pow (2.f, (probe.detectedMidi - 69.f) / 12.f);
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
} // namespace

class PitchAlignmentEngineTests : public juce::UnitTest
{
public:
    PitchAlignmentEngineTests() : juce::UnitTest ("PitchAlignment_Engine", "AviatorKeyz") {}

    void runTest() override
    {
        static float sineA4[kTestFrames] = {};
        static bool init = false;
        if (! init)
        {
            init = true;
            fillSineAtMidi (sineA4, kTestFrames, 69.f);
        }

        beginTest ("rootNote 69 + MIDI 69 preserves A4 fundamental");
        {
            const float hz = peakFrequencyHz (sineA4, kTestFrames, 69);
            expectWithinAbsoluteError (hz, 440.f, 8.f, "Synthetic buffer should measure ~440 Hz");

            SamplerEngine engine;
            juce::dsp::ProcessSpec spec { kTestSr, 512, 2 };
            engine.prepare (spec);
            TestSampleSnapshot snap;
            snap.setMono (sineA4, kTestFrames, 69);
            engine.setSampleSnapshot (&snap.snapshot);
            engine.setEnvelopeTimesMs (0.5f, 300.f, 1.f, 50.f);

            SourceSettings settings;
            settings.playbackMode = SamplePlaybackMode::ChromaticResample;
            settings.rootNote = 60; // stale APVTS default — must be ignored
            settings.bpmSync = false;
            engine.setSourceSettings (settings, 120.0);
            engine.setPlaybackContext (AviatorKeyz::SoundType::OneShot, AviatorKeyz::Category::LEADS);

            engine.noteOn (69, 1.f, false, 0.f);

            juce::AudioBuffer<float> buf (2, 512);
            buf.clear();
            engine.process (buf);

            juce::HeapBlock<float> mono;
            mono.malloc (512);
            for (int i = 0; i < 512; ++i)
                mono[i] = buf.getSample (0, i);

            const float outHz = peakFrequencyHz (mono.getData(), 512, 69);
            expectWithinAbsoluteError (outHz, 440.f, 25.f,
                                       "Played A4 with sample root 69 must output ~440 Hz even if settings.rootNote is 60");
            engine.allSoundOff();
        }

        beginTest ("rootNote 60 + MIDI 60 preserves A4 when buffer is A4");
        {
            SamplerEngine engine;
            juce::dsp::ProcessSpec spec { kTestSr, 512, 2 };
            engine.prepare (spec);
            TestSampleSnapshot snap;
            snap.setMono (sineA4, kTestFrames, 60);
            engine.setSampleSnapshot (&snap.snapshot);
            engine.setEnvelopeTimesMs (0.5f, 300.f, 1.f, 50.f);

            SourceSettings settings;
            settings.playbackMode = SamplePlaybackMode::ChromaticResample;
            settings.rootNote = 72; // wrong on purpose
            settings.bpmSync = false;
            engine.setSourceSettings (settings, 120.0);
            engine.setPlaybackContext (AviatorKeyz::SoundType::OneShot, AviatorKeyz::Category::LEADS);

            engine.noteOn (60, 1.f, false, 0.f);

            juce::AudioBuffer<float> buf (2, 512);
            buf.clear();
            engine.process (buf);

            juce::HeapBlock<float> mono;
            mono.malloc (512);
            for (int i = 0; i < 512; ++i)
                mono[i] = buf.getSample (0, i);

            const float outHz = peakFrequencyHz (mono.getData(), 512, 69);
            expectWithinAbsoluteError (outHz, 440.f, 25.f,
                                       "MIDI 60 with A4 buffer and sample root 60 should still be ~440 Hz");
            engine.allSoundOff();
        }

        beginTest ("Sample root 72 + MIDI 72 is unity; MIDI 60 is one octave down");
        {
            SamplerEngine engine;
            juce::dsp::ProcessSpec spec { kTestSr, 2048, 2 };
            engine.prepare (spec);
            TestSampleSnapshot snap;
            snap.setMono (sineA4, kTestFrames, 72);
            engine.setSampleSnapshot (&snap.snapshot);
            engine.setEnvelopeTimesMs (0.5f, 300.f, 1.f, 50.f);

            SourceSettings settings;
            settings.playbackMode = SamplePlaybackMode::ChromaticResample;
            settings.rootNote = 60;
            settings.bpmSync = false;
            engine.setSourceSettings (settings, 120.0);
            engine.setPlaybackContext (AviatorKeyz::SoundType::OneShot, AviatorKeyz::Category::BELLS);

            juce::AudioBuffer<float> buf (2, 2048);

            engine.noteOn (72, 1.f, false, 0.f);
            buf.clear();
            engine.process (buf);
            const float unityInc = engine.getActiveVoiceReadIncrementForTest();
            engine.allSoundOff();

            engine.noteOn (60, 1.f, false, 0.f);
            buf.clear();
            engine.process (buf);
            const float lowInc = engine.getActiveVoiceReadIncrementForTest();
            engine.allSoundOff();

            expectWithinAbsoluteError (unityInc, 1.f, 0.02f, "MIDI at sample root must be ~unity rate");
            expect (lowInc < unityInc * 0.55f && lowInc > unityInc * 0.45f,
                    "MIDI one octave below sample root must half the read rate");
        }

        beginTest ("Two SamplerEngines with identical snapshot produce matching output");
        {
            SamplerEngine a;
            SamplerEngine b;
            juce::dsp::ProcessSpec spec { kTestSr, 256, 2 };
            a.prepare (spec);
            b.prepare (spec);
            TestSampleSnapshot snap;
            snap.setMono (sineA4, kTestFrames, 60);
            a.setSampleSnapshot (&snap.snapshot);
            b.setSampleSnapshot (&snap.snapshot);
            a.setEnvelopeTimesMs (0.5f, 300.f, 1.f, 50.f);
            b.setEnvelopeTimesMs (0.5f, 300.f, 1.f, 50.f);

            SourceSettings settings;
            settings.playbackMode = SamplePlaybackMode::ChromaticResample;
            settings.bpmSync = false;
            a.setSourceSettings (settings, 120.0);
            b.setSourceSettings (settings, 120.0);

            a.noteOn (60, 1.f, false, 0.f);
            b.noteOn (60, 1.f, false, 0.f);

            juce::AudioBuffer<float> bufA (2, 256);
            juce::AudioBuffer<float> bufB (2, 256);
            bufA.clear();
            bufB.clear();
            a.process (bufA);
            b.process (bufB);

            float maxDiff = 0.f;
            for (int i = 0; i < 256; ++i)
                maxDiff = std::max (maxDiff, std::abs (bufA.getSample (0, i) - bufB.getSample (0, i)));

            expect (maxDiff < 0.001f,
                    "Cross-instance render should match; max diff=" + juce::String (maxDiff));
            a.allSoundOff();
            b.allSoundOff();
        }
    }
};

class PitchAlignmentSampleLibraryTests : public juce::UnitTest
{
public:
    PitchAlignmentSampleLibraryTests()
        : juce::UnitTest ("PitchAlignment_SampleLibrary", "AviatorKeyz") {}

    void runTest() override
    {
        beginTest ("PitchProbe correctBufferToRoot runs on loaded buffer");
        {
            static float sineFlat[kTestFrames] = {};
            static bool init = false;
            if (! init)
            {
                init = true;
                fillSineAtMidi (sineFlat, kTestFrames, 48.f);
            }

            juce::AudioBuffer<float> buf (1, kTestFrames);
            buf.copyFrom (0, 0, sineFlat, kTestFrames);

            const auto before = PitchProbe::analyzeMono (buf.getReadPointer (0), kTestFrames, kTestSr);
            const int root = PitchProbe::correctBufferToRoot (buf, kTestSr, 60);
            const auto after = PitchProbe::analyzeMono (buf.getReadPointer (0), kTestFrames, kTestSr);

            expectEquals (root, 60, "Target root should remain 60");
            expect (before.confidence >= 0.f && after.confidence >= 0.f,
                    "Pitch probe should run without invalidating the buffer");
        }
    }
};

static PitchAlignmentEngineTests        pitchEngineTests;
static PitchAlignmentSampleLibraryTests pitchLibTests;
