#include "BracketValueBox.h"

namespace
{
constexpr float kBracketLen   = 8.f;
constexpr float kBracketThick = 1.f;
const juce::Colour kBracketGold  { 0xffC8A84B };
const juce::Colour kLabelCyan    { 0xff4DB8D4 };
} // namespace

BracketValueBox::BracketValueBox (juce::AudioProcessorValueTreeState& apvts,
                                  const juce::String& id,
                                  const juce::String& label,
                                  Format format,
                                  juce::Colour brackets)
    : apvtsRef (apvts)
    , paramId (id)
    , labelText (label.toUpperCase())
    , valueFormat (format)
    , bracketColour (brackets)
{
    setOpaque (false);
    hiddenSlider.setSliderStyle (juce::Slider::LinearBarVertical);
    hiddenSlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    hiddenSlider.setVisible (false);
    addChildComponent (hiddenSlider);

    attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, id, hiddenSlider);
    apvtsRef.addParameterListener (id, this);
    syncFromParam();
}

BracketValueBox::~BracketValueBox()
{
    apvtsRef.removeParameterListener (paramId, this);
}

void BracketValueBox::parameterChanged (const juce::String& parameterID, float)
{
    if (parameterID != paramId)
        return;

    const auto apply = [self = juce::Component::SafePointer<BracketValueBox> (this)]
    {
        if (self != nullptr)
            self->syncFromParam();
    };

    if (juce::MessageManager::getInstance()->isThisTheMessageThread())
        apply();
    else
        juce::MessageManager::callAsync (apply);
}

void BracketValueBox::syncFromParam()
{
    repaint();
}

juce::String BracketValueBox::formatValue() const
{
    const float v = hiddenSlider.getValue();

    switch (valueFormat)
    {
        case Format::percent:
            if (auto* p = dynamic_cast<juce::RangedAudioParameter*> (apvtsRef.getParameter (paramId)))
            {
                const float norm = p->getNormalisableRange().convertTo0to1 (v);
                if (paramId.contains ("level") || paramId.contains ("amount") || paramId.contains ("mix")
                    || paramId.contains ("depth") || paramId.contains ("sustain") || paramId.contains ("shape")
                    || paramId.contains ("blend") || paramId.contains ("spread") || paramId.contains ("width")
                    || paramId.contains ("density") || paramId.contains ("motion") || paramId.contains ("drift")
                    || paramId.contains ("air") || paramId.contains ("scan") || paramId.contains ("size")
                    || paramId.contains ("rate") && ! paramId.contains ("lfo") && ! paramId.contains ("chorus")
                    || paramId.contains ("smear") || paramId.contains ("reverb_amount") || paramId.contains ("lofi")
                    || paramId.contains ("dist") || paramId.contains ("damp") || paramId.contains ("feedback")
                    || paramId.contains ("resonance") || paramId.contains ("drive") && ! paramId.contains ("filter")
                    || paramId.contains ("mod_") && paramId.contains ("amount"))
                    return juce::String (juce::roundToInt (norm * 100.f)) + "%";
            }
            return juce::String (juce::roundToInt (v * 100.f)) + "%";

        case Format::hz:
            return juce::String (v, v < 10.f ? 2 : 1);

        case Format::ms:
            if (v >= 1000.f)
                return juce::String (v / 1000.f, 2) + "s";
            return juce::String (juce::roundToInt (v));

        case Format::semitones:
            return juce::String (juce::roundToInt (v));

        case Format::cents:
            return juce::String (juce::roundToInt (v));

        case Format::pan:
            return juce::String (v, 2);

        case Format::cutoff:
            if (v >= 1000.f)
                return juce::String (v / 1000.f, 1) + "k";
            return juce::String (juce::roundToInt (v));

        case Format::decibels:
            return juce::String (v, v > -10.f && v < 10.f ? 1 : 0);

        case Format::integer:
            return juce::String (juce::roundToInt (v));

        case Format::plain:
        default:
            if (v == juce::roundToInt (v))
                return juce::String (juce::roundToInt (v));
            return juce::String (v, 2);
    }
}

