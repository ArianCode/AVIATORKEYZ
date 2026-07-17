#include "EffectCellGrid.h"
#include <array>

namespace EffectCellGrid
{
int scaledBoxW (const juce::Component& c, int areaWidth, int columns, int minWDesign)
{
    const int colGap = AviatorTokens::scaledFor (c, kColGapDesign);
    const int minW = AviatorTokens::scaledFor (c, minWDesign);
    const int computed = (areaWidth - colGap * (columns - 1)) / columns;
    return juce::jmax (minW, computed);
}

int scaledBoxH (const juce::Component& c, int heightDesign)
{
    return AviatorTokens::scaledFor (c, heightDesign);
}

void layoutRow (juce::Rectangle<int> row,
                const std::vector<juce::Component*>& tiles,
                int boxW,
                int boxH,
                int colGap)
{
    auto r = row;
    for (auto* tile : tiles)
    {
        if (tile == nullptr)
            continue;
        tile->setBounds (r.removeFromLeft (boxW).withHeight (boxH));
        r.removeFromLeft (colGap);
    }
}

void layoutGrid (juce::Rectangle<int> area,
                 const std::vector<juce::Component*>& tiles,
                 int columns,
                 int boxW,
                 int boxH,
                 int colGap,
                 int rowGap)
{
    if (columns <= 0)
        return;

    int col = 0;
    auto rowArea = area;

    for (auto* tile : tiles)
    {
        if (tile == nullptr)
            continue;

        if (col == 0)
            rowArea = area.removeFromTop (boxH);

        const int x = rowArea.getX() + col * (boxW + colGap);
        tile->setBounds (x, rowArea.getY(), boxW, boxH);

        if (++col >= columns)
        {
            col = 0;
            area.removeFromTop (rowGap);
        }
    }
}

std::array<juce::Rectangle<int>, 9> layout3x3 (juce::Rectangle<int> area, int gapPx)
{
    std::array<juce::Rectangle<int>, 9> cells {};

    const int cellW = (area.getWidth() - gapPx * 2) / 3;
    const int cellH = (area.getHeight() - gapPx * 2) / 3;

    int idx = 0;
    for (int row = 0; row < 3; ++row)
    {
        for (int col = 0; col < 3; ++col)
        {
            const int x = area.getX() + col * (cellW + gapPx);
            const int y = area.getY() + row * (cellH + gapPx);
            cells[(size_t) idx++] = juce::Rectangle<int> (x, y, cellW, cellH);
        }
    }

    return cells;
}
} // namespace EffectCellGrid
