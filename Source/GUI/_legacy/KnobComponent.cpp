#include "KnobComponent.h"

KnobComponent::KnobComponent (juce::AudioProcessorValueTreeState& apvts,
                               const juce::String& paramID,
                               const juce::String& labelText)
{
    slider.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    addAndMakeVisible (slider);

    label.setText (labelText, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (label);

    attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        apvts, paramID, slider);
}

KnobComponent::~KnobComponent() = default;

void KnobComponent::resized()
{
    auto area = getLocalBounds();
    const int labelH = 18;
    label.setBounds (area.removeFromBottom (labelH));
    slider.setBounds (area);
}
