#include "PluginEditor.h"

// =============================================================================
//  AviatorKeyzEditor
// =============================================================================

AviatorKeyzEditor::AviatorKeyzEditor (AviatorKeyzProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    // Resizable with enforced aspect-preserving limits
    setResizable (true, true);
    setResizeLimits (kMinWidth, kMinHeight, kMaxWidth, kMaxHeight);
    setSize (kDefaultWidth, kDefaultHeight);

    // M4: Initialize LuxuryLookAndFeel, add MainPanel, wire APVTS attachments
}

AviatorKeyzEditor::~AviatorKeyzEditor()
{
    // M4: setLookAndFeel(nullptr) before lookAndFeel is destroyed
}

// =============================================================================
//  Paint — M0 placeholder
//  Full LuxuryLookAndFeel rendering replaces this in M4.
// =============================================================================

void AviatorKeyzEditor::paint (juce::Graphics& g)
{
    // Dark background — matches reference UI gold/dark palette
    g.fillAll (juce::Colour (0xff111111));

    // Subtle radial vignette
    const auto bounds = getLocalBounds().toFloat();
    juce::ColourGradient vignette (
        juce::Colours::transparentBlack,
        bounds.getCentreX(), bounds.getCentreY(),
        juce::Colour (0x88000000),
        0.0f, 0.0f,
        true /* radial */);
    g.setGradientFill (vignette);
    g.fillRect (bounds);

    // Gold accent bar at top
    const float barH = 4.0f;
    g.setColour (juce::Colour (0xffc8922a));
    g.fillRect (0.0f, 0.0f, bounds.getWidth(), barH);

    // Wordmark — placeholder until full panel is in place
    g.setColour (juce::Colour (0xffc8922a));
    g.setFont (juce::Font ("Arial", 28.0f, juce::Font::bold));
    g.drawText ("AVIATORKEYZ",
                getLocalBounds(),
                juce::Justification::centred,
                false);

    // Build state indicator (debug only)
#if AVIATORKEYZ_DEBUG
    g.setColour (juce::Colour (0xff555555));
    g.setFont (11.0f);
    g.drawText ("M0 Scaffold  |  Debug Build",
                getLocalBounds().removeFromBottom (24).reduced (8, 0),
                juce::Justification::centredRight,
                false);
#endif
}

void AviatorKeyzEditor::resized()
{
    // M4: trigger MainPanel layout pass
    // mainPanel->setBounds(getLocalBounds());
}
