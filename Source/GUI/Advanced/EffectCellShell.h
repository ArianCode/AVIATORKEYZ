#pragma once

#include "../AviatorTokens.h"
#include <juce_gui_basics/juce_gui_basics.h>

/** Shared Memory-style tile chrome: background, border, title, lock, hints. */
class EffectCellShell : public juce::Component,
                        public juce::SettableTooltipClient
{
public:
    EffectCellShell (const juce::String& title,
                     const juce::String& hintText,
                     const juce::String& tooltip);

    void paintShell (juce::Graphics& g,
                     const juce::String& bottomRightText,
                     bool activeInteraction) const;

    void paintCenterLabel (juce::Graphics& g,
                           const juce::String& centreText,
                           float scale) const;

    void paintCenterMeter (juce::Graphics& g,
                           float normalisedValue,
                           float scale) const;

    bool handleLockClick (const juce::MouseEvent& e);
    void setHovering (bool h);
    void setDragging (bool d);
    void setTitle (const juce::String& title);
    bool isLocked() const noexcept { return locked; }

    void resized() override;

    void mouseEnter (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;

protected:
    juce::Rectangle<int> lockBounds;

private:
    void drawLockIcon (juce::Graphics& g, juce::Rectangle<float> bounds, float scale) const;
    void updateLockBounds();

    juce::String titleText;
    juce::String hintText;

    bool locked    { false };
    bool hovering  { false };
    bool dragging  { false };
};
