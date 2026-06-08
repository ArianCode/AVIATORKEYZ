#pragma once

#include "../AviatorTokens.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

/** Animated LFO shape preview for one LFO strip. */
class LfoWaveformDisplay : public juce::Component,
                           private juce::Timer
{
public:
    explicit LfoWaveformDisplay (juce::AudioProcessorValueTreeState& apvts,
                                 const char* shapeParamId,
                                 const char* phaseParamId);

    void paint (juce::Graphics& g) override;

private:
    void timerCallback() override;
    float sampleShape (float phase01) const;

    juce::AudioProcessorValueTreeState& apvtsRef;
    juce::String shapeId;
    juce::String phaseId;
    float animPhase { 0.f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LfoWaveformDisplay)
};
