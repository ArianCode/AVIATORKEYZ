#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace AviatorTokens
{
    inline juce::Colour instrumentCyan()  { return juce::Colour (0xff00cfff); }
    inline juce::Colour mfdAmber()       { return juce::Colour (0xfff5a623); }
    inline juce::Colour glassFill()      { return juce::Colour (0x8c050d1a); } // rgba(5,13,26,0.55)
    inline juce::Colour textPrimary()    { return juce::Colour (0xffe6f3ff); }
    inline juce::Colour textMuted()      { return juce::Colour (0xff7d97b3); }

    static constexpr int kDesignWidth   = 1600;
    static constexpr int kDesignHeight  = 900;
    static constexpr int kFooterH         = 22;

    inline float scaleFactor (int currentWidth) noexcept
    {
        return juce::jlimit (0.8f, 1.28f, (float) currentWidth / (float) kDesignWidth);
    }

    inline juce::Font hud (float size)
    {
        return juce::Font (juce::FontOptions ("Share Tech Mono", size, juce::Font::plain));
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
            g.fillRoundedRectangle (bounds, 4.f);
            g.setColour (instrumentCyan().withAlpha (0.25f));
            g.drawRoundedRectangle (bounds, 4.f, 1.f);
        }
    }
}
