#include "ModAmountSlider.h"

ModAmountSlider::ModAmountSlider (juce::AudioProcessorValueTreeState& apvts,
                                  const juce::String& id,
                                  juce::Colour colour)
    : apvtsRef (apvts)
    , paramId (id)
    , accent (colour)
{
    hiddenSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    hiddenSlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    hiddenSlider.setVisible (false);
    addChildComponent (hiddenSlider);
    attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, id, hiddenSlider);
    apvtsRef.addParameterListener (paramId, this);
    startTimerHz (30);
}

ModAmountSlider::~ModAmountSlider()
{
    apvtsRef.removeParameterListener (paramId, this);
}

float ModAmountSlider::readAmount() const
{
    if (auto* raw = apvtsRef.getRawParameterValue (paramId))
        return raw->load();
    return 0.f;
}

void ModAmountSlider::setLiveOffset (float bipolarOffset)
{
    liveOffset = juce::jlimit (-1.f, 1.f, bipolarOffset);
}

void ModAmountSlider::setAccentColour (juce::Colour colour)
{
    accent = colour;
}

void ModAmountSlider::parameterChanged (const juce::String& id, float)
{
    if (id == paramId)
        repaint();
}

void ModAmountSlider::timerCallback()
{
    repaint();
}

void ModAmountSlider::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced (2.f, 6.f);
    const float midX = bounds.getCentreX();
    const float trackY = bounds.getCentreY();

    g.setColour (juce::Colour (0xff142840));
    g.fillRoundedRectangle (bounds.withHeight (6.f).withCentre ({ midX, trackY }), 3.f);

    const float amount = readAmount();
    const float dotX = bounds.getX() + amount * bounds.getWidth();

    g.setColour (accent.withAlpha (0.35f));
    g.fillRoundedRectangle (bounds.getX(), trackY - 3.f, dotX - bounds.getX(), 6.f, 3.f);

    g.setColour (accent);
    g.fillEllipse (dotX - 5.f, trackY - 5.f, 10.f, 10.f);

    if (std::abs (liveOffset) > 0.01f)
    {
        const float liveX = bounds.getX() + juce::jlimit (0.f, 1.f, amount + liveOffset * 0.25f) * bounds.getWidth();
        g.setColour (accent.brighter (0.4f));
        g.fillEllipse (liveX - 3.f, trackY - 3.f, 6.f, 6.f);
    }

    g.setFont (AviatorTokens::hud (8.f));
    g.setColour (AviatorTokens::textMuted());
    g.drawText (juce::String (juce::roundToInt (amount * 100.f)) + "%",
                bounds.withY (bounds.getBottom() - 10.f).withHeight (10.f).toNearestInt(),
                juce::Justification::centred);
}

void ModAmountSlider::setAmountFromX (int x)
{
    const auto bounds = getLocalBounds().reduced (2, 6);
    const float norm = juce::jlimit (0.f, 1.f, (float) (x - bounds.getX()) / (float) juce::jmax (1, bounds.getWidth()));

    if (auto* p = apvtsRef.getParameter (paramId))
        p->setValueNotifyingHost (norm);
}

void ModAmountSlider::mouseDown (const juce::MouseEvent& e)
{
    setAmountFromX (e.x);
}

void ModAmountSlider::mouseDrag (const juce::MouseEvent& e)
{
    setAmountFromX (e.x);
}
