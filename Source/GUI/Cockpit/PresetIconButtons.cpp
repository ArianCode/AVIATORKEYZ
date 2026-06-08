#include "PresetIconButtons.h"
#include "../AviatorTokens.h"

void HeartIconButton::paintButton (juce::Graphics& g, bool highlighted, bool down)
{
    auto bounds = getLocalBounds().toFloat().reduced (2.f);
    juce::ignoreUnused (down);

    juce::Path heart;
    const float cx = bounds.getCentreX();
    const float cy = bounds.getCentreY() + bounds.getHeight() * 0.04f;
    const float s  = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.38f;

    heart.startNewSubPath (cx, cy + s * 0.35f);
    heart.cubicTo (cx - s * 0.95f, cy - s * 0.15f, cx - s * 0.45f, cy - s * 0.95f, cx, cy - s * 0.35f);
    heart.cubicTo (cx + s * 0.45f, cy - s * 0.95f, cx + s * 0.95f, cy - s * 0.15f, cx, cy + s * 0.35f);
    heart.closeSubPath();

    const juce::Colour col = favourited ? AviatorTokens::champagneGold()
                          : highlighted ? AviatorTokens::champagneGold().withAlpha (0.75f)
                                        : AviatorTokens::textPrimary().withAlpha (0.65f);
    g.setColour (col);
    g.fillPath (heart);
    g.setColour (col.brighter (favourited ? 0.2f : 0.f));
    g.strokePath (heart, juce::PathStrokeType (1.f));
}
