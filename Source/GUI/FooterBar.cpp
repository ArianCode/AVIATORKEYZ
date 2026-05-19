#include "FooterBar.h"
#include "DesignTokens.h"

FooterBar::FooterBar()
{
    setOpaque (true);
    addAndMakeVisible (statusLabel);
    addAndMakeVisible (metaLabel);
    addAndMakeVisible (hudLabel);
    addAndMakeVisible (settingsButton);
    addAndMakeVisible (aboutButton);
    addAndMakeVisible (versionLabel);

    statusLabel.setFont (DesignTokens::labelFont (7.f));
    statusLabel.setColour (juce::Label::textColourId, DesignTokens::textFaint());
    statusLabel.setText ("Active", juce::dontSendNotification);

    metaLabel.setFont (DesignTokens::labelFont (7.f));
    metaLabel.setColour (juce::Label::textColourId, DesignTokens::textFaint());

    hudLabel.setFont (DesignTokens::monoFont (7.f));
    hudLabel.setColour (juce::Label::textColourId, DesignTokens::champagne().withAlpha (0.18f));

    versionLabel.setFont (DesignTokens::labelFont (7.f));
    versionLabel.setColour (juce::Label::textColourId, DesignTokens::textFaint());
    versionLabel.setText ("v 1.0.0", juce::dontSendNotification);
    versionLabel.setJustificationType (juce::Justification::centredRight);

    for (auto* b : { &settingsButton, &aboutButton })
    {
        b->setColour (juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
        b->setColour (juce::TextButton::textColourOffId, DesignTokens::textFaint());
    }

    settingsButton.onClick = [this] {
        if (onSettingsClicked)
            onSettingsClicked();
    };
    aboutButton.onClick = [this] {
        if (onAboutClicked)
            onAboutClicked();
    };

    refreshMetaLabel();
}

void FooterBar::setSampleRate (double sampleRate)
{
    lastSampleRate = sampleRate;
    refreshMetaLabel();
}

void FooterBar::setBlockSize (int newBlockSize)
{
    blockSize = newBlockSize;
    refreshMetaLabel();
}

void FooterBar::refreshMetaLabel()
{
    const auto kHz = lastSampleRate >= 1000.0 ? lastSampleRate / 1000.0 : 44.1;
    metaLabel.setText (juce::String (kHz, (kHz == (int) kHz ? 0 : 1)) + " kHz · "
                       + juce::String (blockSize) + " spl",
                       juce::dontSendNotification);
}

void FooterBar::setHudText (const juce::String& hud)
{
    hudLabel.setText (hud, juce::dontSendNotification);
}

void FooterBar::paint (juce::Graphics& g)
{
    const float s = DesignTokens::scaleFactorFor (*this);

    g.fillAll (DesignTokens::piano());
    g.setColour (DesignTokens::border0());
    g.drawHorizontalLine (0, 0.f, (float) getWidth());

    g.setColour (DesignTokens::statusGreen());
    g.fillEllipse (26.f * s, (float) getHeight() * 0.5f - 2.f * s, 4.f * s, 4.f * s);
}

void FooterBar::resized()
{
    const float s = DesignTokens::scaleFactorFor (*this);
    const int margin = DesignTokens::scaled (26, s);

    statusLabel.setFont (DesignTokens::labelFont (7.f * s));
    metaLabel.setFont (DesignTokens::labelFont (7.f * s));
    hudLabel.setFont (DesignTokens::monoFont (7.f * s));
    versionLabel.setFont (DesignTokens::labelFont (7.f * s));

    auto left = getLocalBounds().reduced (margin, 0);
    statusLabel.setBounds (left.removeFromLeft (DesignTokens::scaled (48, s)).withTrimmedLeft (DesignTokens::scaled (10, s)));
    metaLabel.setBounds (left.removeFromLeft (DesignTokens::scaled (110, s)));
    hudLabel.setBounds (left.removeFromLeft (DesignTokens::scaled (140, s)));

    auto right = getLocalBounds().reduced (margin, 0);
    versionLabel.setBounds (right.removeFromRight (DesignTokens::scaled (40, s)));
    aboutButton.setBounds (right.removeFromRight (DesignTokens::scaled (42, s)));
    settingsButton.setBounds (right.removeFromRight (DesignTokens::scaled (56, s)));
}
