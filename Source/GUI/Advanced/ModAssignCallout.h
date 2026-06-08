#pragma once

#include "ModRoutingHub.h"
#include "../AviatorTokens.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <functional>

/** Popup: pick LFO source and initial mod depth for a knob destination. */
class ModAssignCallout : public juce::Component
{
public:
    ModAssignCallout (juce::AudioProcessorValueTreeState& apvts,
                      const juce::String& targetParamId,
                      std::function<void()> onDone);

    void resized() override;
    void paint (juce::Graphics& g) override;

    static void showForKnob (juce::Component& anchor,
                           juce::AudioProcessorValueTreeState& apvts,
                           const juce::String& paramId);

private:
    juce::AudioProcessorValueTreeState& apvtsRef;
    juce::String paramId;
    juce::ComboBox sourceBox;
    juce::Slider amountSlider;
    juce::TextButton applyButton { "Assign" };
    juce::TextButton clearButton { "Clear" };
    std::function<void()> doneCallback;
};
