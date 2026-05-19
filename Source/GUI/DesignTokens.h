#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

// Piano-black / champagne design tokens (aviatorkeyz_ui_v3.html)
namespace DesignTokens
{
    inline juce::Colour piano()      { return juce::Colour (0xff06060a); }
    inline juce::Colour deep()       { return juce::Colour (0xff08080d); }
    inline juce::Colour surface()    { return juce::Colour (0xff0c0b12); }
    inline juce::Colour panel()      { return juce::Colour (0xff100f17); }
    inline juce::Colour raised()     { return juce::Colour (0xff161422); }
    inline juce::Colour elevated()   { return juce::Colour (0xff1d1b28); }

    inline juce::Colour champagne()     { return juce::Colour (0xffd4bc86); }
    inline juce::Colour champagneBright(){ return juce::Colour (0xffe2ce9e); }
    inline juce::Colour champagneMid()    { return juce::Colour (0xffc4aa70); }

    inline juce::Colour textPrimary() { return juce::Colour (0xffedeae1); }
    inline juce::Colour textSecondary(){ return juce::Colour (0xff7b7788); }
    inline juce::Colour textMuted()   { return juce::Colour (0xff3e3c4e); }
    inline juce::Colour textFaint()   { return juce::Colour (0xff252336); }

    inline juce::Colour border0() { return juce::Colour::fromFloatRGBA (1.f, 1.f, 1.f, 0.030f); }
    inline juce::Colour border1() { return juce::Colour::fromFloatRGBA (1.f, 1.f, 1.f, 0.055f); }
    inline juce::Colour border2() { return juce::Colour::fromFloatRGBA (1.f, 1.f, 1.f, 0.085f); }

    inline juce::Colour statusGreen() { return juce::Colour (0xff4ade80); }

    static constexpr int kDesignWidth  = 860;
    static constexpr int kDesignHeight = 608;

    static constexpr int kTopTrimH       = 1;
    static constexpr int kHeaderH        = 52;
    static constexpr int kCategoryBarH   = 40;
    static constexpr int kArtworkH       = 224;
    static constexpr int kPanelTrimH     = 1;
    static constexpr int kMacroSectionH  = 174;
    static constexpr int kSecondaryH     = 86;
    static constexpr int kFooterH        = 30;

    static constexpr float kCornerRadius = 13.f;

    inline float scaleFactor (int width) noexcept
    {
        return width / (float) kDesignWidth;
    }

    inline float scaleFactorFor (const juce::Component& c) noexcept
    {
        int refWidth = c.getWidth();
        for (auto* p = c.getParentComponent(); p != nullptr; p = p->getParentComponent())
            refWidth = juce::jmax (refWidth, p->getWidth());
        return scaleFactor (refWidth);
    }

    inline int scaled (int designPx, float s) noexcept
    {
        return juce::roundToInt ((float) designPx * s);
    }

    inline juce::Font brandFont()
    {
        return juce::Font (juce::FontOptions ("Inter", 11.f, juce::Font::bold));
    }

    inline juce::Font labelFont (float size, juce::Font::FontStyleFlags style = juce::Font::plain)
    {
        return juce::Font (juce::FontOptions ("Inter", size, style));
    }

    inline juce::Font monoFont (float size)
    {
        return juce::Font (juce::FontOptions ("DM Mono", size, juce::Font::plain));
    }

    inline juce::Font heroFont (float size)
    {
        return juce::Font (juce::FontOptions ("Cormorant Garamond", size, juce::Font::plain));
    }

    inline void drawChampagneTrim (juce::Graphics& g, juce::Rectangle<float> bounds, bool strongCentre)
    {
        juce::ColourGradient grad (
            juce::Colours::transparentBlack,
            bounds.getX(), bounds.getCentreY(),
            juce::Colours::transparentBlack,
            bounds.getRight(), bounds.getCentreY(),
            false);

        const float centreAlpha = strongCentre ? 0.42f : 0.20f;
        grad.addColour (0.20, champagne().withAlpha (0.22f));
        grad.addColour (0.40, champagne().withAlpha (0.20f));
        grad.addColour (0.50, champagneBright().withAlpha (centreAlpha));
        grad.addColour (0.60, champagne().withAlpha (0.20f));
        grad.addColour (0.80, champagne().withAlpha (0.22f));

        g.setGradientFill (grad);
        g.fillRect (bounds);
    }
}
