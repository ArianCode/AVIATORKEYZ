#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

// =============================================================================
//  AviationIcons — thin-gold vector icon language for the Aviation interface.
//  Paths are built in a 0..1 unit box; use draw()/fill() helpers to place them.
// =============================================================================

namespace AviationIcons
{
    /** Aviation wing emblem: swept twin wings around a central diamond. */
    inline juce::Path wingLogo()
    {
        juce::Path p;
        // left wing — three swept feathers
        p.startNewSubPath (0.46f, 0.52f);
        p.quadraticTo (0.28f, 0.30f, 0.02f, 0.24f);
        p.quadraticTo (0.20f, 0.42f, 0.40f, 0.55f);
        p.closeSubPath();
        p.startNewSubPath (0.44f, 0.62f);
        p.quadraticTo (0.28f, 0.48f, 0.08f, 0.46f);
        p.quadraticTo (0.24f, 0.60f, 0.41f, 0.68f);
        p.closeSubPath();
        p.startNewSubPath (0.44f, 0.73f);
        p.quadraticTo (0.32f, 0.64f, 0.16f, 0.64f);
        p.quadraticTo (0.30f, 0.75f, 0.43f, 0.79f);
        p.closeSubPath();
        // right wing (mirror)
        p.startNewSubPath (0.54f, 0.52f);
        p.quadraticTo (0.72f, 0.30f, 0.98f, 0.24f);
        p.quadraticTo (0.80f, 0.42f, 0.60f, 0.55f);
        p.closeSubPath();
        p.startNewSubPath (0.56f, 0.62f);
        p.quadraticTo (0.72f, 0.48f, 0.92f, 0.46f);
        p.quadraticTo (0.76f, 0.60f, 0.59f, 0.68f);
        p.closeSubPath();
        p.startNewSubPath (0.56f, 0.73f);
        p.quadraticTo (0.68f, 0.64f, 0.84f, 0.64f);
        p.quadraticTo (0.70f, 0.75f, 0.57f, 0.79f);
        p.closeSubPath();
        // central shield
        p.addEllipse (0.455f, 0.42f, 0.09f, 0.16f);
        return p;
    }

    inline juce::Path gear()
    {
        juce::Path p;
        constexpr int teeth = 8;
        for (int i = 0; i < teeth; ++i)
        {
            const float a = juce::MathConstants<float>::twoPi * (float) i / (float) teeth;
            juce::Path tooth;
            tooth.addRoundedRectangle (-0.075f, -0.50f, 0.15f, 0.16f, 0.04f);
            p.addPath (tooth, juce::AffineTransform::rotation (a).translated (0.5f, 0.5f));
        }
        p.addEllipse (0.14f, 0.14f, 0.72f, 0.72f);
        p.setUsingNonZeroWinding (false);
        p.addEllipse (0.34f, 0.34f, 0.32f, 0.32f);
        return p;
    }

    inline juce::Path heart()
    {
        juce::Path p;
        p.startNewSubPath (0.5f, 0.88f);
        p.cubicTo (0.12f, 0.60f, 0.02f, 0.32f, 0.20f, 0.16f);
        p.cubicTo (0.35f, 0.04f, 0.48f, 0.16f, 0.5f, 0.26f);
        p.cubicTo (0.52f, 0.16f, 0.65f, 0.04f, 0.80f, 0.16f);
        p.cubicTo (0.98f, 0.32f, 0.88f, 0.60f, 0.5f, 0.88f);
        p.closeSubPath();
        return p;
    }

    inline juce::Path star()
    {
        juce::Path p;
        p.addStar ({ 0.5f, 0.5f }, 5, 0.19f, 0.48f, -juce::MathConstants<float>::halfPi);
        return p;
    }

    inline juce::Path magnifier()
    {
        juce::Path p;
        p.addEllipse (0.10f, 0.10f, 0.55f, 0.55f);
        p.setUsingNonZeroWinding (false);
        p.addEllipse (0.20f, 0.20f, 0.35f, 0.35f);
        juce::Path handle;
        handle.addRoundedRectangle (-0.05f, 0.0f, 0.10f, 0.38f, 0.05f);
        p.addPath (handle, juce::AffineTransform::rotation (-juce::MathConstants<float>::pi * 0.25f)
                               .translated (0.72f, 0.72f));
        return p;
    }

    /** filled = true gives a solid triangle; pointing right when rotation == 0. */
    inline juce::Path triangle (bool pointLeft)
    {
        juce::Path p;
        if (pointLeft)
            p.addTriangle (0.75f, 0.10f, 0.75f, 0.90f, 0.15f, 0.5f);
        else
            p.addTriangle (0.25f, 0.10f, 0.25f, 0.90f, 0.85f, 0.5f);
        return p;
    }

    inline juce::Path chevronDown()
    {
        juce::Path p;
        p.startNewSubPath (0.12f, 0.30f);
        p.lineTo (0.50f, 0.70f);
        p.lineTo (0.88f, 0.30f);
        return p;
    }

    /** Small audio waveform glyph (vertical bars). */
    inline juce::Path waveform()
    {
        juce::Path p;
        const float heights[] = { 0.30f, 0.62f, 0.95f, 0.50f, 0.78f, 0.35f, 0.60f, 0.25f };
        const float w = 0.07f;
        for (int i = 0; i < 8; ++i)
        {
            const float h = heights[i];
            p.addRoundedRectangle (0.04f + (float) i * 0.12f, 0.5f - h * 0.5f, w, h, w * 0.4f);
        }
        return p;
    }

