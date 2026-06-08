#pragma once

#include "../AviatorTokens.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

/** Horizontal bipolar mod amount slider with live indicator dot. */
class ModAmountSlider : public juce::Component,
                        private juce::Timer,
                        private juce::AudioProcessorValueTreeState::Listener
{
public:
    ModAmountSlider (juce::AudioProcessorValueTreeState& apvts,
                     const juce::String& paramId,
                     juce::Colour sourceColour);

    ~ModAmountSlider() override;

    void setLiveOffset (float bipolarOffset);
    void setAccentColour (juce::Colour colour);
    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;

private:
    void timerCallback() override;
    void parameterChanged (const juce::String& id, float) override;
    float readAmount() const;
    void setAmountFromX (int x);

    juce::AudioProcessorValueTreeState& apvtsRef;
    juce::String paramId;
    juce::Colour accent;
    float liveOffset { 0.f };
    juce::Slider hiddenSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ModAmountSlider)
};
