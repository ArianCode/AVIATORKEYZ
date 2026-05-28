#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace AviatorCockpit
{

struct NormalizedRect
{
    float left   = 0.f;
    float top    = 0.f;
    float right  = 1.f;
    float bottom = 1.f;

    float width()  const noexcept { return right - left; }
    float height() const noexcept { return bottom - top; }

    juce::Rectangle<int> toPixels (juce::Rectangle<int> photoArea) const noexcept
    {
        return { photoArea.getX() + juce::roundToInt (left   * (float) photoArea.getWidth()),
                 photoArea.getY() + juce::roundToInt (top    * (float) photoArea.getHeight()),
                 juce::roundToInt (width()  * (float) photoArea.getWidth()),
                 juce::roundToInt (height() * (float) photoArea.getHeight()) };
    }
};

enum class ZoneId
{
    leftMfd,
    rightMfd,
    radarAdsr,
    radarLfo,
    autopilotStrip,
    overhead,
    throttleQuadrant
};

enum class AnchorKind
{
    rotary,
    toggle,
    verticalLever,
    pushButton,
    guardedDome
};

struct KnobAnchor
{
    const char* id;
    const char* paramId;   // nullptr if action-only
    const char* action;    // nullptr if param-driven
    float x;
    float y;
    float r;
    AnchorKind kind;
    const char* label;
};

inline NormalizedRect zoneRect (ZoneId z) noexcept
{
    switch (z)
    {
        case ZoneId::leftMfd:         return { 0.02f, 0.48f, 0.16f, 0.80f };
        case ZoneId::rightMfd:        return { 0.84f, 0.48f, 0.98f, 0.80f };
        case ZoneId::radarAdsr:       return { 0.28f, 0.50f, 0.44f, 0.75f };
        case ZoneId::radarLfo:        return { 0.56f, 0.50f, 0.72f, 0.75f };
        case ZoneId::autopilotStrip:  return { 0.10f, 0.42f, 0.90f, 0.47f };
        case ZoneId::overhead:        return { 0.00f, 0.00f, 1.00f, 0.28f };
        case ZoneId::throttleQuadrant: return { 0.42f, 0.60f, 0.58f, 0.90f };
    }
    return {};
}

const KnobAnchor* getKnobAnchors() noexcept;
int getKnobAnchorCount() noexcept;

juce::Rectangle<int> anchorBounds (juce::Rectangle<int> photoArea, const KnobAnchor& a) noexcept;

} // namespace AviatorCockpit
