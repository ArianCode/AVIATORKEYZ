#pragma once

#include "../AviatorTokens.h"
#include "../PrecisionKnob.h"
#include "../Cockpit/CockpitLayout.h"
#include <memory>
#include <vector>

namespace AdvancedKnobHelpers
{
inline void layoutKnobs (juce::Component& host,
                         const std::vector<std::unique_ptr<PrecisionKnob>>& knobs,
                         int columns)
{
    const int pad = AviatorTokens::scaledFor (host, AviatorTokens::kPanelPad);
    const int cellW = AviatorTokens::scaledFor (host, AviatorTokens::kKnobCellW);
    const int cellH = AviatorTokens::scaledFor (host, AviatorTokens::kKnobCellH);
    const int gapX = AviatorTokens::scaledFor (host, AviatorTokens::kGridGapX);
    const int gapY = AviatorTokens::scaledFor (host, AviatorTokens::kGridGapY);

    auto area = host.getLocalBounds().reduced (pad);
    std::vector<juce::Component*> ptrs;
    ptrs.reserve (knobs.size());
    for (const auto& k : knobs)
        ptrs.push_back (k.get());
    CockpitLayout::layoutGrid (area, ptrs, columns, cellW, cellH, gapX, gapY);
}

inline std::unique_ptr<PrecisionKnob> makeKnob (juce::AudioProcessorValueTreeState& apvts,
                                              const char* id,
                                              const juce::String& name,
                                              const juce::String& sub,
                                              PrecisionKnob::ValueFormat fmt,
                                              juce::Component& parent,
                                              bool modAssign = true)
{
    auto k = std::make_unique<PrecisionKnob> (apvts, id, name, sub, fmt, true, modAssign);
    parent.addAndMakeVisible (*k);
    return k;
}

inline void paintPageBackground (juce::Graphics& g, juce::Component& c)
{
    AviatorTokens::paintGlassPanel (g, c.getLocalBounds().toFloat(), false);
}
} // namespace AdvancedKnobHelpers
