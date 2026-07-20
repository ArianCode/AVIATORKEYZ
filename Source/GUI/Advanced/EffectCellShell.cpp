#include "EffectCellShell.h"

EffectCellShell::EffectCellShell (const juce::String& title,
                                  const juce::String& hintText,
                                  const juce::String& tooltip)
    : titleText (title)
    , hintText (hintText)
{
    setOpaque (false);
    setTooltip (tooltip);
}

void EffectCellShell::updateLockBounds()
{
    const float s = AviatorTokens::scaleFor (*this);
    const int lockSize = AviatorTokens::scaled (16, s);
    lockBounds = juce::Rectangle<int> (getWidth() - lockSize - AviatorTokens::scaled (6, s),
                                       AviatorTokens::scaled (6, s),
                                       lockSize, lockSize);
}

void EffectCellShell::setHovering (bool h)
{
    if (hovering == h)
        return;
    hovering = h;
    repaint();
}

void EffectCellShell::setDragging (bool d)
{
    if (dragging == d)
        return;
    dragging = d;
    repaint();
}

void EffectCellShell::setTitle (const juce::String& title)
{
    titleText = title;
    repaint();
}

void EffectCellShell::mouseEnter (const juce::MouseEvent&)
{
    setHovering (true);
}

void EffectCellShell::mouseExit (const juce::MouseEvent&)
{
    setHovering (false);
}

bool EffectCellShell::handleLockClick (const juce::MouseEvent& e)
{
    if (! lockBounds.expanded (4).contains (e.getPosition()))
        return false;

    locked = ! locked;
    repaint();
    return true;
}

void EffectCellShell::drawLockIcon (juce::Graphics& g, juce::Rectangle<float> bounds, float scale) const
{
    const auto colour = locked ? AviatorTokens::mfdAmber() : juce::Colours::white.withAlpha (0.35f);

    const float bodyH = bounds.getHeight() * 0.55f;
    auto body = bounds.removeFromBottom (bodyH);
    g.setColour (colour.withAlpha (locked ? 0.85f : 0.5f));
    g.fillRoundedRectangle (body, 1.5f * scale);

    juce::Path shackle;
    const float shackleW = bounds.getWidth() * 0.7f;
    const float cx = bounds.getCentreX();
    juce::Rectangle<float> arcBounds (cx - shackleW * 0.5f, bounds.getY(), shackleW, bounds.getHeight() * 1.6f);
    shackle.addCentredArc (arcBounds.getCentreX(), arcBounds.getBottom(),
                          arcBounds.getWidth() * 0.5f, arcBounds.getHeight() * 0.85f,
                          0.f,
                          juce::degreesToRadians (locked ? 180.f : 200.f),
                          juce::degreesToRadians (360.f),
                          true);
    g.strokePath (shackle, juce::PathStrokeType (1.4f * scale));
}

void EffectCellShell::paintShell (juce::Graphics& g,
                                  const juce::String& bottomRightText,
                                  bool activeInteraction) const
{
    const float s = AviatorTokens::scaleFor (*this);
    auto bounds = getLocalBounds().toFloat().reduced (1.f);
    const float corner = 6.f * s;

    juce::ColourGradient bg (juce::Colour (0xff0e151d), bounds.getX(), bounds.getY(),
                             juce::Colour (0xff070a0e), bounds.getX(), bounds.getBottom(), false);
    g.setGradientFill (bg);
    g.fillRoundedRectangle (bounds, corner);

    const float borderAlpha = activeInteraction ? 0.85f : (hovering ? 0.55f : 0.22f);
    g.setColour (AviatorTokens::instrumentCyan().withAlpha (locked ? 0.18f : borderAlpha));
    g.drawRoundedRectangle (bounds, corner, locked ? 1.f : (activeInteraction ? 1.6f : 1.f) * s);

    if (locked)
    {
        g.setColour (juce::Colours::black.withAlpha (0.35f));
        g.fillRoundedRectangle (bounds, corner);
    }

    auto content = bounds.reduced (8.f * s);

    g.setFont (AviatorTokens::hudBold (10.5f * s));
    g.setColour (AviatorTokens::instrumentCyan().withAlpha (locked ? 0.4f : 0.92f));
    g.drawText (titleText, content.removeFromTop (14.f * s), juce::Justification::centredLeft);

    drawLockIcon (g, lockBounds.toFloat(), s);

    g.setFont (AviatorTokens::hud (8.f * s));
    g.setColour (juce::Colours::white.withAlpha (locked ? 0.18f : 0.32f));
    auto bottomRow = content.removeFromBottom (12.f * s);
    g.drawText (hintText, bottomRow, juce::Justification::bottomLeft);

    g.setFont (AviatorTokens::hud (9.5f * s));
    g.setColour (locked ? juce::Colours::white.withAlpha (0.3f) : AviatorTokens::champagneGold());
    g.drawText (bottomRightText, bottomRow, juce::Justification::bottomRight);
}

void EffectCellShell::paintCenterLabel (juce::Graphics& g,
                                        const juce::String& centreText,
                                        float scale) const
{
    auto centre = getLocalBounds().toFloat().reduced (8.f * scale);
    centre.removeFromTop (18.f * scale);
    centre.removeFromBottom (16.f * scale);

    g.setFont (AviatorTokens::hudBold (juce::jmin (14.f * scale, centre.getHeight() * 0.45f)));
    g.setColour (locked ? juce::Colours::white.withAlpha (0.35f) : AviatorTokens::instrumentCyan().withAlpha (0.95f));
    g.drawText (centreText, centre, juce::Justification::centred);
}

void EffectCellShell::paintCenterMeter (juce::Graphics& g,
                                        float normalisedValue,
                                        float scale) const
{
    auto bounds = getLocalBounds().toFloat().reduced (1.f);
    auto meterArea = bounds.withSizeKeepingCentre (bounds.getWidth() * 0.74f, 3.f * scale);
    g.setColour (juce::Colours::white.withAlpha (0.08f));
    g.fillRoundedRectangle (meterArea, 1.5f * scale);

    const float val = juce::jlimit (0.f, 1.f, normalisedValue);
    auto fillArea = meterArea.withWidth (juce::jmax (3.f * scale, meterArea.getWidth() * val));
    const auto fillColour = locked ? juce::Colours::grey : AviatorTokens::instrumentCyan();
    g.setColour (fillColour.withAlpha (0.30f));
    g.fillRoundedRectangle (fillArea.expanded (0.f, 2.f * scale), 1.5f * scale);
    g.setColour (fillColour.withAlpha (0.95f));
    g.fillRoundedRectangle (fillArea, 1.5f * scale);

    const float dotR = 2.6f * scale;
    g.setColour (fillColour);
    g.fillEllipse (fillArea.getRight() - dotR, meterArea.getCentreY() - dotR, dotR * 2.f, dotR * 2.f);
}

void EffectCellShell::resized()
{
    updateLockBounds();
}
