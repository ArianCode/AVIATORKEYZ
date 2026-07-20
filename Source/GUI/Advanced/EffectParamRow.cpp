#include "EffectParamRow.h"

EffectParamRow::EffectParamRow (juce::AudioProcessorValueTreeState& apvts,
                                const juce::String& paramID,
                                const juce::String& label,
                                const juce::String& tooltip,
                                EffectCell::Format format,
                                juce::StringArray choices)
    : apvtsRef (apvts)
    , paramId (paramID)
    , labelText (label)
    , valueFormat (format)
    , choiceLabels (std::move (choices))
{
    setOpaque (false);
    setTooltip (tooltip);

    if (auto* choiceParam = dynamic_cast<juce::AudioParameterChoice*> (apvtsRef.getParameter (paramId)))
    {
        choiceRow = true;
        if (choiceLabels.isEmpty())
            choiceLabels = choiceParam->choices;
    }
    else if (dynamic_cast<juce::AudioParameterBool*> (apvtsRef.getParameter (paramId)) != nullptr)
    {
        choiceRow = true;
        if (choiceLabels.isEmpty())
            choiceLabels = { "OFF", "ON" };
    }
    else
    {
        slider.setSliderStyle (juce::Slider::LinearBarVertical);
        slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        slider.setInterceptsMouseClicks (false, false);
        slider.onValueChange = [this]
        {
            repaint();
            if (auto* parent = getParentComponent())
                parent->repaint();
        };
        slider.setVisible (false);
        addChildComponent (slider);
        attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
            apvtsRef, paramId, slider);
    }

    apvtsRef.addParameterListener (paramId, this);
}

EffectParamRow::~EffectParamRow()
{
    apvtsRef.removeParameterListener (paramId, this);
}

bool EffectParamRow::isBoolParam() const
{
    return dynamic_cast<juce::AudioParameterBool*> (apvtsRef.getParameter (paramId)) != nullptr;
}

float EffectParamRow::getNormalisedValue() const
{
    if (choiceRow)
        return 0.f;

    return (float) slider.getNormalisableRange().convertTo0to1 (slider.getValue());
}

juce::String EffectParamRow::valueText() const
{
    if (choiceRow)
        return currentChoiceLabel();

    return EffectCellFormat::formatValue (apvtsRef, paramId, valueFormat, (float) slider.getValue());
}

juce::String EffectParamRow::currentChoiceLabel() const
{
    if (auto* param = apvtsRef.getParameter (paramId))
    {
        if (auto* choiceParam = dynamic_cast<juce::AudioParameterChoice*> (param))
        {
            const int idx = choiceParam->getIndex();
            if (juce::isPositiveAndBelow (idx, choiceLabels.size()))
                return choiceLabels[idx].toUpperCase();
        }

        if (isBoolParam())
            return param->getValue() > 0.5f ? "ON" : "OFF";
    }

    return {};
}

void EffectParamRow::cycleChoice()
{
    if (auto* param = apvtsRef.getParameter (paramId))
    {
        if (auto* choiceParam = dynamic_cast<juce::AudioParameterChoice*> (param))
        {
            const int n = choiceParam->choices.size();
            if (n <= 0)
                return;

            const int next = (choiceParam->getIndex() + 1) % n;
            param->setValueNotifyingHost (choiceParam->getNormalisableRange().convertTo0to1 ((float) next));
            return;
        }

        if (isBoolParam())
            param->setValueNotifyingHost (param->getValue() > 0.5f ? 0.f : 1.f);
    }
}

void EffectParamRow::parameterChanged (const juce::String& parameterID, float)
{
    if (parameterID != paramId)
        return;

    const auto apply = [self = juce::Component::SafePointer<EffectParamRow> (this)]
    {
        if (self != nullptr)
        {
            self->repaint();
            if (auto* parent = self->getParentComponent())
                parent->repaint();
        }
    };

    if (juce::MessageManager::getInstance()->isThisTheMessageThread())
        apply();
    else
        juce::MessageManager::callAsync (apply);
}

void EffectParamRow::updateLockBounds()
{
    const float s = AviatorTokens::scaleFor (*this);
    const int lockSize = AviatorTokens::scaled (12, s);
    lockBounds = juce::Rectangle<int> (getWidth() - lockSize - AviatorTokens::scaled (4, s),
                                       (getHeight() - lockSize) / 2,
                                       lockSize, lockSize);
}

