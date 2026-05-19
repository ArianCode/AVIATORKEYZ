#include "HeaderBar.h"
#include "DesignTokens.h"

HeaderBar::HeaderBar()
{
    setOpaque (true);
    addAndMakeVisible (presetNameLabel);
    addAndMakeVisible (presetCountLabel);
    addAndMakeVisible (prevButton);
    addAndMakeVisible (nextButton);
    addAndMakeVisible (libraryButton);

    presetNameLabel.setJustificationType (juce::Justification::centred);
    presetNameLabel.setFont (DesignTokens::labelFont (9.f, juce::Font::bold));
    presetNameLabel.setColour (juce::Label::textColourId, DesignTokens::textPrimary());
    presetNameLabel.setText ("INIT", juce::dontSendNotification);

    presetCountLabel.setJustificationType (juce::Justification::centredRight);
    presetCountLabel.setFont (DesignTokens::labelFont (8.f));
    presetCountLabel.setColour (juce::Label::textColourId, DesignTokens::textFaint());

    for (auto* b : { &prevButton, &nextButton })
    {
        b->setColour (juce::TextButton::buttonColourId, DesignTokens::raised());
        b->setColour (juce::TextButton::textColourOffId, DesignTokens::textMuted());
    }

    libraryButton.setColour (juce::TextButton::buttonColourId, DesignTokens::raised());
    libraryButton.setColour (juce::TextButton::textColourOffId, DesignTokens::textSecondary());

    prevButton.onClick = [this] {
        if (onPreviousPreset) onPreviousPreset();
    };
    nextButton.onClick = [this] {
        if (onNextPreset) onNextPreset();
    };
    libraryButton.onClick = [this] {
        if (onLibraryClicked) onLibraryClicked();
    };
}

void HeaderBar::setPresetDisplayName (const juce::String& displayName)
{
    presetNameLabel.setText (displayName.toUpperCase(), juce::dontSendNotification);
}

void HeaderBar::setPresetCount (int count)
{
    presetCountLabel.setText (juce::String (count) + " Presets", juce::dontSendNotification);
}

void HeaderBar::paint (juce::Graphics& g)
{
    const float s = DesignTokens::scaleFactorFor (*this);

    juce::ColourGradient bg (juce::Colour (0xff0e0d16), 0.f, 0.f,
                             juce::Colour (0xff0c0b12), 0.f, (float) getHeight(), false);
    g.setGradientFill (bg);
    g.fillAll();

    g.setColour (DesignTokens::border1());
    g.drawHorizontalLine (getHeight() - 1, 0.f, (float) getWidth());

    g.setFont (DesignTokens::brandFont().withHeight (11.f * s));
    g.setColour (DesignTokens::textPrimary());
    g.drawText ("AVIATORKEYZ", DesignTokens::scaled (26, s), 0,
                DesignTokens::scaled (200, s), getHeight() - DesignTokens::scaled (4, s),
                juce::Justification::centredLeft);

    g.setFont (DesignTokens::labelFont (7.f * s));
    g.setColour (DesignTokens::textFaint());
    g.drawText ("INSTRUMENT", DesignTokens::scaled (26, s), getHeight() / 2,
                DesignTokens::scaled (120, s), getHeight() / 2,
                juce::Justification::centredLeft);
}

void HeaderBar::resized()
{
    const float s = DesignTokens::scaleFactorFor (*this);
    const int margin = DesignTokens::scaled (26, s);
    const int btnH = DesignTokens::scaled (24, s);

    auto right = getLocalBounds().reduced (margin, 0);
    libraryButton.setBounds (right.removeFromRight (DesignTokens::scaled (72, s)).withSizeKeepingCentre (DesignTokens::scaled (72, s), btnH));
    presetCountLabel.setBounds (right.removeFromRight (DesignTokens::scaled (80, s)));
    presetCountLabel.setFont (DesignTokens::labelFont (8.f * s));

    auto centre = getLocalBounds().withSizeKeepingCentre (DesignTokens::scaled (220, s), btnH);
    prevButton.setBounds (centre.removeFromLeft (DesignTokens::scaled (22, s)));
    nextButton.setBounds (centre.removeFromRight (DesignTokens::scaled (22, s)));
    presetNameLabel.setBounds (centre);
    presetNameLabel.setFont (DesignTokens::labelFont (9.f * s, juce::Font::bold));
}
