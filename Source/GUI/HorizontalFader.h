#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

class HorizontalFader : public juce::Component
{
public:
    using TextFormatter = std::function<juce::String (float value)>;

    HorizontalFader (juce::AudioProcessorValueTreeState& apvts,
                     const juce::String& paramID,
                     const juce::String& shortLabel,
                     TextFormatter formatter);

    void paint (juce::Graphics& g) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;
    void mouseUp (const juce::MouseEvent& e) override;

private:
    void setFromMouse (int x);
    juce::Rectangle<int> trackBounds() const;
    float getProportion() const;

    juce::Slider slider;
    juce::Label  shortLabel;
    juce::Label  valueLabel;
    TextFormatter formatter;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HorizontalFader)
};
