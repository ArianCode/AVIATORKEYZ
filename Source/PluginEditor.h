#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "GUI/Aviation/AviationMainView.h"
#include "GUI/Aviation/TopHeader.h"
#include "GUI/Cockpit/AboutOverlay.h"
#include "GUI/Cockpit/PresetLibraryOverlay.h"
#include "GUI/FlightDeck/FlightDeckView.h"
#include "PluginProcessor.h"

// =============================================================================
//  AviatorKeyzEditor — hosts one fixed 1647 x 955 design canvas, scaled
//  proportionally as a single unit. The canvas holds the shared top header
//  (MAIN / PERFORMANCE navigation, SAVE, settings, about) and two full pages
//  underneath it: the Aviation MAIN view and the PERFORMANCE Flight Deck.
//  Exactly one page is visible at a time; the header switches between them.
// =============================================================================

class AviatorKeyzEditor final : public juce::AudioProcessorEditor
{
public:
    explicit AviatorKeyzEditor (AviatorKeyzProcessor& processor);
    ~AviatorKeyzEditor() override;

    void paint (juce::Graphics& g) override;
    void resized() override;
    void visibilityChanged() override;

private:
    void layoutContent();
    void setPerformanceView (bool performance);
    void openLibraryOverlay();
    void openAboutOverlay();

    AviatorKeyzProcessor& processorRef;

    // Default opens at 1100 x 638, preserving the 1647:955 design aspect ratio.
    static constexpr int kDefaultWidth = 1100;
    static constexpr int kMinWidth  = 824;
    static constexpr int kMaxWidth  = 1976;

    // Design-space root: everything below is laid out in 1647 x 955 pixels and
    // this one component carries the scale transform.
    juce::Component canvas;

    std::unique_ptr<AviationMainView> mainView;
    std::unique_ptr<FlightDeckView> flightDeck;
    TopHeader header;
    std::unique_ptr<PresetLibraryOverlay> libraryOverlay;
    std::unique_ptr<AboutOverlay> aboutOverlay;
    bool performanceView { false };

    using PresetLoadedHandler = std::function<void (const juce::String& category,
                                                    const juce::String& name,
                                                    const juce::String& sampleId,
                                                    int rootNote)>;

    PresetLoadedHandler previousPresetLoadedHandler;
    std::function<void()> previousUserSampleHandler;

    // Renders native hover tooltips for any child component with setTooltip() text.
    juce::TooltipWindow tooltipWindow { this, 500 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AviatorKeyzEditor)
};
