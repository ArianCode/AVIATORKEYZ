#pragma once

#include "../AviatorTokens.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>

namespace CockpitLayout
{

inline float scale (const juce::Component& c) noexcept
{
    return AviatorTokens::scaleFor (c);
}

inline int s (const juce::Component& c, int designPx) noexcept
{
    return juce::roundToInt ((float) designPx * scale (c));
}

inline juce::Rectangle<int> contentArea (juce::Rectangle<int> panelBounds,
                                          int titleH,
                                          int pad) noexcept
{
    return panelBounds.reduced (pad).withTrimmedTop (titleH);
}

/** Place components in a uniform grid; returns unused area. */
inline void layoutGrid (juce::Rectangle<int> area,
                        const std::vector<juce::Component*>& items,
                        int columns,
                        int cellW,
                        int cellH,
                        int gapX,
                        int gapY)
{
    if (columns < 1 || items.empty())
        return;

    int col = 0;
    int x0 = area.getX();
    int y  = area.getY();

    for (auto* comp : items)
    {
        if (comp == nullptr)
            continue;

        const int x = x0 + col * (cellW + gapX);
        comp->setBounds (x, y, cellW, cellH);

        if (++col >= columns)
        {
            col = 0;
            x0 = area.getX();
            y += cellH + gapY;
        }
    }
}

inline void layoutRow (juce::Rectangle<int>& area,
                       const std::vector<juce::Component*>& items,
                       int gap)
{
    if (items.empty())
        return;

    const int totalGap = gap * (int) (items.size() - 1);
    const int w = juce::jmax (1, (area.getWidth() - totalGap) / (int) items.size());

    for (auto* comp : items)
    {
        if (comp == nullptr)
            continue;

        comp->setBounds (area.removeFromLeft (w));
        if (area.getWidth() > 0)
            area.removeFromLeft (gap);
    }
}

} // namespace CockpitLayout
