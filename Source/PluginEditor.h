#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "GUI/FlightDeck/FlightDeckView.h"
#include "GUI/Aviation/AviationMainView.h"
#include "PluginProcessor.h"

// =============================================================================
//  AviatorKeyzEditor — hosts the Aviation MAIN interface (fixed 1647 x 955
//  design space, scaled proportionally as one unit) and the PERFORMANCE view
//  (Flight Deck, fixed 1366 x 860, scaled to fit below the header), switched
//  from the main view's top header.
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

    AviatorKeyzProcessor& processorRef;

    // Default opens at 1100 x 638, preserving the 1647:955 design aspect ratio.
    static constexpr int kDefaultWidth = 1100;
    static constexpr int kMinWidth  = 824;
    static constexpr int kMaxWidth  = 1976;

    std::unique_ptr<AviationMainView> mainView;
    std::unique_ptr<FlightDeckView> flightDeck;
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
