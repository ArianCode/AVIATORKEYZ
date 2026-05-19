#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

class PrecisionKnob : public juce::Component
{
public:
    enum class ValueFormat
    {
        glideSeconds,
        percent,
        toneDb
    };

    PrecisionKnob (juce::AudioProcessorValueTreeState& apvts,
                   const juce::String& paramID,
                   const juce::String& macroName,
                   const juce::String& sublabel,
                   ValueFormat format);

    void paint (juce::Graphics& g) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;
    void mouseUp (const juce::MouseEvent& e) override;
    void mouseDoubleClick (const juce::MouseEvent& e) override;

    juce::String getValueText() const;

private:
    void syncFromSlider();
    float getNormalisedValue() const;

    juce::Slider slider;
    juce::Label  nameLabel;
    juce::Label  subLabel;
    juce::Label  valueLabel;
    ValueFormat  valueFormat;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PrecisionKnob)
};
