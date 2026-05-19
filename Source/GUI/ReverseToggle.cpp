#include "ReverseToggle.h"
#include "DesignTokens.h"

ReverseToggle::ReverseToggle (juce::AudioProcessorValueTreeState& apvts, const juce::String& paramID)
{
    setOpaque (false);
    button.setClickingTogglesState (true);
    button.onClick = [this] { syncState(); };
    addChildComponent (button);

    nameLabel.setText ("Reverse", juce::dontSendNotification);
    nameLabel.setJustificationType (juce::Justification::centred);
    nameLabel.setFont (DesignTokens::labelFont (8.f, juce::Font::bold));
    nameLabel.setColour (juce::Label::textColourId, DesignTokens::textSecondary());
    addAndMakeVisible (nameLabel);

    subLabel.setText ("Playback Phase", juce::dontSendNotification);
    subLabel.setJustificationType (juce::Justification::centred);
    subLabel.setFont (DesignTokens::labelFont (6.f));
    subLabel.setColour (juce::Label::textColourId, DesignTokens::textFaint());
    addAndMakeVisible (subLabel);

    stateLabel.setJustificationType (juce::Justification::centred);
    stateLabel.setFont (DesignTokens::labelFont (6.f, juce::Font::bold));
    addAndMakeVisible (stateLabel);

    valueLabel.setJustificationType (juce::Justification::centred);
    valueLabel.setFont (DesignTokens::monoFont (8.f));
    valueLabel.setColour (juce::Label::textColourId, DesignTokens::textMuted());
    addAndMakeVisible (valueLabel);

    attachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        apvts, paramID, button);

    syncState();
}

void ReverseToggle::syncState()
{
    const bool on = button.getToggleState();
    stateLabel.setText (on ? "On" : "Off", juce::dontSendNotification);
    stateLabel.setColour (juce::Label::textColourId,
                          on ? DesignTokens::champagneMid() : DesignTokens::textFaint());
    valueLabel.setText (on ? "Active" : "", juce::dontSendNotification);
    repaint();
}

void ReverseToggle::resized()
{
    const float s = DesignTokens::scaleFactorFor (*this);

    nameLabel.setFont (DesignTokens::labelFont (8.f * s, juce::Font::bold));
    subLabel.setFont (DesignTokens::labelFont (6.f * s));
    stateLabel.setFont (DesignTokens::labelFont (6.f * s, juce::Font::bold));
    valueLabel.setFont (DesignTokens::monoFont (8.f * s));

    auto area = getLocalBounds();
    valueLabel.setBounds (area.removeFromBottom (DesignTokens::scaled (11, s)));
    subLabel.setBounds (area.removeFromBottom (DesignTokens::scaled (10, s)));
    nameLabel.setBounds (area.removeFromTop (DesignTokens::scaled (12, s)));

    auto btnArea = area.withSizeKeepingCentre (DesignTokens::scaled (60, s), DesignTokens::scaled (38, s));
    button.setBounds (btnArea);
}

void ReverseToggle::paint (juce::Graphics& g)
{
    const float s = DesignTokens::scaleFactorFor (*this);
    const auto bounds = button.getBounds().toFloat();
    const bool on = button.getToggleState();
    const float corner = 8.f * s;

    juce::ColourGradient bg (DesignTokens::raised(), bounds.getX(), bounds.getY(),
                             juce::Colour (0xff0f0d1a), bounds.getX(), bounds.getBottom(), false);
    if (on)
    {
        bg = juce::ColourGradient (DesignTokens::champagne().withAlpha (0.12f),
                                    bounds.getX(), bounds.getY(),
                                    DesignTokens::champagne().withAlpha (0.05f),
                                    bounds.getX(), bounds.getBottom(), false);
    }
    g.setGradientFill (bg);
    g.fillRoundedRectangle (bounds, corner);

    g.setColour (on ? DesignTokens::champagne().withAlpha (0.28f) : DesignTokens::border2());
    g.drawRoundedRectangle (bounds, corner, 1.f * s);

    auto shineArea = bounds;
    g.setColour (juce::Colours::white.withAlpha (0.025f));
    g.fillRoundedRectangle (shineArea.removeFromTop (bounds.getHeight() * 0.4f), corner - 1.f);

    const float cx = button.getBounds().getCentreX();
    const float cy = button.getBounds().getCentreY() - 4.f * s;
    const auto arrowCol = on ? DesignTokens::champagne() : DesignTokens::textFaint();

    juce::Path arrow;
    arrow.startNewSubPath (cx - 4.f * s, cy);
    arrow.lineTo (cx + 4.f * s, cy);
    arrow.lineTo (cx + 1.f * s, cy - 3.f * s);
    arrow.closeSubPath();
    arrow.lineTo (cx + 1.f * s, cy + 3.f * s);
    arrow.lineTo (cx + 4.f * s, cy);
    g.setColour (arrowCol);
    g.strokePath (arrow, juce::PathStrokeType (1.15f * s));

    juce::Path arc;
    arc.addCentredArc (cx + 4.f * s, cy, 4.f * s, 4.f * s, 0.f,
                       juce::MathConstants<float>::pi * 0.5f,
                       juce::MathConstants<float>::pi * 1.75f, true);
    g.strokePath (arc, juce::PathStrokeType (1.15f * s));
}

void ReverseToggle::mouseUp (const juce::MouseEvent& e)
{
    if (button.getBounds().contains (e.getPosition()))
        button.triggerClick();
}