void EffectParamRow::drawLockIcon (juce::Graphics& g, juce::Rectangle<float> bounds, float scale) const
{
    const auto colour = locked ? AviatorTokens::mfdAmber() : juce::Colours::white.withAlpha (0.25f);
    const float bodyH = bounds.getHeight() * 0.55f;
    auto body = bounds.removeFromBottom (bodyH);
    g.setColour (colour.withAlpha (locked ? 0.85f : 0.45f));
    g.fillRoundedRectangle (body, 1.f * scale);
}

void EffectParamRow::paintThumb (juce::Graphics& g, juce::Rectangle<int> rowBounds, float scale) const
{
    if (choiceRow || locked)
        return;

    const float val = juce::jlimit (0.f, 1.f, getNormalisedValue());
    const int trackW = AviatorTokens::scaled (4, scale);
    const int thumbR = AviatorTokens::scaled (3, scale);
    const int trackTop = rowBounds.getY() + AviatorTokens::scaled (2, scale);
    const int trackBottom = rowBounds.getBottom() - AviatorTokens::scaled (2, scale);
    const int trackH = juce::jmax (1, trackBottom - trackTop);
    const int thumbY = trackBottom - (int) std::round (val * (float) trackH);

    const int trackX = railColumnX + (AviatorTokens::scaled (10, scale) - trackW) / 2;
    g.setColour (AviatorTokens::instrumentCyan().withAlpha (0.95f));
    g.fillEllipse ((float) (trackX + trackW / 2 - thumbR),
                   (float) (thumbY - thumbR),
                   (float) (thumbR * 2),
                   (float) (thumbR * 2));
}

void EffectParamRow::paint (juce::Graphics& g)
{
    const float s = AviatorTokens::scaleFor (*this);
    auto bounds = getLocalBounds();
    const int railW = AviatorTokens::scaled (10, s);
    auto textArea = bounds.withTrimmedRight (railW + AviatorTokens::scaled (4, s));

    g.setFont (AviatorTokens::hudBold (8.5f * s));
    g.setColour (AviatorTokens::instrumentCyan().withAlpha (locked ? 0.35f : 0.88f));
    const int labelW = juce::jmin (textArea.getWidth() / 3, AviatorTokens::scaled (52, s));
    g.drawText (labelText, textArea.removeFromLeft (labelW), juce::Justification::centredLeft);

    g.setFont (AviatorTokens::hud (8.5f * s));
    g.setColour (locked ? juce::Colours::white.withAlpha (0.28f) : AviatorTokens::champagneGold());

    if (choiceRow)
    {
        auto valueArea = textArea.removeFromLeft (textArea.getWidth() / 2);
        g.drawText (currentChoiceLabel(), valueArea, juce::Justification::centredLeft);
        g.setFont (AviatorTokens::hud (7.f * s));
        g.setColour (juce::Colours::white.withAlpha (0.28f));
        g.drawText (choiceLabels.size() <= 2 ? "CLICK = TOGGLE" : "CLICK = MODE",
                    textArea, juce::Justification::centredRight);
    }
    else
    {
        g.drawText (valueText(), textArea, juce::Justification::centred);
    }

    drawLockIcon (g, lockBounds.toFloat(), s);

    if (dragging && ! choiceRow)
    {
        g.setColour (AviatorTokens::instrumentCyan().withAlpha (0.08f));
        g.fillRect (bounds);
    }
}

void EffectParamRow::resized()
{
    updateLockBounds();
    slider.setBounds (getLocalBounds());
}

void EffectParamRow::mouseDown (const juce::MouseEvent& e)
{
    if (lockBounds.expanded (3).contains (e.getPosition()))
    {
        locked = ! locked;
        repaint();
        return;
    }

    if (locked)
        return;

    if (choiceRow)
    {
        cycleChoice();
        return;
    }

    dragging = true;
    dragStartValue = (float) slider.getValue();
    dragStartY = e.getPosition().y;
    slider.mouseDown (e.getEventRelativeTo (&slider));
}

void EffectParamRow::mouseDrag (const juce::MouseEvent& e)
{
    if (locked || choiceRow || ! dragging)
        return;

    slider.mouseDrag (e.getEventRelativeTo (&slider));
    repaint();
}

void EffectParamRow::mouseUp (const juce::MouseEvent& e)
{
    if (! dragging)
        return;

    dragging = false;
    slider.mouseUp (e.getEventRelativeTo (&slider));
    repaint();
}

void EffectParamRow::mouseDoubleClick (const juce::MouseEvent& e)
{
    if (locked || lockBounds.expanded (3).contains (e.getPosition()))
        return;

    if (auto* param = apvtsRef.getParameter (paramId))
        param->setValueNotifyingHost (param->getDefaultValue());
}
