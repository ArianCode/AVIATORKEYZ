#include "HeaderBar.h"
#include "LuxuryLookAndFeel.h"

HeaderBar::HeaderBar()
{
    titleLabel.setJustificationType (juce::Justification::centredLeft);
    titleLabel.setFont (juce::Font (juce::FontOptions (20.f)).boldened());
    titleLabel.setColour (juce::Label::textColourId, LuxuryLookAndFeel::textPrimary());
    addAndMakeVisible (titleLabel);

    subtitleLabel.setJustificationType (juce::Justification::centredLeft);
    subtitleLabel.setFont (juce::Font (juce::FontOptions (13.f)));
    subtitleLabel.setColour (juce::Label::textColourId, LuxuryLookAndFeel::textDim());
    addAndMakeVisible (subtitleLabel);

    setPresetInfo ({}, "AviatorKeyz");
}

void HeaderBar::setPresetInfo (const juce::String& category, const juce::String& presetName)
{
    titleLabel.setText (presetName, juce::dontSendNotification);
    subtitleLabel.setText (category.isEmpty() ? juce::String ("Factory / User presets")
                                               : ("Category: " + category),
                          juce::dontSendNotification);
}

void HeaderBar::paint (juce::Graphics& g)
{
    g.setColour (LuxuryLookAndFeel::surfaceColour());
    g.fillRoundedRectangle (getLocalBounds().toFloat().reduced (0.f, 4.f), 6.f);
    g.setColour (LuxuryLookAndFeel::goldPrimary().withAlpha (0.9f));
    g.fillRect (0.f, 0.f, (float) getWidth(), 3.f);
}

void HeaderBar::resized()
{
    auto r = getLocalBounds().reduced (12, 8);
    titleLabel.setBounds (r.removeFromTop (26));
    subtitleLabel.setBounds (r.removeFromTop (18));
}