    /** Aircraft top view (for the blueprint display). Nose points up. */
    inline juce::Path aircraftTop()
    {
        juce::Path p;
        // fuselage
        p.startNewSubPath (0.50f, 0.02f);
        p.cubicTo (0.545f, 0.10f, 0.55f, 0.22f, 0.55f, 0.34f);
        p.lineTo (0.55f, 0.80f);
        p.cubicTo (0.55f, 0.88f, 0.53f, 0.94f, 0.50f, 0.98f);
        p.cubicTo (0.47f, 0.94f, 0.45f, 0.88f, 0.45f, 0.80f);
        p.lineTo (0.45f, 0.34f);
        p.cubicTo (0.45f, 0.22f, 0.455f, 0.10f, 0.50f, 0.02f);
        p.closeSubPath();
        // main wings
        p.startNewSubPath (0.455f, 0.38f);
        p.lineTo (0.02f, 0.60f);
        p.lineTo (0.02f, 0.66f);
        p.lineTo (0.455f, 0.55f);
        p.closeSubPath();
        p.startNewSubPath (0.545f, 0.38f);
        p.lineTo (0.98f, 0.60f);
        p.lineTo (0.98f, 0.66f);
        p.lineTo (0.545f, 0.55f);
        p.closeSubPath();
        // tail wings
        p.startNewSubPath (0.46f, 0.82f);
        p.lineTo (0.24f, 0.93f);
        p.lineTo (0.24f, 0.97f);
        p.lineTo (0.46f, 0.90f);
        p.closeSubPath();
        p.startNewSubPath (0.54f, 0.82f);
        p.lineTo (0.76f, 0.93f);
        p.lineTo (0.76f, 0.97f);
        p.lineTo (0.54f, 0.90f);
        p.closeSubPath();
        return p;
    }

    /** Circular utility icon: ring with inner dot (record / power style). */
    inline juce::Path ringDot()
    {
        juce::Path p;
        p.addEllipse (0.06f, 0.06f, 0.88f, 0.88f);
        p.setUsingNonZeroWinding (false);
        p.addEllipse (0.16f, 0.16f, 0.68f, 0.68f);
        p.addEllipse (0.34f, 0.34f, 0.32f, 0.32f);
        return p;
    }

    /** Die showing `face` (1-6): rounded square with the pips punched out. */
    inline juce::Path dice (int face)
    {
        juce::Path p;
        p.addRoundedRectangle (0.06f, 0.06f, 0.88f, 0.88f, 0.2f);
        p.setUsingNonZeroWinding (false);
        constexpr float lo = 0.29f, mid = 0.5f, hi = 0.71f, d = 0.16f;
        auto pip = [&p] (float x, float y) { p.addEllipse (x - d * 0.5f, y - d * 0.5f, d, d); };
        switch (juce::jlimit (1, 6, face))
        {
            case 1:  pip (mid, mid); break;
            case 2:  pip (lo, lo); pip (hi, hi); break;
            case 3:  pip (lo, lo); pip (mid, mid); pip (hi, hi); break;
            case 4:  pip (lo, lo); pip (hi, lo); pip (lo, hi); pip (hi, hi); break;
            case 5:  pip (lo, lo); pip (hi, lo); pip (mid, mid); pip (lo, hi); pip (hi, hi); break;
            default: pip (lo, lo); pip (hi, lo); pip (lo, mid); pip (hi, mid); pip (lo, hi); pip (hi, hi); break;
        }
        return p;
    }

    /** Padlock in a fixed 0..1 box (the open shackle lifts inside it, so draw
        with a plain scale transform rather than scale-to-fit). */
    inline juce::Path padlock (bool closed)
    {
        juce::Path p;
        p.addRoundedRectangle (0.16f, 0.46f, 0.68f, 0.50f, 0.08f);

        const float top = closed ? 0.30f : 0.20f;
        juce::Path shackle;
        shackle.startNewSubPath (0.33f, closed ? 0.48f : top + 0.08f);
        shackle.lineTo (0.33f, top);
        shackle.addCentredArc (0.5f, top, 0.17f, 0.17f, 0.0f,
                               -juce::MathConstants<float>::halfPi, juce::MathConstants<float>::halfPi);
        shackle.lineTo (0.67f, 0.48f);

        juce::Path stroked;
        juce::PathStrokeType (0.10f).createStrokedPath (stroked, shackle);
        p.addPath (stroked);
        return p;
    }

    // --- placement helpers ---------------------------------------------------
    inline void fill (juce::Graphics& g, const juce::Path& unitPath,
                      juce::Rectangle<float> area, juce::Colour colour)
    {
        g.setColour (colour);
        g.fillPath (unitPath, unitPath.getTransformToScaleToFit (area, true));
    }

    inline void stroke (juce::Graphics& g, const juce::Path& unitPath,
                        juce::Rectangle<float> area, juce::Colour colour, float thickness)
    {
        juce::Path scaled (unitPath);
        scaled.applyTransform (unitPath.getTransformToScaleToFit (area, true));
        g.setColour (colour);
        g.strokePath (scaled, juce::PathStrokeType (thickness, juce::PathStrokeType::curved,
                                                    juce::PathStrokeType::rounded));
    }

} // namespace AviationIcons
