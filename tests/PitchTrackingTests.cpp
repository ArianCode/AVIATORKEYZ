// =============================================================================
//  Pitch tracking / root-note / keytrack regression tests
// =============================================================================

#include <juce_core/juce_core.h>
#include <juce_dsp/juce_dsp.h>
#include <cmath>

#include "DSP/SamplerEngine.h"
#include "DSP/FastMath.h"
#include "State/CategorySoundPolicy.h"
#include "State/PresetManager.h"
#include "State/ParameterLayout.h"
#include "State/SampleLibrary.h"
#include "State/FactoryResources.h"

namespace
{
struct PitchTestProcessor : juce::AudioProcessor
{
    PitchTestProcessor()
        : AudioProcessor (BusesProperties()),
          apvts (*this, nullptr, "AviatorKeyzState", AviatorKeyz::createParameterLayout())
    {}

    const juce::String getName() const override { return "PitchTrackingTest"; }
    bool acceptsMidi() const override { return true; }
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

struct TestSnap
{
    SampleLibrary::AudioSnapshot snapshot;
    SampleLibrary::AudioRegion region;
    std::vector<float> buf;

    void setMono (const float* data, int frames, int root, double fileRate = 44100.0)
    {
        buf.assign (data, data + frames);
        region = {};
        region.data = buf.data();
        region.numFrames = frames;
        region.rootNote = root;
        region.noteMin = 0;
        region.noteMax = 127;
        region.velocityMin = 0.0f;
        region.velocityMax = 1.0f;
        region.fileSampleRate = fileRate;
        snapshot.regions.clear();
        snapshot.regions.push_back (region);
    }
};

std::vector<float> makeSine (int frames, float hz, double sr)
{
    std::vector<float> out (static_cast<size_t> (frames));
    for (int i = 0; i < frames; ++i)
        out[static_cast<size_t> (i)] = static_cast<float> (
            std::sin (2.0 * juce::MathConstants<double>::pi
                      * static_cast<double> (hz)
                      * static_cast<double> (i) / sr));
    return out;
}
} // namespace

class PitchTrackingTests : public juce::UnitTest
{
public:
    PitchTrackingTests() : juce::UnitTest ("PitchTracking", "AviatorKeyz") {}

