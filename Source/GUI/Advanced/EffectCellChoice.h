#pragma once

#include "EffectCellShell.h"
#include <juce_audio_processors/juce_audio_processors.h>

/** Click-to-cycle tile for bool and choice APVTS parameters. */
class EffectCellChoice : public EffectCellShell,
                         private juce::AudioProcessorValueTreeState::Listener
{
public:
    EffectCellChoice (juce::AudioProcessorValueTreeState& apvts,
                      const juce::String& paramID,
                      const juce::String& title,
                      const juce::String& tooltip,
                      juce::StringArray choices = {});

    ~EffectCellChoice() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDoubleClick (const juce::MouseEvent& e) override;

private:
    void parameterChanged (const juce::String& parameterID, float newValue) override;

    juce::String currentLabel() const;
    void cycleValue();
    bool isBoolParam() const;

    juce::AudioProcessorValueTreeState& apvtsRef;
    juce::String paramId;
    juce::StringArray choiceLabels;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EffectCellChoice)
};
