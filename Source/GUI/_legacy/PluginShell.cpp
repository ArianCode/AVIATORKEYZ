#include "PluginShell.h"
#include "../PluginProcessor.h"
#include "DesignTokens.h"

PluginShell::PluginShell (AviatorKeyzProcessor& processor)
    : macroSection (processor)
    , secondaryPanel (processor)
{
    setOpaque (true);

    juce::Component* children[] = { &headerBar, &categoryBar, &artworkPanel,
                                    &macroSection, &secondaryPanel, &footerBar };

    for (auto* c : children)
    {
        c->setOpaque (true);
        addAndMakeVisible (c);
    }
}

void PluginShell::paint (juce::Graphics& g)
{
    g.fillAll (DesignTokens::deep());

    DesignTokens::drawChampagneTrim (g,
                                     { 0.f, 0.f, (float) getWidth(), 1.f },
                                     true);
}

void PluginShell::resized()
{
    auto area = getLocalBounds();

    if (area.getHeight() <= 0)
        return;

    const float s = DesignTokens::scaleFactorFor (*this);

    area.removeFromTop (DesignTokens::scaled (DesignTokens::kTopTrimH, s));
    headerBar.setBounds (area.removeFromTop (DesignTokens::scaled (DesignTokens::kHeaderH, s)));
    categoryBar.setBounds (area.removeFromTop (DesignTokens::scaled (DesignTokens::kCategoryBarH, s)));
    artworkPanel.setBounds (area.removeFromTop (DesignTokens::scaled (DesignTokens::kArtworkH, s)));
    area.removeFromTop (DesignTokens::scaled (DesignTokens::kPanelTrimH, s));
    macroSection.setBounds (area.removeFromTop (DesignTokens::scaled (DesignTokens::kMacroSectionH, s)));
    secondaryPanel.setBounds (area.removeFromTop (DesignTokens::scaled (DesignTokens::kSecondaryH, s)));
    footerBar.setBounds (area.removeFromTop (DesignTokens::scaled (DesignTokens::kFooterH, s)));
}
