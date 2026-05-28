#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

// =============================================================================
//  KnobComponent — M4
//
//  Reusable premium knob widget.
//
//  Features:
//    - Rotary slider with LuxuryLookAndFeel rendering
//    - Label below showing parameter name
//    - Value tooltip on hover / while dragging
//    - Double-click to reset to default
//    - Right-click context menu: "Reset to default", "Enter value"
//    - APVTS attachment owned internally
// =============================================================================

class KnobComponent : public juce::Component
{
public:
    KnobComponent (juce::AudioProcessorValueTreeState& apvts,
                   const juce::String& paramID,
                   const juce::String& labelText);
    ~KnobComponent() override;

    void resized() override;

private:
    juce::Slider slider;
    juce::Label  label;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KnobComponent)
};
