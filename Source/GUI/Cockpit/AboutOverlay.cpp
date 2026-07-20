#include "AboutOverlay.h"

#include <juce_audio_processors/juce_audio_processors.h>

namespace
{
constexpr int kPanelDesignW = 280;
constexpr int kPanelDesignH = 140;
} // namespace

AboutOverlay::AboutOverlay()
{
    setVisible (false);
    setInterceptsMouseClicks (true, true);

    closeButton.onClick = [this] { dismiss(); };
    closeButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff0d1f33));
    closeButton.setColour (juce::TextButton::textColourOffId, AviatorTokens::instrumentCyan());
    addAndMakeVisible (closeButton);
}

void AboutOverlay::showOverlay()
{
    setVisible (true);
    toFront (true);
}

void AboutOverlay::dismiss()
{
    setVisible (false);
    if (onDismiss)
        onDismiss();
}

juce::Rectangle<int> AboutOverlay::getPanelBounds() const
{
    const int panelW = juce::jmin (AviatorTokens::scaledFor (*this, kPanelDesignW), getWidth() - 40);
    const int panelH = juce::jmin (AviatorTokens::scaledFor (*this, kPanelDesignH), getHeight() - 60);
    return { getWidth() / 2 - panelW / 2,
             getHeight() / 2 - panelH / 2,
             panelW,
             panelH };
}

void AboutOverlay::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0x66000000));

    const auto panel = getPanelBounds().toFloat();
    g.setColour (juce::Colour (0xe60a1628));
    g.fillRoundedRectangle (panel, 10.f);
    g.setColour (AviatorTokens::instrumentCyan().withAlpha (0.35f));
    g.drawRoundedRectangle (panel, 10.f, 1.2f);

    g.setFont (AviatorTokens::hudBold (14.f));
    g.setColour (AviatorTokens::champagneGold());
    g.drawText (JucePlugin_Name, getPanelBounds().removeFromTop (36), juce::Justification::centred);

    g.setFont (AviatorTokens::hud (11.f));
    g.setColour (AviatorTokens::textPrimary());
    auto textArea = getPanelBounds().reduced (16).withTrimmedTop (32).withTrimmedBottom (36);
    g.drawFittedText ("Photo-anchored cockpit UI.\n" + juce::String (JucePlugin_Name) + " v" + juce::String (JucePlugin_VersionString),
                      textArea,
                      juce::Justification::centred,
                      3);
}

void AboutOverlay::resized()
{
    auto panel = getPanelBounds().reduced (16);
    closeButton.setBounds (panel.removeFromBottom (28).withSizeKeepingCentre (80, 26));
}

void AboutOverlay::mouseDown (const juce::MouseEvent& e)
{
    if (! getPanelBounds().contains (e.getPosition()))
        dismiss();
}
