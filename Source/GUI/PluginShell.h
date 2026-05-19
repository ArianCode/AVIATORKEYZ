#pragma once

#include "ArtworkPanel.h"
#include "CategoryBar.h"
#include "FooterBar.h"
#include "HeaderBar.h"
#include "MacroSection.h"
#include "SecondaryPanel.h"
#include <juce_gui_basics/juce_gui_basics.h>

class AviatorKeyzProcessor;

/** Fixed 860×608 chrome — all sections laid out at design proportions. */
class PluginShell : public juce::Component
{
public:
    explicit PluginShell (AviatorKeyzProcessor& processor);

    HeaderBar&    getHeader()    { return headerBar; }
    CategoryBar&  getCategories(){ return categoryBar; }
    ArtworkPanel& getArtwork()   { return artworkPanel; }
    FooterBar&    getFooter()    { return footerBar; }

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    HeaderBar      headerBar;
    CategoryBar    categoryBar;
    ArtworkPanel   artworkPanel;
    MacroSection   macroSection;
    SecondaryPanel secondaryPanel;
    FooterBar      footerBar;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginShell)
};
