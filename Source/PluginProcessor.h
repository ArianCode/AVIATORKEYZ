#pragma once

// Set by the unit-test target: links the processor without the GUI layer so the
// host state/lifecycle path can be tested directly.
#ifndef AVIATORKEYZ_HEADLESS_TESTS
 #define AVIATORKEYZ_HEADLESS_TESTS 0
#endif

#include <array>
#include <atomic>
#include <functional>
#include <vector>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

#include "DSP/SamplerEngine.h"
#include "DSP/SynthEngine.h"
#include "DSP/FilterProcessor.h"
#include "DSP/TextureEngine.h"
#include "DSP/OutputLimiter.h"
#include "DSP/Arpeggiator.h"
#include "DSP/Mfx/MfxRack.h"
#include "DSP/StretchPlayer.h"
#include "DSP/Performance/PerformanceTypes.h"
#include "DSP/Performance/PerformanceTexturePipeline.h"
#include "DSP/Performance/PerformanceApvtsReader.h"
#include "DSP/ToneShaper.h"
#include "DSP/SmearProcessor.h"
#include "DSP/ReverbTail.h"
#include "DSP/FxChain.h"
#include "MIDI/MidiHandler.h"
#include "State/StateSchema.h"
#include "State/PresetManager.h"
#include "State/SampleLibrary.h"

// =============================================================================
//  AviatorKeyzProcessor — root AudioProcessor
//
//  Voice path per block (sample-accurate segments between MIDI events):
//    host MIDI ─▶ [Arpeggiator] ─▶ MidiHandler ─▶ SamplerEngine (+ SynthEngine layer)
//               ─▶ FilterProcessor ─▶ input gain ─▶ TextureEngine layer
//               ─▶ PerformanceTexturePipeline (chop / motion / grain / perf FX)
//               ─▶ tone / brightness / macro HP / reverb / FX chain
//               ─▶ pan ─▶ limiter ─▶ output gain
// =============================================================================

