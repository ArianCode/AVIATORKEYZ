#include "EffectCell.h"

EffectCell::EffectCell (juce::AudioProcessorValueTreeState& apvts,
                       const juce::String& paramID,
                       const juce::String& title,
                       const juce::String& dragHint,
                       const juce::String& tooltip,
                       Format format)
    : EffectCellShell (title, dragHint, tooltip)
    , apvtsRef (apvts)
    , paramId (paramID)
    , valueFormat (format)
{
    slider.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.setVelocityModeParameters (1.0, 1, 0.0, true);
    slider.setInterceptsMouseClicks (false, false);
    slider.onValueChange = [this] { repaint(); };
    slider.setVisible (false);
    addChildComponent (slider);

    attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        apvtsRef, paramId, slider);
}

float EffectCell::getNormalisedValue() const
{
    return (float) slider.getNormalisableRange().convertTo0to1 (slider.getValue());
}

juce::String EffectCell::valueText() const
{
    return EffectCellFormat::formatValue (apvtsRef, paramId, valueFormat, (float) slider.getValue());
}

void EffectCell::resized()
{
    EffectCellShell::resized();
    slider.setBounds (getLocalBounds());
}

void EffectCell::paint (juce::Graphics& g)
{
    const float s = AviatorTokens::scaleFor (*this);
    const auto valText = valueText();

    paintShell (g, valText, dragging);
    paintCenterMeter (g, getNormalisedValue(), s);
}

void EffectCell::mouseDown (const juce::MouseEvent& e)
{
    if (handleLockClick (e))
        return;

    if (isLocked())
        return;

    dragging = true;
    setDragging (true);
    slider.mouseDown (e.getEventRelativeTo (&slider));
}

void EffectCell::mouseDrag (const juce::MouseEvent& e)
{
    if (isLocked() || ! dragging)
        return;

    slider.mouseDrag (e.getEventRelativeTo (&slider));
}

void EffectCell::mouseUp (const juce::MouseEvent& e)
{
    if (! dragging)
        return;

    dragging = false;
    setDragging (false);
    slider.mouseUp (e.getEventRelativeTo (&slider));
}

void EffectCell::mouseDoubleClick (const juce::MouseEvent& e)
{
    if (isLocked() || lockBounds.expanded (4).contains (e.getPosition()))
        return;

    if (auto* param = apvtsRef.getParameter (paramId))
        param->setValueNotifyingHost (param->getDefaultValue());
}
