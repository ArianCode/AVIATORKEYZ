#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

#include "DSP/SamplerEngine.h"
#include "DSP/ToneShaper.h"
#include "DSP/SmearProcessor.h"
#include "DSP/ReverbTail.h"
#include "MIDI/MidiHandler.h"
#include "State/StateSchema.h"
#include "State/PresetManager.h"
#include "State/SampleLibrary.h"

// =============================================================================
//  AviatorKeyzProcessor — root AudioProcessor
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
    bool hasEditor() const override { return true; }

    const juce::String getName() const override         { return JucePlugin_Name; }
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

    /** Message / UI thread: factory waveform thumbnail (mono). May be null if empty. */
    const float* getFactoryWaveformData() const noexcept;
    int          getFactoryWaveformFrames() const noexcept { return factoryWaveformFrames; }

    /** Message thread: load embedded factory sample by id (from preset sampleId). */
    void loadFactorySample (const juce::String& sampleId, int rootNote = 60);

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    static void applyStereoWidth (juce::AudioBuffer<float>& buffer, float width) noexcept;
    static void applyPan (juce::AudioBuffer<float>& buffer, float pan) noexcept;

    juce::AudioProcessorValueTreeState apvts;

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> inputGainSmoothed;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> outputGainSmoothed;

    SamplerEngine   samplerEngine;
    MidiHandler     midiHandler;
    ToneShaper      toneShaper;
    SmearProcessor  smearProcessor;
    ReverbTail      reverbTail;

    SampleLibrary          sampleLibrary;
    int                    factoryRootNote { 60 };
    int                    factoryWaveformFrames { 0 };
    juce::String           loadedSampleId;

    std::unique_ptr<PresetManager> presetManager;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AviatorKeyzProcessor)
};
