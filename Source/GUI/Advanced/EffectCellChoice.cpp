#include "EffectCellChoice.h"

EffectCellChoice::EffectCellChoice (juce::AudioProcessorValueTreeState& apvts,
                                    const juce::String& paramID,
                                    const juce::String& title,
                                    const juce::String& tooltip,
                                    juce::StringArray choices)
    : EffectCellShell (title, choices.size() <= 2 ? "CLICK = TOGGLE" : "CLICK = MODE", tooltip)
    , apvtsRef (apvts)
    , paramId (paramID)
    , choiceLabels (std::move (choices))
{
    if (choiceLabels.isEmpty())
    {
        if (auto* choiceParam = dynamic_cast<juce::AudioParameterChoice*> (apvtsRef.getParameter (paramId)))
            choiceLabels = choiceParam->choices;
        else
            choiceLabels = { "OFF", "ON" };
    }

    apvtsRef.addParameterListener (paramId, this);
}

EffectCellChoice::~EffectCellChoice()
{
    apvtsRef.removeParameterListener (paramId, this);
}

bool EffectCellChoice::isBoolParam() const
{
    return dynamic_cast<juce::AudioParameterBool*> (apvtsRef.getParameter (paramId)) != nullptr;
}

juce::String EffectCellChoice::currentLabel() const
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

void EffectCellChoice::cycleValue()
{
    if (auto* param = apvtsRef.getParameter (paramId))
    {
        if (auto* choiceParam = dynamic_cast<juce::AudioParameterChoice*> (param))
        {
            const int n = choiceParam->choices.size();
            if (n <= 0)
                return;

            const int next = (choiceParam->getIndex() + 1) % n;
            const float norm = choiceParam->getNormalisableRange().convertTo0to1 ((float) next);
            param->setValueNotifyingHost (norm);
            return;
        }

        if (isBoolParam())
        {
            const float next = param->getValue() > 0.5f ? 0.f : 1.f;
            param->setValueNotifyingHost (next);
        }
    }
}

void EffectCellChoice::parameterChanged (const juce::String& parameterID, float)
{
    if (parameterID != paramId)
        return;

    const auto apply = [self = juce::Component::SafePointer<EffectCellChoice> (this)]
    {
        if (self != nullptr)
            self->repaint();
    };

    if (juce::MessageManager::getInstance()->isThisTheMessageThread())
        apply();
    else
        juce::MessageManager::callAsync (apply);
}

void EffectCellChoice::paint (juce::Graphics& g)
{
    const float s = AviatorTokens::scaleFor (*this);
    const auto label = currentLabel();

    paintShell (g, {}, false);
    paintCenterLabel (g, label, s);
}

void EffectCellChoice::resized()
{
    EffectCellShell::resized();
}

void EffectCellChoice::mouseDown (const juce::MouseEvent& e)
{
    if (handleLockClick (e))
        return;

    if (isLocked())
        return;

    cycleValue();
}

void EffectCellChoice::mouseDoubleClick (const juce::MouseEvent& e)
{
    if (isLocked() || lockBounds.expanded (4).contains (e.getPosition()))
        return;

    if (auto* param = apvtsRef.getParameter (paramId))
        param->setValueNotifyingHost (param->getDefaultValue());
}
