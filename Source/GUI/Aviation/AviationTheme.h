#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

// =============================================================================
//  AviationTheme — style system for the Aviation Audio cockpit interface.
//
//  All Aviation components are laid out in a fixed 1647 x 955 design space;
//  the editor scales the whole view with an AffineTransform, so components
//  never apply their own scale factors. Coordinates in these files are
//  literal reference-image pixels.
// =============================================================================

namespace Aviation
{
    // --- Canonical design space -------------------------------------------
    static constexpr int kDesignW = 1647;
    static constexpr int kDesignH = 955;

    // --- Region bounds (reference-image pixels) ---------------------------
    inline juce::Rectangle<int> topHeaderBounds()     { return { 6, 4, 1635, 78 }; }
    inline juce::Rectangle<int> presetHeaderBounds()  { return { 6, 82, 1635, 45 }; }
    inline juce::Rectangle<int> categoryTabsBounds()  { return { 6, 127, 1635, 43 }; }
    inline juce::Rectangle<int> cockpitBounds()       { return { 6, 170, 1635, 556 }; }
    inline juce::Rectangle<int> macroDeckBounds()     { return { 6, 726, 1635, 178 }; }
    inline juce::Rectangle<int> statusBarBounds()     { return { 6, 908, 1635, 41 }; }
    inline juce::Rectangle<int> sourceDropdownBounds(){ return { 1296, 186, 334, 54 }; }

    // --- Colors ------------------------------------------------------------
    inline juce::Colour bgBlack()        { return juce::Colour (0xff02070c); }
    inline juce::Colour bgDeep()         { return juce::Colour (0xff040b13); }
    inline juce::Colour panelDark()      { return juce::Colour (0xff07131d); }
    inline juce::Colour panelDarker()    { return juce::Colour (0xff050d16); }
    inline juce::Colour panelBlue()      { return juce::Colour (0xff0a1823); }
    inline juce::Colour glassNavy()      { return juce::Colour (0xf0060f18); }

    inline juce::Colour gold()           { return juce::Colour (0xffc9a35b); }
    inline juce::Colour goldBright()     { return juce::Colour (0xffd7b36a); }
    inline juce::Colour goldDeep()       { return juce::Colour (0xffa77c3b); }
    inline juce::Colour goldDim()        { return juce::Colour (0x66a77c3b); }
    inline juce::Colour bronze()         { return juce::Colour (0xff8a6a38); }

    inline juce::Colour cyan()           { return juce::Colour (0xff39bdf8); }
    inline juce::Colour cyanBright()     { return juce::Colour (0xffa7e7ff); }
    inline juce::Colour cyanDim()        { return juce::Colour (0x5539bdf8); }

    inline juce::Colour textPrimary()    { return juce::Colour (0xffd8dde2); }
    inline juce::Colour textSecondary()  { return juce::Colour (0xff708596); }
    inline juce::Colour textDim()        { return juce::Colour (0xff4a5d6d); }
    inline juce::Colour activeGreen()    { return juce::Colour (0xff70c75a); }

    // --- Typography ---------------------------------------------------------
    // Clean narrow premium sans. Avenir Next exists on macOS; JUCE falls back
    // to the platform default sans on other systems.
    inline juce::Font sans (float size, bool medium = false)
    {
        auto f = juce::Font (juce::FontOptions ("Avenir Next", size,
                                                medium ? juce::Font::bold : juce::Font::plain));
        return f;
    }

    /** Uppercase label style with letter spacing (tracking). */
    inline juce::Font label (float size, float tracking = 0.08f)
    {
        auto f = sans (size, true);
        f.setExtraKerningFactor (tracking);
        return f;
    }

    /** Compact body / list text. */
    inline juce::Font body (float size)
    {
        auto f = sans (size);
        f.setExtraKerningFactor (0.01f);
        return f;
    }

    /** Numeric / value text. */
    inline juce::Font value (float size, bool medium = true)
    {
        return sans (size, medium);
    }

    // --- Shared painters ----------------------------------------------------

    /** Deep black->midnight metallic vertical panel gradient. */
    inline void fillMetalPanel (juce::Graphics& g, juce::Rectangle<float> r, float corner,
                                juce::Colour top = juce::Colour (0xff0c1824),
                                juce::Colour bottom = juce::Colour (0xff040a11))
    {
        juce::ColourGradient grad (top, r.getX(), r.getY(), bottom, r.getX(), r.getBottom(), false);
        grad.addColour (0.35, top.interpolatedWith (bottom, 0.45f));
        g.setGradientFill (grad);
        g.fillRoundedRectangle (r, corner);
    }

    /** Thin warm-gold edge with soft inner blue illumination. */
    inline void strokeGoldEdge (juce::Graphics& g, juce::Rectangle<float> r, float corner,
                                float alpha = 0.55f, float width = 1.0f)
    {
        g.setColour (goldDeep().withAlpha (alpha * 0.8f));
        g.drawRoundedRectangle (r, corner, width);
        g.setColour (goldBright().withAlpha (alpha * 0.35f));
        g.drawRoundedRectangle (r.reduced (0.5f), juce::jmax (0.0f, corner - 0.5f), 0.6f);
    }

    /** Recessed dark glass screen with a faint cyan rim. */
    inline void fillGlassScreen (juce::Graphics& g, juce::Rectangle<float> r, float corner,
                                 float cyanRim = 0.30f)
    {
        juce::ColourGradient grad (juce::Colour (0xe0030a12), r.getX(), r.getY(),
                                   juce::Colour (0xe8060e17), r.getX(), r.getBottom(), false);
        g.setGradientFill (grad);
        g.fillRoundedRectangle (r, corner);

        // tight inner shadow
        g.setColour (juce::Colours::black.withAlpha (0.55f));
        g.drawRoundedRectangle (r.reduced (1.0f), juce::jmax (0.0f, corner - 1.0f), 1.5f);

        g.setColour (cyan().withAlpha (cyanRim));
        g.drawRoundedRectangle (r, corner, 1.0f);
    }

    /** Subtle top specular highlight line for metallic panels. */
    inline void topSpecular (juce::Graphics& g, juce::Rectangle<float> r, float alpha = 0.10f)
    {
        g.setColour (juce::Colours::white.withAlpha (alpha));
        g.fillRect (juce::Rectangle<float> (r.getX() + 2.0f, r.getY() + 1.0f, r.getWidth() - 4.0f, 1.0f));
    }

} // namespace Aviation
