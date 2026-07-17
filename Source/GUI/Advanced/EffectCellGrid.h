#pragma once

#include "../AviatorTokens.h"
#include <array>
#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>

namespace EffectCellGrid
{
    constexpr int kBoxHDesign    = 78;
    constexpr int kBoxMinWDesign = 76;
    constexpr int kColGapDesign  = 8;
    constexpr int kRowGapDesign  = 8;

    int scaledBoxW (const juce::Component& c, int areaWidth, int columns, int minWDesign = kBoxMinWDesign);
    int scaledBoxH (const juce::Component& c, int heightDesign = kBoxHDesign);

    void layoutRow (juce::Rectangle<int> row,
                    const std::vector<juce::Component*>& tiles,
                    int boxW,
                    int boxH,
                    int colGap);

    void layoutGrid (juce::Rectangle<int> area,
                     const std::vector<juce::Component*>& tiles,
                     int columns,
                     int boxW,
                     int boxH,
                     int colGap,
                     int rowGap);

    std::array<juce::Rectangle<int>, 9> layout3x3 (juce::Rectangle<int> area, int gapPx);
}