    void runTest() override
    {
        beginTest ("Root 60 / note 60 → offset 0, ratio 1.0");
        {
            expectWithinAbsoluteError (AviatorFastMath::semitoneRatio (0.f), 1.f, 1.0e-5f);
            const auto diag = renderDiag (60, 60, true, SamplePlaybackMode::ChromaticResample, 44100.0, 44100.0);
            expectEquals (diag.rootNote, 60);
            expectWithinAbsoluteError (diag.semitoneOffset, 0.f, 1.0e-4f);
            expectWithinAbsoluteError (diag.pitchRatio, 1.f, 1.0e-4f);
            expectWithinAbsoluteError (diag.finalIncrement, 1.f, 1.0e-4f);
        }

        beginTest ("Root 60 / note 72 → +12, ratio 2.0");
        {
            expectWithinAbsoluteError (AviatorFastMath::semitoneRatio (12.f), 2.f, 1.0e-5f);
            const auto diag = renderDiag (72, 60, true, SamplePlaybackMode::ChromaticResample, 44100.0, 44100.0);
            expectWithinAbsoluteError (diag.semitoneOffset, 12.f, 1.0e-4f);
            expectWithinAbsoluteError (diag.pitchRatio, 2.f, 1.0e-4f);
            expectWithinAbsoluteError (diag.finalIncrement, 2.f, 1.0e-4f);
        }

        beginTest ("Root 60 / note 48 → -12, ratio 0.5");
        {
            expectWithinAbsoluteError (AviatorFastMath::semitoneRatio (-12.f), 0.5f, 1.0e-5f);
            const auto diag = renderDiag (48, 60, true, SamplePlaybackMode::ChromaticResample, 44100.0, 44100.0);
            expectWithinAbsoluteError (diag.semitoneOffset, -12.f, 1.0e-4f);
            expectWithinAbsoluteError (diag.pitchRatio, 0.5f, 1.0e-4f);
            expectWithinAbsoluteError (diag.finalIncrement, 0.5f, 1.0e-4f);
        }

        beginTest ("Root 60 / note 67 → +7, ratio ≈ 1.498307");
        {
            const float expected = AviatorFastMath::semitoneRatio (7.f);
            expectWithinAbsoluteError (expected, 1.498307f, 1.0e-4f);
            const auto diag = renderDiag (67, 60, true, SamplePlaybackMode::ChromaticResample, 44100.0, 44100.0);
            expectWithinAbsoluteError (diag.semitoneOffset, 7.f, 1.0e-4f);
            expectWithinAbsoluteError (diag.pitchRatio, expected, 1.0e-4f);
        }

        beginTest ("Missing / fallback root stays deterministic (60), never equals incoming note");
        {
            // Engine falls back to root 60 when no region is loaded (sine path).
            SamplerEngine engine;
            juce::dsp::ProcessSpec spec { 44100.0, 512, 2 };
            engine.prepare (spec);
            engine.setSampleSnapshot (nullptr);
            engine.setEnvelopeTimesMs (0.5f, 0.f, 1.f, 50.f);

            SourceSettings settings;
            settings.keytrack = true;
            settings.playbackMode = SamplePlaybackMode::ChromaticResample;
            settings.bpmSync = false;
            engine.setSourceSettings (settings, 120.0);

            engine.noteOn (72, 1.f, false, 0.f);
            const auto diag = engine.getLastNotePitchDiag();
            expectEquals (diag.rootNote, 60);
            expect (diag.rootNote != diag.midiNote,
                    "Fallback root must not equal incoming MIDI note");
            expectEquals (diag.midiNote, 72);
            engine.allSoundOff();
        }

        beginTest ("Fixed-pitch (keytrack off): ratio stays 1.0 across MIDI notes");
        {
            const auto a = renderDiag (48, 60, false, SamplePlaybackMode::PhraseOriginal, 44100.0, 44100.0);
            const auto b = renderDiag (72, 60, false, SamplePlaybackMode::PhraseOriginal, 44100.0, 44100.0);
            expectWithinAbsoluteError (a.pitchRatio, 1.f, 1.0e-4f);
            expectWithinAbsoluteError (b.pitchRatio, 1.f, 1.0e-4f);
            expectWithinAbsoluteError (a.finalIncrement, b.finalIncrement, 1.0e-4f);
        }

        beginTest ("Key-tracked sound: ratio changes with MIDI notes");
        {
            const auto low = renderDiag (48, 60, true, SamplePlaybackMode::PhraseOriginal, 44100.0, 44100.0);
            const auto high = renderDiag (72, 60, true, SamplePlaybackMode::PhraseOriginal, 44100.0, 44100.0);
            expect (high.pitchRatio > low.pitchRatio * 1.5f);
            expectWithinAbsoluteError (high.pitchRatio / low.pitchRatio, 4.f, 1.0e-3f);
        }

        beginTest ("Keytrack toggle on ChromaticResample locks / unlocks pitch");
        {
            const auto on = renderDiag (72, 60, true, SamplePlaybackMode::ChromaticResample, 44100.0, 44100.0);
            const auto off = renderDiag (72, 60, false, SamplePlaybackMode::ChromaticResample, 44100.0, 44100.0);
            expectWithinAbsoluteError (on.pitchRatio, 2.f, 1.0e-4f);
            expectWithinAbsoluteError (off.pitchRatio, 1.f, 1.0e-4f);
        }

        beginTest ("Voice reuse recalculates increment per note");
        {
            SamplerEngine engine;
            TestSnap snap;
            auto sine = makeSine (4096, 261.63f, 44100.0);
            snap.setMono (sine.data(), 4096, 60);

            juce::dsp::ProcessSpec spec { 44100.0, 512, 2 };
            engine.prepare (spec);
            engine.setSampleSnapshot (&snap.snapshot);
            engine.setEnvelopeTimesMs (0.5f, 0.f, 1.f, 50.f);
            engine.setPolyphony (1);

            SourceSettings settings;
            settings.keytrack = true;
            settings.playbackMode = SamplePlaybackMode::ChromaticResample;
            settings.bpmSync = false;
            engine.setSourceSettings (settings, 120.0);

            engine.noteOn (60, 1.f, false, 0.f);
            const float inc60 = engine.getLastNotePitchDiag().finalIncrement;
            engine.noteOn (72, 1.f, false, 0.f); // steal / reuse
            const float inc72 = engine.getLastNotePitchDiag().finalIncrement;
            expectWithinAbsoluteError (inc60, 1.f, 1.0e-3f);
            expectWithinAbsoluteError (inc72, 2.f, 1.0e-3f);
            engine.allSoundOff();
        }

        beginTest ("44.1 kHz source at 48 kHz host combines rate conversion with MIDI transpose");
        {
            const auto diag = renderDiag (72, 60, true, SamplePlaybackMode::ChromaticResample, 44100.0, 48000.0);
            const float expectedSrc = 44100.f / 48000.f;
            expectWithinAbsoluteError (diag.sourceRateRatio, expectedSrc, 1.0e-5f);
            expectWithinAbsoluteError (diag.pitchRatio, 2.f, 1.0e-4f);
            expectWithinAbsoluteError (diag.finalIncrement, expectedSrc * 2.f, 1.0e-4f);
        }

        beginTest ("Factory Electric Guitar Fading loads as key-tracked Ensembles one-shot");
        {
            PitchTestProcessor proc;
            PresetManager pm (proc.apvts);
            SampleLibrary library;
            SamplerEngine engine;
            juce::dsp::ProcessSpec spec { 44100.0, 512, 2 };
            engine.prepare (spec);

            pm.onPresetLoaded = [&] (const juce::String&, const juce::String&,
                                     const juce::String& sampleId, int rootNote)
            {
                int numBytes = 0;
                const void* data = FactoryResources::tryGetEmbeddedWavData (sampleId, numBytes);
                expect (data != nullptr && numBytes > 0, "Embedded sample missing");
                if (data == nullptr)
                    return;
                library.clearAll();
                expect (library.loadFromMemory (data, static_cast<size_t> (numBytes), sampleId, rootNote));
                library.publish();
                engine.setSampleSnapshot (library.getPublishedSnapshot());
            };

            expect (pm.loadPreset ("Ensembles", "KMRBI_DPP_electric_guitar_fading_C"));
            expect (pm.getCurrentSoundType() == AviatorKeyz::SoundType::OneShot);

            if (auto* keyParam = proc.apvts.getParameter (AviatorKeyz::ParamID::SRC_KEYTRACK))
                expect (keyParam->getValue() > 0.5f, "Keytrack must be ON after loading EGF");

            if (auto* modeParam = dynamic_cast<juce::AudioParameterChoice*> (
                    proc.apvts.getParameter (AviatorKeyz::ParamID::SRC_PLAYBACK_MODE)))
                expectEquals (modeParam->getIndex(),
                              static_cast<int> (SamplePlaybackMode::ChromaticResample));

            SourceSettings settings;
            settings.keytrack = true;
            settings.playbackMode = SamplePlaybackMode::ChromaticResample;
            settings.bpmSync = false;
            engine.setSourceSettings (settings, 120.0);
            engine.setPlaybackContext (pm.getCurrentSoundType(), pm.getCurrentCategory());
            engine.setEnvelopeTimesMs (0.5f, 0.f, 1.f, 50.f);

            engine.noteOn (60, 1.f, false, 0.f);
            const auto c3 = engine.getLastNotePitchDiag();
            engine.allSoundOff();
            engine.noteOn (64, 1.f, false, 0.f);
            const auto e3 = engine.getLastNotePitchDiag();
            engine.allSoundOff();
            engine.noteOn (72, 1.f, false, 0.f);
            const auto c4 = engine.getLastNotePitchDiag();
            engine.allSoundOff();

            expectEquals (c3.rootNote, 60);
            expectWithinAbsoluteError (c3.pitchRatio, 1.f, 1.0e-3f);
            expectWithinAbsoluteError (e3.pitchRatio, AviatorFastMath::semitoneRatio (4.f), 1.0e-3f);
            expectWithinAbsoluteError (c4.pitchRatio, 2.f, 1.0e-3f);

            // Disable keytrack → fixed pitch
            settings.keytrack = false;
            engine.setSourceSettings (settings, 120.0);
            engine.noteOn (48, 1.f, false, 0.f);
            const auto fixedLow = engine.getLastNotePitchDiag();
            engine.allSoundOff();
            engine.noteOn (84, 1.f, false, 0.f);
            const auto fixedHigh = engine.getLastNotePitchDiag();
            engine.allSoundOff();
            expectWithinAbsoluteError (fixedLow.pitchRatio, 1.f, 1.0e-3f);
            expectWithinAbsoluteError (fixedHigh.pitchRatio, 1.f, 1.0e-3f);

            // Re-enable
            settings.keytrack = true;
            engine.setSourceSettings (settings, 120.0);
            engine.noteOn (72, 1.f, false, 0.f);
            expectWithinAbsoluteError (engine.getLastNotePitchDiag().pitchRatio, 2.f, 1.0e-3f);
            engine.allSoundOff();
        }

        beginTest ("Preset switch restores keytrack: phrase then chromatic");
        {
            PitchTestProcessor proc;
            PresetManager pm (proc.apvts);

            expect (pm.loadPreset ("Ensembles", "KSHMR_sok5_135_string_violin_highland_Cm")
                    || pm.loadPreset ("Arps", "OS_FF_Cm_Arp_Keys"));
            // Phrase categories force keytrack off via policy.
            if (auto* keyParam = proc.apvts.getParameter (AviatorKeyz::ParamID::SRC_KEYTRACK))
                expect (keyParam->getValue() < 0.5f, "Phrase preset must load with keytrack OFF");

            expect (pm.loadPreset ("Brass", "CSV_brass_happy_stab_held_C"));
            if (auto* keyParam = proc.apvts.getParameter (AviatorKeyz::ParamID::SRC_KEYTRACK))
                expect (keyParam->getValue() > 0.5f, "Chromatic Brass must load with keytrack ON");

            expect (pm.loadPreset ("Ensembles", "KMRBI_DPP_electric_guitar_fading_C"));
            if (auto* keyParam = proc.apvts.getParameter (AviatorKeyz::ParamID::SRC_KEYTRACK))
                expect (keyParam->getValue() > 0.5f, "EGF must load with keytrack ON after phrase preset");
        }
    }

private:
    SamplerEngine::NotePitchDiag renderDiag (int midiNote,
                                             int rootNote,
                                             bool keytrack,
                                             SamplePlaybackMode mode,
                                             double fileRate,
                                             double hostRate)
    {
        SamplerEngine engine;
        TestSnap snap;
        auto sine = makeSine (4096, 261.63f, fileRate);
        snap.setMono (sine.data(), 4096, rootNote, fileRate);

        juce::dsp::ProcessSpec spec { hostRate, 512, 2 };
        engine.prepare (spec);
        engine.setSampleSnapshot (&snap.snapshot);
        engine.setEnvelopeTimesMs (0.5f, 0.f, 1.f, 50.f);

        SourceSettings settings;
        settings.keytrack = keytrack;
        settings.playbackMode = mode;
        settings.bpmSync = false;
        settings.speed = 1.f;
        engine.setSourceSettings (settings, 120.0);
        engine.setPlaybackContext (keytrack ? AviatorKeyz::SoundType::OneShot
                                            : AviatorKeyz::SoundType::Phrase,
                                   keytrack ? AviatorKeyz::Category::ENSEMBLES
                                            : AviatorKeyz::Category::ARPS);

        engine.noteOn (midiNote, 1.f, false, 0.f);
        const auto diag = engine.getLastNotePitchDiag();
        engine.allSoundOff();
        return diag;
    }
};

static PitchTrackingTests pitchTrackingTests;
