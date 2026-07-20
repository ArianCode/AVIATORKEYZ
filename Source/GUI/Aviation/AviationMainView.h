#pragma once

#include "CategoryTabs.h"
#include "CenterDashboard.h"
#include "CockpitBackground.h"
#include "EnvelopePanel.h"
#include "FilterPanel.h"
#include "LayerMixPanel.h"
#include "MacroDeck.h"
#include "PresetBrowser.h"
#include "PresetHeader.h"
#include "SourceDropdown.h"
#include "StatusBar.h"
#include "TopHeader.h"
#include "VelocityPanel.h"
#include "../Cockpit/AboutOverlay.h"
#include "../Cockpit/PresetLibraryOverlay.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>
#include <memory>
#include <unordered_set>

class AviatorKeyzProcessor;

// =============================================================================
//  AviationMainView — the complete MAIN interface at the canonical
//  1647 x 955 design size. The editor scales this view as one unit.
// =============================================================================

class AviationMainView : public juce::Component,
                         private juce::AsyncUpdater
{
public:
    explicit AviationMainView (AviatorKeyzProcessor& processor);
    ~AviationMainView() override;

    /** Forwarded to the embedded top header (editor switches views). */
    std::function<void (bool performance)> onModeChanged;

    TopHeader& getTopHeader() noexcept { return topHeader; }

    void requestPresetUiRefresh() { triggerAsyncUpdate(); }

    void paint (juce::Graphics& g) override;
    void paintOverChildren (juce::Graphics& g) override;
    void resized() override;

private:
    using PresetLoadedHandler = std::function<void (const juce::String&, const juce::String&,
                                                    const juce::String&, int)>;

    void handleAsyncUpdate() override { refreshPresetUI(); }
    void refreshPresetUI();
    void selectCategory (const juce::String& category);
    void openSourceMenu();
    void openAbMenu();

    // favorites (persisted per user)
    juce::File favouritesFile() const;
    void loadFavourites();
    void saveFavourites() const;
    juce::String favouriteKey (const juce::String& cat, const juce::String& name) const;
    bool isFavourited (const juce::String& cat, const juce::String& name) const;
    void setFavourited (const juce::String& cat, const juce::String& name, bool fav);

    AviatorKeyzProcessor& processor;

    CockpitBackground cockpitBackground;
    TopHeader topHeader;
    PresetHeader presetHeader;
    CategoryTabs categoryTabs;
    PresetBrowserPanel presetBrowser;
    SourceDropdown sourceDropdown;
    std::unique_ptr<CenterDashboard> dashboard;
    std::unique_ptr<VelocityPanel> velocityPanel;
    std::unique_ptr<LayerMixPanel> layerMixPanel;
    std::unique_ptr<FilterPanel> filterPanel;
    std::unique_ptr<EnvelopePanel> envelopePanel;
    std::unique_ptr<MacroDeck> macroDeck;
    StatusBar statusBar;

    std::unique_ptr<PresetLibraryOverlay> libraryOverlay;
    std::unique_ptr<AboutOverlay> aboutOverlay;

    PresetLoadedHandler previousPresetLoadedHandler;

    std::unordered_set<std::string> favourites;

    // A/B compare snapshots (session-scoped)
    juce::ValueTree abSnapshots[2];
    int currentAbSlot { 0 };
    bool abUsed { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AviationMainView)
};
