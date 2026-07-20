#pragma once

#include "CockpitPhotoBackground.h"
#include "CockpitPresetControlBar.h"
#include "CockpitTopPresetBar.h"
#include "CockpitZones.h"
#include "InstrumentPanelBar.h"
#include "PhotoAnchoredKnob.h"
#include "PresetCenterNavigator.h"
#include "AboutOverlay.h"
#include "PresetLibraryOverlay.h"
#include "PresetSearchOverlay.h"
#include "../Advanced/AdvancedPresetSidebar.h"
#include "../FooterBar.h"
#include "../PrecisionKnob.h"
#include "../ReverseToggle.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>
#include <memory>
#include <unordered_set>
#include <vector>

class AviatorKeyzProcessor;

/** Main cockpit: preset sidebar, full-bleed photo, centered navigator, bottom gauges. */
class CockpitCrossworldPanel : public juce::Component,
                               private juce::AsyncUpdater
{
public:
    static constexpr int kTopPresetBarH = 52;
    static constexpr int kPresetControlBarH = 34;
    static constexpr int kSidebarDesignW = 280;

    explicit CockpitCrossworldPanel (AviatorKeyzProcessor& processor);
    ~CockpitCrossworldPanel() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

    void requestPresetUiRefresh();
    void refreshPresetUI();

private:
    void handleAsyncUpdate() override;
    void refreshPresetUIImpl();
    void openLibraryOverlay();
    void openAboutOverlay();
    void buildInstrumentGauges();
    void buildPhotoAnchors();
    void buildTopPresetBar();
    void selectCategory (const juce::String& category);
    void layoutZones();
    void openSearchOverlay();
    juce::String favouriteKey (const juce::String& cat, const juce::String& name) const;
    bool isFavourited (const juce::String& cat, const juce::String& name) const;
    void setFavourited (const juce::String& cat, const juce::String& name, bool fav);
    void saveCurrentPreset();

    AviatorKeyzProcessor& processor;
    CockpitPhotoBackground photoBackground;
    CockpitTopPresetBar topPresetBar;
    CockpitPresetControlBar presetControlBar;
    PresetCenterNavigator presetNavigator;
    std::unique_ptr<AdvancedPresetSidebar> presetSidebar;
    std::unique_ptr<PresetSearchOverlay> searchOverlay;
    std::unique_ptr<PresetLibraryOverlay> libraryOverlay;
    std::unique_ptr<AboutOverlay> aboutOverlay;

    std::unique_ptr<InstrumentPanelBar> instrumentBar;
    FooterBar footer;

    std::vector<AviatorCockpit::KnobAnchor> photoAnchorSpecs;
    std::vector<std::unique_ptr<juce::Component>> photoControls;

    juce::String activeCategory;
    bool topBarCategoriesBuilt { false };
    std::unordered_set<std::string> favourites;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CockpitCrossworldPanel)
};
