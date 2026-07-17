#pragma once

#include "EffectCell.h"
#include "EffectCellFormat.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

/** Single parameter row inside an EffectParamBox (continuous drag or click-to-cycle). */
class EffectParamRow : public juce::Component,
                         public juce::SettableTooltipClient,
                         private juce::AudioProcessorValueTreeState::Listener
{
public:
    EffectParamRow (juce::AudioProcessorValueTreeState& apvts,
                    const juce::String& paramID,
                    const juce::String& label,
                    const juce::String& tooltip,
                    EffectCell::Format format = EffectCell::Format::percent,
                    juce::StringArray choices = {});

    ~EffectParamRow() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;
    void mouseUp (const juce::MouseEvent& e) override;
    void mouseDoubleClick (const juce::MouseEvent& e) override;

    bool isChoiceRow() const noexcept { return choiceRow; }
    float getNormalisedValue() const;
    juce::String valueText() const;
    bool isLocked() const noexcept { return locked; }

    void setRailColumnX (int x) noexcept { railColumnX = x; }
    void paintThumb (juce::Graphics& g, juce::Rectangle<int> rowBounds, float scale) const;

private:
    void parameterChanged (const juce::String& parameterID, float newValue) override;
    bool isBoolParam() const;
    juce::String currentChoiceLabel() const;
    void cycleChoice();
    void drawLockIcon (juce::Graphics& g, juce::Rectangle<float> bounds, float scale) const;
    void updateLockBounds();

    juce::AudioProcessorValueTreeState& apvtsRef;
    juce::String paramId;
    juce::String labelText;
    EffectCell::Format valueFormat;
    juce::StringArray choiceLabels;
    bool choiceRow { false };

    juce::Slider slider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;

    bool locked    { false };
    bool dragging  { false };
    int  railColumnX { 0 };
    juce::Rectangle<int> lockBounds;
    float dragStartValue { 0.f };
    int dragStartY { 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EffectParamRow)
};