void BracketValueBox::paintBrackets (juce::Graphics& g, juce::Rectangle<float> b) const
{
    const float len = juce::jmin (kBracketLen, b.getWidth() * 0.22f);
    const float t = kBracketThick;
    g.setColour (kBracketGold);

    // top-left
    g.drawLine (b.getX(), b.getY() + len, b.getX(), b.getY(), t);
    g.drawLine (b.getX(), b.getY(), b.getX() + len, b.getY(), t);
    // top-right
    g.drawLine (b.getRight() - len, b.getY(), b.getRight(), b.getY(), t);
    g.drawLine (b.getRight(), b.getY(), b.getRight(), b.getY() + len, t);
    // bottom-left
    g.drawLine (b.getX(), b.getBottom() - len, b.getX(), b.getBottom(), t);
    g.drawLine (b.getX(), b.getBottom(), b.getX() + len, b.getBottom(), t);
    // bottom-right
    g.drawLine (b.getRight() - len, b.getBottom(), b.getRight(), b.getBottom(), t);
    g.drawLine (b.getRight(), b.getBottom() - len, b.getRight(), b.getBottom(), t);
}

void BracketValueBox::paint (juce::Graphics& g)
{
    const float h = (float) getHeight();
    const bool large = getWidth() >= 58;
    const float valueSize = large ? 13.f : juce::jlimit (10.f, 14.f, h * 0.38f);
    const float labelSize = large ? 8.f : juce::jlimit (6.f, 8.f, h * 0.2f);
    auto bounds = getLocalBounds().toFloat().reduced (0.5f);

    paintBrackets (g, bounds);

    g.setFont (AviatorTokens::hud (valueSize));
    g.setColour (juce::Colours::white);
    auto valueArea = bounds.reduced (2.f, 4.f);
    valueArea.removeFromBottom (labelSize + 2.f);
    g.drawText (formatValue(), valueArea, juce::Justification::centred);

    g.setFont (AviatorTokens::hud (labelSize));
    g.setColour (kLabelCyan);
    g.drawText (labelText, bounds.removeFromBottom (labelSize + 1.f), juce::Justification::centred);
}

void BracketValueBox::mouseDown (const juce::MouseEvent& e)
{
    dragStartValue = hiddenSlider.getValue();
    dragStartY = e.y;
}

void BracketValueBox::mouseDrag (const juce::MouseEvent& e)
{
    const float range = hiddenSlider.getMaximum() - hiddenSlider.getMinimum();
    const float delta = (float) (dragStartY - e.y) * range * 0.008f;
    hiddenSlider.setValue (juce::jlimit (hiddenSlider.getMinimum(), hiddenSlider.getMaximum(),
                                         (double) dragStartValue + (double) delta),
                           juce::sendNotificationSync);
}

void BracketValueBox::mouseUp (const juce::MouseEvent&) {}

void BracketValueBox::mouseDoubleClick (const juce::MouseEvent&)
{
    showTextEditor();
}

void BracketValueBox::showTextEditor()
{
    auto* ed = new juce::TextEditor();
    ed->setText (formatValue().retainCharacters ("0123456789.-"));
    ed->setFont (AviatorTokens::hud (12.f));
    ed->setColour (juce::TextEditor::backgroundColourId, juce::Colour (0xff0a1628));
    ed->setColour (juce::TextEditor::textColourId, AviatorTokens::textPrimary());
    ed->setJustification (juce::Justification::centred);
    ed->setBounds (getLocalBounds());
    ed->onReturnKey = ed->onFocusLost = [this, ed]
    {
        const float typed = ed->getText().getFloatValue();
        hiddenSlider.setValue (juce::jlimit (hiddenSlider.getMinimum(), hiddenSlider.getMaximum(), (double) typed),
                               juce::sendNotificationSync);
        removeChildComponent (ed);
        delete ed;
    };
    addAndMakeVisible (ed);
    ed->grabKeyboardFocus();
    ed->selectAll();
}
