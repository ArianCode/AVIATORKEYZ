#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

class ReverseToggle : public juce::Component
{
public:
    ReverseToggle (juce::AudioProcessorValueTreeState& apvts,
                   const juce::String& paramID,
                   const juce::String& displayName = "Reverse",
                   const juce::String& sublabel = "Playback Phase");

    void paint (juce::Graphics& g) override;
    void resized() override;
    void mouseUp (const juce::MouseEvent& e) override;

private:
    void syncState();

    juce::ToggleButton button;
    juce::Label        nameLabel;
    juce::Label        subLabel;
    juce::Label        stateLabel;
    juce::Label        valueLabel;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> attachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ReverseToggle)
};
