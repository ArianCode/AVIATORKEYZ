#include "PresetLibraryOverlay.h"
#include "../../PluginProcessor.h"

namespace
{
constexpr int kPanelDesignW = 340;
constexpr int kPanelDesignH = 220;
} // namespace

PresetLibraryOverlay::PresetLibraryOverlay (AviatorKeyzProcessor& processor)
    : processorRef (processor)
{
    setVisible (false);
    setInterceptsMouseClicks (true, true);

    browser = std::make_unique<PresetBrowser> (processor, [this] { dismiss(); });
    addAndMakeVisible (*browser);
}

void PresetLibraryOverlay::showOverlay()
{
    if (browser != nullptr)
        browser->refreshPresetList();
    setVisible (true);
    toFront (true);
}

void PresetLibraryOverlay::dismiss()
{
    setVisible (false);
    if (onDismiss)
        onDismiss();
}

juce::Rectangle<int> PresetLibraryOverlay::getPanelBounds() const
{
    const int panelW = juce::jmin (AviatorTokens::scaledFor (*this, kPanelDesignW), getWidth() - 40);
    const int panelH = juce::jmin (AviatorTokens::scaledFor (*this, kPanelDesignH), getHeight() - 60);
    return { getWidth() / 2 - panelW / 2,
             getHeight() / 2 - panelH / 2,
             panelW,
             panelH };
}

void PresetLibraryOverlay::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0x66000000));

    const auto panel = getPanelBounds().toFloat();
    g.setColour (juce::Colour (0xe60a1628));
    g.fillRoundedRectangle (panel, 10.f);
    g.setColour (AviatorTokens::instrumentCyan().withAlpha (0.35f));
    g.drawRoundedRectangle (panel, 10.f, 1.2f);

    g.setFont (AviatorTokens::hudBold (11.f));
    g.setColour (AviatorTokens::champagneGold());
    g.drawText ("LIBRARY", getPanelBounds().removeFromTop (22), juce::Justification::centred);
}

void PresetLibraryOverlay::resized()
{
    if (browser != nullptr)
        browser->setBounds (getPanelBounds().reduced (12).withTrimmedTop (22));
}

void PresetLibraryOverlay::mouseDown (const juce::MouseEvent& e)
{
    if (! getPanelBounds().contains (e.getPosition()))
        dismiss();
}
