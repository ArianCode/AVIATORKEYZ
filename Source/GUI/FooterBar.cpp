#include "FooterBar.h"
#include "AviatorTokens.h"

FooterBar::FooterBar()
{
    setOpaque (false);
    addAndMakeVisible (statusLabel);
    addAndMakeVisible (metaLabel);
    addAndMakeVisible (hostLabel);
    addAndMakeVisible (settingsButton);
    addAndMakeVisible (aboutButton);
    addAndMakeVisible (versionLabel);

    statusLabel.setFont (AviatorTokens::hud (9.f));
    statusLabel.setColour (juce::Label::textColourId, AviatorTokens::instrumentCyan());
    statusLabel.setText ("Active", juce::dontSendNotification);

    metaLabel.setFont (AviatorTokens::hud (9.f));
    metaLabel.setColour (juce::Label::textColourId, AviatorTokens::textMuted());

    hostLabel.setFont (AviatorTokens::hud (9.f));
    hostLabel.setColour (juce::Label::textColourId, AviatorTokens::textMuted());
    hostLabel.setJustificationType (juce::Justification::centred);

    versionLabel.setFont (AviatorTokens::hud (9.f));
    versionLabel.setColour (juce::Label::textColourId, AviatorTokens::textMuted());
    versionLabel.setText ("v1.0.0", juce::dontSendNotification);
    versionLabel.setJustificationType (juce::Justification::centredRight);

    for (auto* b : { &settingsButton, &aboutButton })
    {
        b->setColour (juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
        b->setColour (juce::TextButton::textColourOffId, AviatorTokens::textMuted());
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

void FooterBar::setActiveStatus (const juce::String& status)
{
    statusLabel.setText (status, juce::dontSendNotification);
}

void FooterBar::setHostDescription (const juce::String& host)
{
    hostDescription = host;
    hostLabel.setText (host, juce::dontSendNotification);
}

void FooterBar::refreshMetaLabel()
{
    const auto kHz = lastSampleRate >= 1000.0 ? lastSampleRate / 1000.0 : 44.1;
    metaLabel.setText (juce::String (kHz, (kHz == (int) kHz ? 0 : 1)) + " kHz  |  "
                       + juce::String (blockSize) + " spl",
                       juce::dontSendNotification);
}

void FooterBar::paint (juce::Graphics& g)
{
    const float sc = AviatorTokens::scaleFor (*this);
    const auto b = getLocalBounds().toFloat();

    g.setColour (juce::Colour (0xcc030309));
    g.fillRect (b);
    g.setColour (AviatorTokens::instrumentCyan().withAlpha (0.35f));
    g.drawHorizontalLine (0, 0.f, b.getWidth());

    g.setColour (AviatorTokens::instrumentCyan());
    g.fillEllipse (10.f * sc, b.getCentreY() - 3.f * sc, 6.f * sc, 6.f * sc);
}

void FooterBar::resized()
{
    const float sc = AviatorTokens::scaleFor (*this);
    const int margin = AviatorTokens::scaledFor (*this, 12);

    statusLabel.setFont (AviatorTokens::hud (9.f * sc));
    metaLabel.setFont (AviatorTokens::hud (9.f * sc));
    hostLabel.setFont (AviatorTokens::hud (9.f * sc));
    versionLabel.setFont (AviatorTokens::hud (9.f * sc));

    auto left = getLocalBounds().reduced (margin, 0);
    statusLabel.setBounds (left.removeFromLeft (AviatorTokens::scaledFor (*this, 72)).withTrimmedLeft (AviatorTokens::scaledFor (*this, 8)));
    metaLabel.setBounds (left.removeFromLeft (AviatorTokens::scaledFor (*this, 130)));

    auto right = getLocalBounds().reduced (margin, 0);
    versionLabel.setBounds (right.removeFromRight (AviatorTokens::scaledFor (*this, 48)));
    aboutButton.setBounds (right.removeFromRight (AviatorTokens::scaledFor (*this, 48)));
    settingsButton.setBounds (right.removeFromRight (AviatorTokens::scaledFor (*this, 60)));

    hostLabel.setBounds (getLocalBounds().reduced (margin, 0));
}
