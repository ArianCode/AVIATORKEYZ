// =============================================================================
//  HostStateRoundtripTests — project save/reopen and prepare/release lifecycle
//
//  These tests drive the real AviatorKeyzProcessor (linked into the test target
//  with AVIATORKEYZ_HEADLESS_TESTS=1) so they exercise the shipped state path,
//  not a copy of it.
//
//  Regression covered: FL Studio reopening a .flp (and starting an offline
//  render) calls setStateInformation -> releaseResources -> prepareToPlay.
//  releaseResources() nulls the sampler snapshot; prepareToPlay used to skip the
//  reload because the restored sampleId already matched loadedSampleId. The
//  instance came back with its preset name and parameters intact but with no
//  sample attached, so SamplerEngine::startVoice took its sine-fallback branch —
//  audible output, wrong sound.
//
//  A peak check alone does NOT catch this (the sine fallback is loud). The
//  load-bearing assertion is hasSamplerSampleForTest(): the sine fallback is
//  reachable only when the snapshot is missing.
// =============================================================================

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

#include "PluginProcessor.h"
#include "State/PresetManager.h"
#include "State/StateSchema.h"

namespace
{
constexpr double kSampleRate   = 48000.0;
constexpr int    kBlockSize    = 512;
constexpr int    kTestNote     = 60;
constexpr float  kSilenceFloor = 1.0e-4f;

/** Renders a note-on through the full processor and returns the output peak. */
float renderNotePeak (AviatorKeyzProcessor& proc, int midiNote = kTestNote, int numBlocks = 8)
{
    juce::AudioBuffer<float> buffer (2, kBlockSize);
    float peak = 0.f;

    for (int block = 0; block < numBlocks; ++block)
    {
        juce::MidiBuffer midi;
        if (block == 0)
            midi.addEvent (juce::MidiMessage::noteOn (1, midiNote, (juce::uint8) 100), 0);

        buffer.clear();
        proc.processBlock (buffer, midi);

        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            peak = juce::jmax (peak, buffer.getMagnitude (ch, 0, buffer.getNumSamples()));
    }

    return peak;
}

/** The first Leads preset that actually resolves to an embedded sample. */
bool loadAnyPreset (AviatorKeyzProcessor& proc)
{
    auto& pm = proc.getPresetManager();
    for (const auto& name : pm.getPresetsForCategory (AviatorKeyz::Category::LEADS))
        if (pm.loadPreset (AviatorKeyz::Category::LEADS, name)
            && pm.getCurrentSampleId().isNotEmpty())
            return true;
    return false;
}
} // namespace

class HostStateRoundtripTests : public juce::UnitTest
{
public:
    HostStateRoundtripTests() : juce::UnitTest ("HostStateRoundtrip", "AviatorKeyz") {}

    void runTest() override
    {
        beginTest ("Saved host state restores preset identity and the sample itself");
        {
            AviatorKeyzProcessor source;
            expect (loadAnyPreset (source), "Test needs a factory preset with an embedded sample");

            const auto savedName     = source.getPresetManager().getCurrentPresetName();
            const auto savedSampleId = source.getPresetManager().getCurrentSampleId();

            juce::MemoryBlock saved;
            source.getStateInformation (saved);
            expect (saved.getSize() > 0, "getStateInformation produced no data");

            // A fresh instance, as the host creates on project reopen.
            AviatorKeyzProcessor restored;
            restored.setStateInformation (saved.getData(), (int) saved.getSize());

            expectEquals (restored.getPresetManager().getCurrentPresetName(), savedName);
            expectEquals (restored.getPresetManager().getCurrentSampleId(), savedSampleId);

            restored.prepareToPlay (kSampleRate, kBlockSize);
            expect (restored.hasSamplerSampleForTest(),
                    "Restored instance has no sample attached — voices would sine-fall-back");
            expect (renderNotePeak (restored) > kSilenceFloor, "Restored instance rendered silence");
        }

        beginTest ("FL reopen order: setState -> releaseResources -> prepareToPlay keeps the sample");
        {
            AviatorKeyzProcessor source;
            expect (loadAnyPreset (source));

            juce::MemoryBlock saved;
            source.getStateInformation (saved);

            AviatorKeyzProcessor restored;
            restored.prepareToPlay (kSampleRate, kBlockSize);
            restored.setStateInformation (saved.getData(), (int) saved.getSize());
            restored.releaseResources();
            restored.prepareToPlay (kSampleRate, kBlockSize);

            expect (restored.hasSamplerSampleForTest(),
                    "Reopened instance lost its sample across release/prepare — sine fallback");
            expect (renderNotePeak (restored) > kSilenceFloor, "Reopened instance rendered silence");
        }

        beginTest ("Repeated release/prepare cycles (offline render) keep the sample");
        {
            AviatorKeyzProcessor proc;
            expect (loadAnyPreset (proc));

            proc.prepareToPlay (kSampleRate, kBlockSize);
            expect (proc.hasSamplerSampleForTest(), "Baseline load left no sample attached");
            expect (renderNotePeak (proc) > kSilenceFloor, "Baseline render was silent");

            for (int cycle = 0; cycle < 3; ++cycle)
            {
                proc.releaseResources();
                expect (! proc.hasSamplerSampleForTest(),
                        "releaseResources is expected to detach the snapshot");

                proc.prepareToPlay (kSampleRate, kBlockSize);
                expect (proc.hasSamplerSampleForTest(),
                        "Cycle " + juce::String (cycle) + " lost the sample — sine fallback");
                expect (renderNotePeak (proc) > kSilenceFloor,
                        "Cycle " + juce::String (cycle) + " rendered silence");
            }
        }

        beginTest ("Zero amp sustain is a valid patch, not a failed load");
        {
            AviatorKeyzProcessor proc;
            expect (loadAnyPreset (proc));

            if (auto* sustain = proc.getAPVTS().getParameter (AviatorKeyz::ParamID::ENV_AMP_SUSTAIN))
                sustain->setValueNotifyingHost (0.f);

            proc.prepareToPlay (kSampleRate, kBlockSize);

            // Before the split, validateCurrentState() failed on sustain == 0, so this
            // reported failure and prepareToPlay re-decoded the sample every cycle.
            const auto sampleId = proc.getPresetManager().getCurrentSampleId();
            expect (proc.loadFactorySample (sampleId, proc.getPresetManager().getCurrentRootNote()),
                    "Reloading the active sample with sustain=0 must still report success");
            expect (proc.hasSamplerSampleForTest());
        }
    }
};

static HostStateRoundtripTests hostStateRoundtripTests;
