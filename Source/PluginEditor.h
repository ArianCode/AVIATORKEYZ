#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

// =============================================================================
//  AviatorKeyzEditor — root AudioProcessorEditor
//
//  Responsibilities:
//    - Own and lay out GUI components
//    - Enforce resize limits
//    - Bridge APVTS ↔ component attachments
//
//  Architecture rules:
//    - Must NEVER call DSP methods directly — all state access goes through APVTS
//    - Component attachments (Slider, Button, etc.) are declared here and
//      initialized in the constructor using APVTS helpers
//    - All paint/layout work happens on the message thread only
// =============================================================================

class AviatorKeyzEditor final : public juce::AudioProcessorEditor
{
public:
    explicit AviatorKeyzEditor (AviatorKeyzProcessor& processor);
    ~AviatorKeyzEditor() override;

    void paint   (juce::Graphics&) override;
    void resized () override;

private:
    AviatorKeyzProcessor& processorRef;

    // -------------------------------------------------------------------------
    // Editor size constraints
    // The editor is resizable with a fixed aspect ratio (≈ 900:520 ≈ 1.73:1).
    // Minimum enforced so knobs remain legible at small sizes.
    // -------------------------------------------------------------------------
    static constexpr int kDefaultWidth  = 900;
    static constexpr int kDefaultHeight = 520;
    static constexpr int kMinWidth      = 700;
    static constexpr int kMinHeight     = 404;  // maintains ~1.73 ratio
    static constexpr int kMaxWidth      = 1800;
    static constexpr int kMaxHeight     = 1040;

    // -------------------------------------------------------------------------
    // GUI sub-components (added in M4 — stubs declared here for structure)
    // -------------------------------------------------------------------------
    // std::unique_ptr<LuxuryLookAndFeel>  lookAndFeel;
    // std::unique_ptr<MainPanel>          mainPanel;
    // std::unique_ptr<PresetBrowser>      presetBrowser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AviatorKeyzEditor)
};
