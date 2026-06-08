#pragma once

#include "../AviatorTokens.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

/** Static OSC waveform preview driven by type + shape APVTS params. */
class OscWaveformDisplay : public juce::Component,
                           private juce::AudioProcessorValueTreeState::Listener,
                           private juce::Timer
{
public:
    OscWaveformDisplay (juce::AudioProcessorValueTreeState& apvts,
                        const char* typeParamId,
                        const char* shapeParamId,
                        juce::Colour waveColour);

    ~OscWaveformDisplay() override;

    void paint (juce::Graphics& g) override;

private:
    void parameterChanged (const juce::String& parameterID, float newValue) override;
    void timerCallback() override;
    float sampleAt (float phase) const;

    juce::AudioProcessorValueTreeState& apvtsRef;
    juce::String typeId;
    juce::String shapeId;
    juce::Colour colour;
    float animPhase { 0.f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OscWaveformDisplay)
};
