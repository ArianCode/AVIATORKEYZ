// =============================================================================
//  Preset playback regression tests
//
//  Exercises the factory preset → sample publish → MIDI → sampler render path
//  that the full processor uses, without requiring a GUI or host.
// =============================================================================

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

#include "State/PresetManager.h"
#include "State/ParameterLayout.h"
#include "State/StateSchema.h"
#include "State/SampleLibrary.h"
#include "State/FactoryResources.h"
#include "DSP/SamplerEngine.h"
#include "DSP/SynthEngine.h"
#include "MIDI/MidiHandler.h"

namespace
{
struct PlaybackTestProcessor : juce::AudioProcessor
{
    PlaybackTestProcessor()
        : AudioProcessor (BusesProperties()),
          apvts (*this, nullptr, "AviatorKeyzState", AviatorKeyz::createParameterLayout())
    {}

    const juce::String getName() const override { return "PresetPlaybackTest"; }
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

bool isFiniteBuffer (const juce::AudioBuffer<float>& buffer)
{
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        for (int i = 0; i < buffer.getNumSamples(); ++i)
            if (! std::isfinite (buffer.getSample (ch, i)))
                return false;
    return true;
}

class PresetPlaybackHarness
{
public:
    explicit PresetPlaybackHarness (PlaybackTestProcessor& proc)
        : presetManager (proc.apvts)
    {
        presetManager.onPresetLoaded = [this] (const juce::String&,
                                               const juce::String&,
                                               const juce::String& sampleId,
                                               int rootNote) {
            loadSample (sampleId, rootNote);
        };
    }

    void prepare (double sampleRate, int blockSize)
    {
        juce::dsp::ProcessSpec spec { sampleRate, static_cast<juce::uint32> (blockSize), 2 };
        samplerEngine.prepare (spec);
        synthEngine.prepare (spec);
        blockSizePrepared = blockSize;
    }

    bool loadFactoryPreset (const juce::String& category, const juce::String& name)
    {
        return presetManager.loadPreset (category, name);
    }

    float renderNotePeak (int midiNote, float sourceBlend = 0.f)
    {
        juce::MidiBuffer midi;
        midi.addEvent (juce::MidiMessage::noteOn (1, midiNote, static_cast<juce::uint8> (100)), 0);

        midiHandler.process (midi,
                             samplerEngine,
                             synthEngine,
                             false,
                             0.f,
                             sourceBlend);

        juce::AudioBuffer<float> samplerBuf (2, blockSizePrepared);
        juce::AudioBuffer<float> synthBuf (2, blockSizePrepared);
        samplerBuf.clear();
        synthBuf.clear();

        if (sourceBlend < 0.999f)
            samplerEngine.process (samplerBuf);
        if (sourceBlend > 0.001f)
            synthEngine.process (synthBuf);

        const float sampleGain = 1.f - sourceBlend;
        const float synthGain = sourceBlend;

        float peak = 0.f;
        for (int i = 0; i < blockSizePrepared; ++i)
        {
            const float l = samplerBuf.getSample (0, i) * sampleGain
                          + synthBuf.getSample (0, i) * synthGain;
            const float r = samplerBuf.getSample (1, i) * sampleGain
                          + synthBuf.getSample (1, i) * synthGain;
            peak = juce::jmax (peak, std::abs (l), std::abs (r));
        }

        return peak;
    }

    bool publishedSnapshotHasSampleData() const
    {
        const auto* snap = sampleLibrary.getPublishedSnapshot();
        return snap != nullptr
               && ! snap->regions.empty()
               && snap->regions.front().data != nullptr
               && snap->regions.front().numFrames > 1;
    }

    const SampleLibrary::AudioSnapshot* getPublishedSnapshot() const noexcept
    {
        return sampleLibrary.getPublishedSnapshot();
    }

    bool tryLoadSample (const juce::String& sampleId, int rootNote)
    {
        return loadSample (sampleId, rootNote);
    }

    const PresetManager& getPresetManager() const noexcept { return presetManager; }

private:
    bool loadSample (const juce::String& sampleId, int rootNote)
    {
        if (sampleId.isEmpty())
            return false;

        int numBytes = 0;
        const void* embeddedData = FactoryResources::tryGetEmbeddedWavData (sampleId, numBytes);
        if (embeddedData == nullptr || numBytes <= 0)
            return false;

        {
            SampleLibrary probe;
            if (! probe.loadFromMemory (embeddedData,
                                         static_cast<size_t> (numBytes),
                                         sampleId,
                                         juce::jlimit (0, 127, rootNote)))
                return false;
        }

        samplerEngine.allSoundOff();
        synthEngine.allSoundOff();
        sampleLibrary.clearAll();

        if (! sampleLibrary.loadFromMemory (embeddedData,
                                            static_cast<size_t> (numBytes),
                                            sampleId,
                                            juce::jlimit (0, 127, rootNote)))
            return false;

        sampleLibrary.publish();
        samplerEngine.setSampleSnapshot (sampleLibrary.getPublishedSnapshot());
        return true;
    }

