#include "PresetNavButton.h"
#include "../AviatorTokens.h"

PresetNavButton::PresetNavButton (Direction dir, Style s)
    : juce::Button ({})
    , direction (dir)
    , style (s)
{
}

void PresetNavButton::paintButton (juce::Graphics& g, bool highlighted, bool down)
{
    auto bounds = getLocalBounds().toFloat().reduced (1.f);

    if (style == Style::boxed)
    {
        const auto fill = down ? AviatorTokens::instrumentCyan().withAlpha (0.45f)
                               : highlighted ? AviatorTokens::instrumentCyan().withAlpha (0.28f)
                                             : juce::Colour (0xff0d1f33);
        g.setColour (fill);
        g.fillRoundedRectangle (bounds, 4.f);
        g.setColour (AviatorTokens::instrumentCyan().withAlpha (0.7f));
        g.drawRoundedRectangle (bounds, 4.f, 1.f);
    }

    juce::Path chevron;
    const float cx = bounds.getCentreX();
    const float cy = bounds.getCentreY();
    const float w = juce::jmin (bounds.getWidth(), bounds.getHeight()) * (style == Style::plainChevron ? 0.28f : 0.22f);
    const float h = w * 1.35f;

    switch (direction)
    {
        case Direction::left:
            chevron.addTriangle (cx + w * 0.35f, cy - h, cx - w * 0.65f, cy, cx + w * 0.35f, cy + h);
            break;
        case Direction::right:
            chevron.addTriangle (cx - w * 0.35f, cy - h, cx + w * 0.65f, cy, cx - w * 0.35f, cy + h);
            break;
        case Direction::up:
            chevron.addTriangle (cx - w, cy + h * 0.35f, cx, cy - h * 0.65f, cx + w, cy + h * 0.35f);
            break;
        case Direction::down:
            chevron.addTriangle (cx - w, cy - h * 0.35f, cx, cy + h * 0.65f, cx + w, cy - h * 0.35f);
            break;
    }

    const juce::Colour chevCol = (style == Style::plainChevron && (highlighted || down))
                                     ? AviatorTokens::champagneGold()
                                     : AviatorTokens::textPrimary().withAlpha (style == Style::plainChevron ? 0.92f : 1.f);
    g.setColour (chevCol);
    g.fillPath (chevron);
}
