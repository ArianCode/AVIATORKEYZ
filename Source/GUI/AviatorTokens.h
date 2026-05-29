#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace AviatorTokens
{
    inline juce::Colour instrumentCyan()   { return juce::Colour (0xff00cfff); }
    inline juce::Colour champagneGold()   { return juce::Colour (0xffd4bc86); }
    inline juce::Colour mfdAmber()          { return juce::Colour (0xfff5a623); }
    inline juce::Colour glassFill()         { return juce::Colour (0x8c050d1a); }
    inline juce::Colour textPrimary()     { return juce::Colour (0xffe6f3ff); }
    inline juce::Colour textMuted()       { return juce::Colour (0xff7d97b3); }

    static constexpr int kDesignWidth   = 1600;
    static constexpr int kDesignHeight  = 900;
    static constexpr int kFooterH       = 26;

    static constexpr int kPanelPad      = 8;
    static constexpr int kTitleH        = 16;
    static constexpr int kKnobCellW     = 76;
    static constexpr int kKnobCellH     = 96;
    static constexpr int kGridGapX      = 6;
    static constexpr int kGridGapY      = 4;

    inline float scaleFactor (int currentWidth) noexcept
    {
        return juce::jlimit (0.8f, 1.28f, (float) currentWidth / (float) kDesignWidth);
    }

    inline float scaleFor (const juce::Component& c) noexcept
    {
        int w = c.getWidth();
        for (auto* p = c.getParentComponent(); p != nullptr; p = p->getParentComponent())
            w = juce::jmax (w, p->getWidth());
        return scaleFactor (w > 0 ? w : kDesignWidth);
    }

    inline int scaled (int designPx, float s) noexcept
    {
        return juce::roundToInt ((float) designPx * s);
    }

    inline int scaledFor (const juce::Component& c, int designPx) noexcept
    {
        return scaled (designPx, scaleFor (c));
    }

    inline juce::Font hud (float size)
    {
        return juce::Font (juce::FontOptions ("Share Tech Mono", size, juce::Font::plain));
    }

    inline juce::Font hudBold (float size)
    {
        return juce::Font (juce::FontOptions ("Share Tech Mono", size, juce::Font::bold));
    }

    inline void paintGlassPanel (juce::Graphics& g, juce::Rectangle<float> bounds, bool circular)
    {
        g.setColour (glassFill());
        if (circular)
        {
            g.fillEllipse (bounds);
            g.setColour (instrumentCyan().withAlpha (0.35f));
            g.drawEllipse (bounds, 1.f);
        }
        else
        {
            g.fillRoundedRectangle (bounds, 5.f);
            g.setColour (instrumentCyan().withAlpha (0.28f));
            g.drawRoundedRectangle (bounds, 5.f, 1.f);
            g.setColour (champagneGold().withAlpha (0.12f));
            g.drawRoundedRectangle (bounds.reduced (1.f), 4.f, 0.5f);
        }
    }

    inline void paintSectionTitle (juce::Graphics& g,
                                   juce::Rectangle<int> bounds,
                                   const juce::String& title,
                                   float scale)
    {
        g.setFont (hudBold (10.f * scale));
        g.setColour (instrumentCyan().withAlpha (0.92f));
        g.drawText (title, bounds, juce::Justification::centredLeft);
    }

    inline void paintStatusStrip (juce::Graphics& g, juce::Rectangle<float> bounds)
    {
        paintGlassPanel (g, bounds, false);
    }

} // namespace AviatorTokens