class AviatorKeyzProcessor final : public juce::AudioProcessor
{
public:
    AviatorKeyzProcessor();
    ~AviatorKeyzProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const AudioProcessor::BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void processBlockBypassed (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
   #if AVIATORKEYZ_HEADLESS_TESTS
    bool hasEditor() const override { return false; }
    const juce::String getName() const override         { return "AviatorKeyz"; }
   #else
    bool hasEditor() const override { return true; }
    const juce::String getName() const override         { return JucePlugin_Name; }
   #endif
    bool   acceptsMidi() const override                 { return true; }
    bool   producesMidi() const override                { return false; }
    bool   isMidiEffect() const override                { return false; }
    double getTailLengthSeconds() const override        { return 4.0; }

    int  getNumPrograms() override                              { return 1; }
    int  getCurrentProgram() override                           { return 0; }
    void setCurrentProgram (int) override                       {}
    const juce::String getProgramName (int) override            { return {}; }
    void changeProgramName (int, const juce::String&) override  {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getAPVTS() noexcept { return apvts; }
    PresetManager&                       getPresetManager() noexcept { return *presetManager; }

    /** Message / UI thread: loaded waveform thumbnail (mono). May be null if empty. */
    const float* getFactoryWaveformData() const noexcept;
    int          getFactoryWaveformFrames() const noexcept { return factoryWaveformFrames; }

    /** Message thread: load embedded factory sample by id (from preset sampleId).
        Ids prefixed "user:" resolve to a CARGO HOLD file path. */
    bool loadFactorySample (const juce::String& sampleId, int rootNote = 60);

    // --- CARGO HOLD: user sample import (message thread) ---------------------
    struct UserSampleInfo
    {
        bool         loaded { false };
        juce::String name;
        juce::String path;
        double       seconds { 0.0 };
        double       fileSampleRate { 0.0 };
        float        detectedBpm { 0.f };
        int          keyPitchClass { -1 };
        bool         keyMinor { false };
        int          rootNote { 60 };
    };

    static constexpr double kMaxUserSampleSeconds = 60.0;

    /** Loads a WAV/AIFF (<= 60 s) as the active sound. Returns false with a
        human-readable reason in errorOut. */
    bool loadUserSample (const juce::File& file, juce::String& errorOut);

    /** Message thread only. */
    const UserSampleInfo& getUserSampleInfo() const noexcept { return userSampleInfo; }
    bool isUserSampleLoaded() const noexcept { return userSampleInfo.loaded; }

    /** Message thread: fired after a user sample loads (UI refresh). */
    std::function<void()> onUserSampleChanged;

    static juce::String userSampleIdForFile (const juce::File& file) { return "user:" + file.getFullPathName(); }
    static bool isUserSampleId (const juce::String& id) { return id.startsWith ("user:"); }

    const std::array<MacroControl, 4>& getMacroControls() const noexcept { return macroControls; }

    /** Unit tests: true when the sampler holds a usable sample snapshot. */
    bool hasSamplerSampleForTest() const noexcept { return samplerEngine.hasLoadedSample(); }

    /** Last tempo reported by the host playhead (UI thread readout). */
    double getLastKnownHostBpm() const noexcept { return lastKnownHostBpm.load (std::memory_order_relaxed); }

    // --- FLIGHT DECK readouts (any thread, atomics) ---------------------------
    const Arpeggiator::UiState& getArpUiState() const noexcept { return arpeggiator.getUiState(); }
    float getPlayheadNorm() const noexcept { return stretchActiveForUi.load (std::memory_order_relaxed) ? stretchPlayer.getPlayheadNorm() : samplerEngine.getPlayheadNorm(); }
    bool  isStretchMode() const noexcept { return stretchActiveForUi.load (std::memory_order_relaxed); }
    float getTextureLevel() const noexcept { return textureLevel.load (std::memory_order_relaxed); }
    int   getActiveVoiceCount() const noexcept { return activeVoices.load (std::memory_order_relaxed); }
    double getTransportBeat() const noexcept { return transportBeatForUi.load (std::memory_order_relaxed); }
    bool  isFlipPending() const noexcept { return flipPendingForUi.load (std::memory_order_relaxed); }

    // --- MFX rack (message-thread helpers for the slot panels) ----------------
    MfxRack& getMfxRack() noexcept { return mfxRack; }

    struct MfxSnapshot
    {
        int effect { 0 };
        std::array<float, Mfx::kParamsPerSlot> values {};
        float send { 0.f };
        float level { 0.f };
    };

    /** Randomiser lock bits (bit n = slot param n). Plugin state, not a parameter. */
    uint32_t getMfxLocks (int slot) const noexcept { return mfxLocks[(size_t) juce::jlimit (0, Mfx::kNumSlots - 1, slot)]; }
    void     setMfxLocks (int slot, uint32_t mask) noexcept { mfxLocks[(size_t) juce::jlimit (0, Mfx::kNumSlots - 1, slot)] = mask; }

    MfxSnapshot captureMfx (int slot) const;
    void        applyMfx (int slot, const MfxSnapshot& snapshot);
    /** Push the current slot state before a destructive edit (reroll / surprise / effect change). */
    void pushMfxHistory (int slot);
    bool canUndoMfx (int slot) const noexcept { return ! mfxHistory[(size_t) juce::jlimit (0, Mfx::kNumSlots - 1, slot)].empty(); }
    bool undoMfx (int slot);
    /** Switch effect and load that effect's defaults (or a preset) into the bank. */
    void setMfxEffect (int slot, Mfx::Effect effect, int presetIndex = -1);

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    Arp::Settings readArpSettings() const noexcept;
    int  flipWindowFrames (int sampleFrames, double hostBpm) const noexcept;
    void renderVoiceSegment (juce::AudioBuffer<float>& buffer, int start, int numSamples, bool synthLayerOn);
    void dispatchTimelineMessage (const juce::MidiMessage& msg,
                                  bool reverse, float glideMs, float sourceBlend, bool synthLayerOn,
                                  int sampleFrames, double hostBpm, const SourceSettings& src);
    bool installSampleFromLibrary (const juce::String& sampleId);

    juce::AudioProcessorValueTreeState apvts;

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> inputGainSmoothed;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> outputGainSmoothed;

    SamplerEngine   samplerEngine;
    SynthEngine     synthEngine;
    FilterProcessor filterProcessor;
    TextureEngine   textureEngine;
    OutputLimiter   outputLimiter;
    MidiHandler     midiHandler;
    ToneShaper      toneShaper;
    ToneShaper      brightnessShaper;
    SmearProcessor  smearProcessor;
    ReverbTail      reverbTail;
    FxChain         fxChain;
    PerformanceTexturePipeline performancePipeline;
    Arpeggiator     arpeggiator;
    MfxRack         mfxRack;
    StretchPlayer   stretchPlayer;      // STRETCH playback mode voice
    juce::MidiBuffer filteredMidi;      // host MIDI minus consumed trigger notes
    float           lfo1Phase { 0.f };

    std::array<uint32_t, Mfx::kNumSlots> mfxLocks {};
    std::array<std::vector<MfxSnapshot>, Mfx::kNumSlots> mfxHistory;
    static constexpr size_t kMfxHistoryDepth = 32;

    // MIDI timeline (preallocated in prepareToPlay; never grows on the audio thread)
    juce::MidiBuffer timelineMidi;
    Arpeggiator::Event arpEvents[Arpeggiator::kMaxEvents];
    bool arpWasOn { false };

    // beat clock shared by flip snapping (arp keeps its own, host-synced copy)
    double transportBeat { 0.0 };
    bool   lastReverseParam { false };
    bool   flipPending { false };
    bool   flipPendingValue { false };
    bool   liveReverseApplied { false };
    int    lastFlipWindowFrames { 0 };
    bool   stretchModeThisBlock { false };

    std::array<MacroControl, 4> macroControls {};

    std::atomic<double> lastKnownHostBpm { 120.0 };
    std::atomic<float>  textureLevel { 0.f };
    std::atomic<int>    activeVoices { 0 };
    std::atomic<double> transportBeatForUi { 0.0 };
    std::atomic<bool>   flipPendingForUi { false };
    std::atomic<bool>   stretchActiveForUi { false };

    PerformanceApvtsReader::ParamCache perfParamCache;

    SampleLibrary          sampleLibrary;
    juce::HeapBlock<float> factoryWaveformCopy;
    int                    factoryRootNote { 60 };
    int                    factoryWaveformFrames { 0 };
    const float*           factoryWaveformData { nullptr };
    juce::String           loadedSampleId;
    UserSampleInfo         userSampleInfo;

    std::unique_ptr<PresetManager> presetManager;

    mutable juce::CriticalSection sampleLoadLock;
    bool isPrepared { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AviatorKeyzProcessor)
};
