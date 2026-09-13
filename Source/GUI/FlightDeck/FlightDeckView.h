#pragma once

#include "CargoHoldZone.h"
#include "ManeuverZone.h"
#include "MfxSlotPanel.h"
#include "RollingSamplerStrip.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>

class AviatorKeyzProcessor;

// =============================================================================
//  FlightDeckView — the PERFORMANCE page ("Flight Deck"). A full 1647 x 955
//  page that replaces the MAIN page below the shared top header; the editor
//  scales the whole canvas as one unit.
//
//    ○LIVE ▁▂▃▅▂▁ live 30s ▁▂▅▃  0.00s   TO CARGO  DRAG   SRC · KEY · BPM
//    ┌ MFX · SLOT A ───────────────────────┐ ┌ MFX · SLOT B ─────────┐
//    │                                     │ │                       │
//    └─────────────────────────────────────┘ │                       │
//    ┌ MANEUVER · SAMPLE FLIP ─────────────┐ │                       │
//    └─────────────────────────────────────┘ └───────────────────────┘
//    ┌ CARGO HOLD ──────────────────────────────────────────────────┐
//    └──────────────────────────────────────────────────────────────┘
//
//  The live sampler lives entirely in the title strip: it never covers or
//  resizes a zone below it.
// =============================================================================

class FlightDeckView : public juce::Component
{
public:
    explicit FlightDeckView (AviatorKeyzProcessor& processor);
    ~FlightDeckView() override;

    /** Message thread: preset / sample identity changed. */
    void refreshPresetUI();

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    juce::Rectangle<int> titleStripBounds() const;

    AviatorKeyzProcessor& processorRef;

    std::unique_ptr<MfxSlotPanel> slotA, slotB;
    std::unique_ptr<ManeuverZone> maneuverZone;
    std::unique_ptr<CargoHoldZone> cargoZone;
    std::unique_ptr<RollingSamplerStrip> samplerStrip;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FlightDeckView)
};