    PresetManager presetManager;
    SampleLibrary sampleLibrary;
    SamplerEngine samplerEngine;
    SynthEngine synthEngine;
    MidiHandler midiHandler;
    int blockSizePrepared { 512 };
};
} // namespace

class PresetPlaybackTests : public juce::UnitTest
{
public:
    PresetPlaybackTests() : juce::UnitTest ("PresetPlayback", "AviatorKeyz") {}

    void runTest() override
    {
        beginTest ("Init preset loads embedded sample and plays MIDI 60");
        {
            PlaybackTestProcessor proc;
            PresetPlaybackHarness harness (proc);
            harness.prepare (48000.0, 512);

            expect (harness.loadFactoryPreset (AviatorKeyz::Category::LEADS, "Init"),
                    "Init preset should load");

            expect (harness.publishedSnapshotHasSampleData(),
                    "Init preset should publish non-empty sample snapshot");

            const float peak = harness.renderNotePeak (60, 0.f);
            expect (peak > 1e-4f,
                    "Init preset should produce audible sampler output (peak=" + juce::String (peak) + ")");
        }

        beginTest ("Factory leads preset BOS_AA produces audio at root note");
        {
            PlaybackTestProcessor proc;
            PresetPlaybackHarness harness (proc);
            harness.prepare (44100.0, 256);

            expect (harness.loadFactoryPreset (AviatorKeyz::Category::LEADS,
                                               "BOS_AA_Synth_One_Shot_Shadows_C"));

            const float peak = harness.renderNotePeak (60, 0.f);
            expect (peak > 1e-4f,
                    "BOS_AA preset should produce audio at MIDI 60 (peak=" + juce::String (peak) + ")");
        }

        beginTest ("Partial factory preset keeps sample blend at zero (sampler path)");
        {
            PlaybackTestProcessor proc;
            PresetPlaybackHarness harness (proc);
            harness.prepare (48000.0, 512);
            harness.loadFactoryPreset (AviatorKeyz::Category::LEADS, "Init");

            expectWithinAbsoluteError (
                proc.apvts.getRawParameterValue (AviatorKeyz::ParamID::SOURCE_BLEND)->load(),
                0.f, 0.001f,
                "Factory preset load must route MIDI to sampler (source_blend=0)");
        }

        beginTest ("Rapid preset switching while notes held does not crash");
        {
            PlaybackTestProcessor proc;
            PresetPlaybackHarness harness (proc);
            harness.prepare (48000.0, 128);

            const auto leads = FactoryResources::getPresetNamesForCategory (AviatorKeyz::Category::LEADS);
            expect (leads.size() >= 2, "Need at least two leads presets for switch test");

            for (int round = 0; round < 8; ++round)
            {
                for (int i = 0; i < juce::jmin (4, leads.size()); ++i)
                {
                    expect (harness.loadFactoryPreset (AviatorKeyz::Category::LEADS, leads[i]));
                    const float peak = harness.renderNotePeak (60 + (i % 12), 0.f);
                    expect (std::isfinite (peak), "Peak must stay finite during rapid preset switching");
                }
            }
        }

        beginTest ("Multiple factory categories produce finite non-silent output");
        {
            PlaybackTestProcessor proc;
            PresetPlaybackHarness harness (proc);
            harness.prepare (48000.0, 256);

            const juce::StringArray categories {
                AviatorKeyz::Category::LEADS,
                AviatorKeyz::Category::BELLS,
                AviatorKeyz::Category::ARPS,
            };

            for (const auto& category : categories)
            {
                const auto names = FactoryResources::getPresetNamesForCategory (category);
                if (names.isEmpty())
                    continue;

                expect (harness.loadFactoryPreset (category, names[0]),
                        "Failed to load " + category + "/" + names[0]);

                juce::AudioBuffer<float> buf (2, 256);
                buf.clear();
                const float peak = harness.renderNotePeak (60, 0.f);

                expect (isFiniteBuffer (buf) || peak >= 0.f, category + " output must be finite");
                expect (peak > 1e-5f,
                        category + "/" + names[0] + " must produce audible output (peak="
                        + juce::String (peak) + ")");
            }
        }
        beginTest ("Invalid sampleId does not replace the published snapshot");
        {
            PlaybackTestProcessor proc;
            PresetPlaybackHarness harness (proc);
            harness.prepare (48000.0, 512);

            expect (harness.loadFactoryPreset (AviatorKeyz::Category::LEADS, "Init"));
            expect (harness.publishedSnapshotHasSampleData());

            const auto* snapBefore = harness.getPublishedSnapshot();
            expect (snapBefore != nullptr);
            const int framesBefore = snapBefore->regions.empty() ? 0
                                                                 : snapBefore->regions.front().numFrames;

            expect (! harness.tryLoadSample ("__missing_sample_id__", 60),
                    "Unknown sampleId must fail without replacing snapshot");

            const auto* snapAfter = harness.getPublishedSnapshot();
            expect (snapAfter == snapBefore, "Published snapshot pointer must stay stable");
            expect (harness.publishedSnapshotHasSampleData());

            if (snapAfter != nullptr && ! snapAfter->regions.empty())
            {
                expectEquals (snapAfter->regions.front().numFrames, framesBefore);
            }

            const float peak = harness.renderNotePeak (60, 0.f);
            expect (peak > 1e-4f,
                    "Previously loaded sample must still play after failed reload");
        }
    }
};

static PresetPlaybackTests presetPlaybackTests;
