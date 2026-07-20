#pragma once

#include "MiniControls.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

// =============================================================================
//  VelocityPanel — left cockpit glass display: VELOCITY CURVE.
//  Cyan curve showing the real velocity response (velocity_sensitivity):
//  out = (1 - s) + s * velocity. Vertical drag edits the parameter.
// =============================================================================

class VelocityPanel : public juce::Component
{
public:
    explicit VelocityPanel (juce::AudioProcessorValueTreeState& apvts);

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    juce::AudioProcessorValueTreeState& apvtsRef;
    JetsonicMini::FineDragSlider slider { juce::Slider::RotaryVerticalDrag, juce::Slider::NoTextBox };
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VelocityPanel)
};
