#include "InstrumentPanelBar.h"
#include "../AviatorTokens.h"

void InstrumentPanelBar::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    juce::ColourGradient fade (juce::Colour (0x00060d1a), bounds.getX(), bounds.getY(),
                               juce::Colour (0x99060d1a), bounds.getX(), bounds.getBottom(), false);
    g.setGradientFill (fade);
    g.fillRect (bounds);

    g.setColour (juce::Colour (0x99060d1a));
    g.fillRect (bounds.withTrimmedTop (bounds.getHeight() * 0.08f));

    g.setColour (AviatorTokens::instrumentCyan().withAlpha (0.35f));
    g.drawHorizontalLine (0, bounds.getX(), bounds.getRight());
}

void InstrumentPanelBar::resized()
{
    auto area = getLocalBounds().reduced (2, 4);
    const int n = (int) gauges.size();
    if (n == 0)
        return;

    const int gap = 2;
    const int cellW = (area.getWidth() - gap * (n - 1)) / n;
    int x = area.getX();
    for (auto& gauge : gauges)
    {
        if (gauge != nullptr)
            gauge->setBounds (x, area.getY(), cellW, area.getHeight());
        x += cellW + gap;
    }
}
