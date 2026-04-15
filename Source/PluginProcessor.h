#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

#include "State/StateSchema.h"

// =============================================================================
//  AviatorKeyzProcessor — root AudioProcessor
//
//  Responsibilities:
//    - Own the AudioProcessorValueTreeState (APVTS)
//    - Orchestrate DSP module chain on the audio thread
//    - Implement state save/restore with schema versioning
//    - Handle prepareToPlay / releaseResources lifecycle
//
//  Architecture rules:
//    - DSP modules are prepared here and called from processBlock
//    - GUI must only access APVTS parameters — never call DSP methods directly
//    - No allocations, no UI calls, no locks inside processBlock
// =============================================================================

class AviatorKeyzProcessor final : public juce::AudioProcessor
{
public:
    AviatorKeyzProcessor();
    ~AviatorKeyzProcessor() override;

    // -------------------------------------------------------------------------
    // AudioProcessor interface
    // -------------------------------------------------------------------------
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void processBlockBypassed (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    // -------------------------------------------------------------------------
    // Plugin identity
    // -------------------------------------------------------------------------
    const juce::String getName() const override         { return JucePlugin_Name; }
    bool   acceptsMidi() const override                 { return true; }
    bool   producesMidi() const override                { return false; }
    bool   isMidiEffect() const override                { return false; }
    double getTailLengthSeconds() const override        { return 2.0; }

    // -------------------------------------------------------------------------
    // Programs (not used — presets handled by PresetManager)
    // -------------------------------------------------------------------------
    int  getNumPrograms() override                              { return 1; }
    int  getCurrentProgram() override                           { return 0; }
    void setCurrentProgram (int) override                       {}
    const juce::String getProgramName (int) override            { return {}; }
    void changeProgramName (int, const juce::String&) override  {}

    // -------------------------------------------------------------------------
    // State persistence — called by host on project save/load
    // CompatibilityNote: schema versioned; migrations live in setStateInformation
    // -------------------------------------------------------------------------
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // -------------------------------------------------------------------------
    // Public accessors (GUI / PresetManager use only)
    // -------------------------------------------------------------------------
    juce::AudioProcessorValueTreeState& getAPVTS() noexcept { return apvts; }

private:
    // -------------------------------------------------------------------------
    // Parameter layout — called once at construction
    // CompatibilityNote: param IDs are locked; see StateSchema.h
    // -------------------------------------------------------------------------
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // -------------------------------------------------------------------------
    // APVTS — single source of truth for all automatable parameters
    // -------------------------------------------------------------------------
    juce::AudioProcessorValueTreeState apvts;

    // -------------------------------------------------------------------------
    // Smoothed parameter values (audio-thread safe)
    // These track their corresponding APVTS params and prevent zipper noise.
    // Reset timing: 20 ms default, set in prepareToPlay.
    // -------------------------------------------------------------------------
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> inputGainSmoothed;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> outputGainSmoothed;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> toneSmoothed;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smearSmoothed;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> reverbAmountSmoothed;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> stereoWidthSmoothed;

    // -------------------------------------------------------------------------
    // DSP module placeholders — replaced with real implementations in M1/M2
    // -------------------------------------------------------------------------
    // std::unique_ptr<SamplerEngine>    samplerEngine;
    // std::unique_ptr<ToneShaper>       toneShaper;
    // std::unique_ptr<SmearProcessor>   smearProcessor;
    // std::unique_ptr<GlideEngine>      glideEngine;
    // std::unique_ptr<ReversePlayer>    reversePlayer;
    // juce::dsp::Reverb                 reverb;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AviatorKeyzProcessor)
};
