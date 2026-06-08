#include "SearchIconButton.h"
#include "../AviatorTokens.h"

void SearchIconButton::paintButton (juce::Graphics& g, bool highlighted, bool down)
{
    auto bounds = getLocalBounds().toFloat().reduced (4.f);
    g.setColour (down ? juce::Colour (0xff1a3050) : juce::Colour (0xff142840));
    g.fillRoundedRectangle (bounds, 4.f);

    const juce::Colour iconCol = highlighted ? AviatorTokens::champagneGold() : AviatorTokens::textPrimary();
    g.setColour (iconCol);

    const float cx = bounds.getCentreX();
    const float cy = bounds.getCentreY();
    const float r = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.22f;
    g.drawEllipse (cx - r, cy - r, r * 2.f, r * 2.f, 1.6f);
    g.drawLine (cx + r * 0.65f, cy + r * 0.65f, cx + r * 1.5f, cy + r * 1.5f, 1.8f);
}
