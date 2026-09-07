#pragma once

#include "CategoryTabs.h"
#include "CenterDashboard.h"
#include "CockpitBackground.h"
#include "EnvelopePanel.h"
#include "FilterPanel.h"
#include "LayerMixPanel.h"
#include "MacroDeck.h"
#include "MenuLookAndFeel.h"
#include "PresetHeader.h"
#include "PresetSelectorOverlay.h"
#include "StatusBar.h"
#include "VelocityPanel.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>
#include <memory>
#include <unordered_set>

class AviatorKeyzProcessor;

// =============================================================================
//  AviationMainView — the MAIN page at the canonical 1647 x 955 design size.
//  The editor owns the shared top header (MAIN / PERFORMANCE navigation, SAVE,
//  settings, about) and lays it over whichever page is showing; this view
//  paints everything below it. Preset navigation: category tabs, preset
//  header, and anchored dropdown.
// =============================================================================

class AviationMainView : public juce::Component,
                         private juce::AsyncUpdater
{
public:
    explicit AviationMainView (AviatorKeyzProcessor& processor);
    ~AviationMainView() override;

    void requestPresetUiRefresh() { triggerAsyncUpdate(); }

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    using PresetLoadedHandler = std::function<void (const juce::String&, const juce::String&,
                                                    const juce::String&, int)>;

    void handleAsyncUpdate() override { refreshPresetUI(); }
    void refreshPresetUI();
    void selectCategory (const juce::String& category);
    void openPresetSelector();
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
    PresetHeader presetHeader;
    CategoryTabs categoryTabs;
    std::unique_ptr<CenterDashboard> dashboard;
    std::unique_ptr<VelocityPanel> velocityPanel;
    std::unique_ptr<LayerMixPanel> layerMixPanel;
    std::unique_ptr<FilterPanel> filterPanel;
    std::unique_ptr<EnvelopePanel> envelopePanel;
    std::unique_ptr<MacroDeck> macroDeck;
    StatusBar statusBar;

    std::unique_ptr<PresetSelectorOverlay> presetSelectorOverlay;

    MenuLookAndFeel menuLookAndFeel;

    PresetLoadedHandler previousPresetLoadedHandler;

    std::unordered_set<std::string> favourites;

    // A/B compare snapshots (session-scoped)
    juce::ValueTree abSnapshots[2];
    int currentAbSlot { 0 };
    bool abUsed { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AviationMainView)
};
