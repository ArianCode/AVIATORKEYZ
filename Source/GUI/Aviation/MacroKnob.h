#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>

// =============================================================================
//  MacroKnob — heavy premium rotary for the bottom macro deck.
//  Black outer housing, blue-white LED tick ring, bronze cap, gold label,
//  white value readout, blue secondary function label.
//
//  Bound to one APVTS parameter (SliderAttachment => host automation, preset
//  recall and double-click-to-default all work). Hold Cmd/Ctrl/Shift for fine
//  drag. Layout is in Aviation design pixels (whole view is scaled outside).
// =============================================================================

class MacroKnob : public juce::Component
{
public:
    MacroKnob (juce::AudioProcessorValueTreeState& apvts,
               const juce::String& paramId,
               const juce::String& title,
               const juce::String& subLabel);
    ~MacroKnob() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    class KnobSlider : public juce::Slider
    {
    public:
        using juce::Slider::Slider;
        void mouseDown (const juce::MouseEvent& e) override;
        void mouseEnter (const juce::MouseEvent& e) override;
        void mouseExit (const juce::MouseEvent& e) override;
    };

    juce::String valueText() const;

    juce::AudioProcessorValueTreeState& apvtsRef;
    juce::String paramId;
    juce::String titleText;
    juce::String subText;

    KnobSlider slider { juce::Slider::RotaryVerticalDrag, juce::Slider::NoTextBox };
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MacroKnob)
};
