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

/** Loads the nth preset of a category that resolves to an embedded sample. */
bool loadPresetWithSample (AviatorKeyzProcessor& proc,
                           const juce::String& category,
                           int skip = 0)
{
    auto& pm = proc.getPresetManager();
    for (const auto& name : pm.getPresetsForCategory (category))
    {
        if (pm.loadPreset (category, name) && pm.getCurrentSampleId().isNotEmpty())
        {
            if (skip-- <= 0)
                return true;
        }
    }
    return false;
}

/** The first Leads preset that actually resolves to an embedded sample. */
bool loadAnyPreset (AviatorKeyzProcessor& proc)
{
    return loadPresetWithSample (proc, AviatorKeyz::Category::LEADS);
}

/** Saves host state and restores it into a fresh instance taken through the
    host's prepare cycle — the shape every one of these tests needs. */
void saveAndRestore (AviatorKeyzProcessor& from, AviatorKeyzProcessor& into)
{
    juce::MemoryBlock saved;
    from.getStateInformation (saved);
    into.setStateInformation (saved.getData(), (int) saved.getSize());
    into.prepareToPlay (kSampleRate, kBlockSize);
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

        beginTest ("Two instances keep independent presets across save/restore");
        {
            // A host with several channels saves one state blob per instance. If any
            // sample or preset state were shared (statics, a singleton library), the
            // second restore would overwrite the first.
            AviatorKeyzProcessor a, b;
            expect (loadPresetWithSample (a, AviatorKeyz::Category::LEADS, 0));
            expect (loadPresetWithSample (b, AviatorKeyz::Category::LEADS, 1));

            const auto idA = a.getPresetManager().getCurrentSampleId();
            const auto idB = b.getPresetManager().getCurrentSampleId();
            expect (idA != idB, "Test needs two presets with different samples");

            AviatorKeyzProcessor restoredA, restoredB;
            saveAndRestore (a, restoredA);
            saveAndRestore (b, restoredB);

            expectEquals (restoredA.getPresetManager().getCurrentSampleId(), idA);
            expectEquals (restoredB.getPresetManager().getCurrentSampleId(), idB);
            expect (restoredA.hasSamplerSampleForTest(), "Instance A lost its sample");
            expect (restoredB.hasSamplerSampleForTest(), "Instance B lost its sample");

            // Restoring B must not have disturbed A.
            expectEquals (restoredA.getPresetManager().getCurrentSampleId(), idA);
            expect (renderNotePeak (restoredA) > kSilenceFloor, "Instance A rendered silence");
            expect (renderNotePeak (restoredB) > kSilenceFloor, "Instance B rendered silence");
        }

        beginTest ("Live instances stay independent through a shared release/prepare cycle");
        {
            // Bounce/consolidate takes every instance through release/prepare together.
            AviatorKeyzProcessor a, b;
            expect (loadPresetWithSample (a, AviatorKeyz::Category::LEADS, 0));
            expect (loadPresetWithSample (b, AviatorKeyz::Category::LEADS, 1));

            const auto idA = a.getPresetManager().getCurrentSampleId();
            const auto idB = b.getPresetManager().getCurrentSampleId();

            a.prepareToPlay (kSampleRate, kBlockSize);
            b.prepareToPlay (kSampleRate, kBlockSize);
            a.releaseResources();
            b.releaseResources();
            a.prepareToPlay (kSampleRate, kBlockSize);
            b.prepareToPlay (kSampleRate, kBlockSize);

            expect (a.hasSamplerSampleForTest(), "Instance A lost its sample on bounce");
            expect (b.hasSamplerSampleForTest(), "Instance B lost its sample on bounce");
            expectEquals (a.getPresetManager().getCurrentSampleId(), idA);
            expectEquals (b.getPresetManager().getCurrentSampleId(), idB);
        }

        beginTest ("Restore survives a host that prepares at a different rate and block size");
        {
            // Offline render frequently runs at a different sample rate or block size
            // than the live session, which is a second prepareToPlay on the same state.
            AviatorKeyzProcessor source;
            expect (loadAnyPreset (source));
            source.prepareToPlay (44100.0, 128);

            AviatorKeyzProcessor restored;
            saveAndRestore (source, restored);
            expect (restored.hasSamplerSampleForTest());

            restored.releaseResources();
            restored.prepareToPlay (96000.0, 1024);
            expect (restored.hasSamplerSampleForTest(),
                    "Sample lost when the host re-prepared at a different rate/block size");
            expect (renderNotePeak (restored) > kSilenceFloor, "Rendered silence at 96k/1024");
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
