#include "TopHeader.h"
#include "JetsonicIcons.h"
#include "JetsonicTheme.h"

TopHeader::TopHeader()
{
    setRepaintsOnMouseActivity (true);
}

void TopHeader::resized()
{
    const int w = getWidth();
    const int h = getHeight();

    // Centered navigation. MAIN sits left of center, PERFORMANCE right.
    mainTabArea = { w / 2 - 290, 0, 240, h };
    perfTabArea = { w / 2 + 50, 0, 300, h };

    gearArea    = { w - 220, h / 2 - 14, 28, 28 };
    utilityArea = { w - 172, h / 2 - 13, 26, 26 };
    saveArea    = { w - 130, h / 2 - 18, 100, 36 };
}

TopHeader::Hit TopHeader::hitAt (juce::Point<int> pos) const
{
    if (mainTabArea.contains (pos))    return Hit::mainTab;
    if (perfTabArea.contains (pos))    return Hit::perfTab;
    if (gearArea.expanded (6).contains (pos))    return Hit::gear;
    if (utilityArea.expanded (6).contains (pos)) return Hit::utility;
    if (saveArea.contains (pos))       return Hit::save;
    return Hit::none;
}

void TopHeader::mouseMove (const juce::MouseEvent& e)
{
    const auto hit = hitAt (e.getPosition());
    if (hit != hovered)
    {
        hovered = hit;
        setMouseCursor (hit == Hit::none ? juce::MouseCursor::NormalCursor
                                         : juce::MouseCursor::PointingHandCursor);
        repaint();
    }
}

void TopHeader::mouseExit (const juce::MouseEvent&)
{
    hovered = Hit::none;
    repaint();
}

void TopHeader::mouseDown (const juce::MouseEvent& e)
{
    switch (hitAt (e.getPosition()))
    {
        case Hit::mainTab:
            if (performanceSelected) { performanceSelected = false; if (onModeChanged) onModeChanged (false); repaint(); }
            break;
        case Hit::perfTab:
            if (! performanceSelected) { performanceSelected = true; if (onModeChanged) onModeChanged (true); repaint(); }
            break;
        case Hit::gear:    if (onSettingsClicked) onSettingsClicked(); break;
        case Hit::utility: if (onUtilityClicked)  onUtilityClicked();  break;
        case Hit::save:    if (onSaveClicked)     onSaveClicked();     break;
        case Hit::none:    break;
    }
}

void TopHeader::setPerformanceSelected (bool performance)
{
    if (performanceSelected != performance)
    {
        performanceSelected = performance;
        repaint();
    }
}

void TopHeader::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();

    // Metallic panel: rounded top corners only.
    {
        juce::Path panel;
        panel.addRoundedRectangle (r.getX(), r.getY(), r.getWidth(), r.getHeight(),
                                   10.0f, 10.0f, true, true, false, false);
        juce::ColourGradient grad (juce::Colour (0xff0e1c2a), r.getX(), r.getY(),
                                   juce::Colour (0xff040a10), r.getX(), r.getBottom(), false);
        grad.addColour (0.5, juce::Colour (0xff081321));
        g.setGradientFill (grad);
        g.fillPath (panel);

        // subtle internal blue illumination along the bottom
        juce::ColourGradient glow (Jetsonic::cyan().withAlpha (0.10f), r.getCentreX(), r.getBottom(),
                                   juce::Colours::transparentBlack, r.getCentreX(), r.getBottom() - 26.0f, false);
        g.setGradientFill (glow);
        g.fillRect (r.withTop (r.getBottom() - 26.0f));

        // warm gold edge highlight
        g.setColour (Jetsonic::goldDeep().withAlpha (0.45f));
        g.strokePath (panel, juce::PathStrokeType (1.0f));
        Jetsonic::topSpecular (g, r, 0.08f);
    }

    // --- Branding (left) ---------------------------------------------------
    {
        const juce::Rectangle<float> logoArea (30.0f, r.getCentreY() - 22.0f, 52.0f, 44.0f);
        JetsonicIcons::fill (g, JetsonicIcons::wingLogo(), logoArea, Jetsonic::goldBright());

        g.setFont (Jetsonic::label (21.0f, 0.26f));
        g.setColour (Jetsonic::goldBright());
        g.drawText ("JETSONIC", 96, (int) r.getCentreY() - 21, 190, 24, juce::Justification::centredLeft);
        g.setFont (Jetsonic::label (10.0f, 0.55f));
        g.setColour (Jetsonic::gold().withAlpha (0.75f));
        g.drawText ("A U D I O", 99, (int) r.getCentreY() + 3, 190, 13, juce::Justification::centredLeft);
    }

    // --- Center navigation ---------------------------------------------------
    auto drawNav = [&] (juce::Rectangle<int> area, const juce::String& text, bool active, bool hover)
    {
        g.setFont (Jetsonic::label (15.0f, 0.18f));
        g.setColour (active ? Jetsonic::textPrimary()
                            : (hover ? Jetsonic::textSecondary().brighter (0.35f)
                                     : Jetsonic::textSecondary()));
        g.drawText (text, area, juce::Justification::centred);

        if (active)
        {
            // narrow cyan-white illuminated underline
            const int uw = 64;
            juce::Rectangle<float> underline ((float) area.getCentreX() - uw * 0.5f,
                                              (float) area.getBottom() - 5.0f, (float) uw, 3.0f);
            g.setColour (Jetsonic::cyan().withAlpha (0.35f));
            g.fillRoundedRectangle (underline.expanded (3.0f, 2.0f), 3.0f);
            g.setColour (juce::Colour (0xffd9f2ff));
            g.fillRoundedRectangle (underline, 1.5f);
        }
    };
    drawNav (mainTabArea, "MAIN", ! performanceSelected, hovered == Hit::mainTab);
    drawNav (perfTabArea, "PERFORMANCE", performanceSelected, hovered == Hit::perfTab);

    // --- Right controls --------------------------------------------------------
    JetsonicIcons::fill (g, JetsonicIcons::gear(), gearArea.toFloat(),
                         Jetsonic::gold().withAlpha (hovered == Hit::gear ? 1.0f : 0.8f));
    JetsonicIcons::fill (g, JetsonicIcons::ringDot(), utilityArea.toFloat(),
                         Jetsonic::gold().withAlpha (hovered == Hit::utility ? 1.0f : 0.8f));

    {
        auto sr = saveArea.toFloat();
        g.setColour (juce::Colour (0xff060d14).withAlpha (hovered == Hit::save ? 0.6f : 0.9f));
        g.fillRoundedRectangle (sr, 7.0f);
        g.setColour (Jetsonic::gold().withAlpha (hovered == Hit::save ? 1.0f : 0.75f));
        g.drawRoundedRectangle (sr, 7.0f, 1.2f);
        g.setFont (Jetsonic::label (13.0f, 0.16f));
        g.setColour (Jetsonic::textPrimary());
        g.drawText ("SAVE", saveArea, juce::Justification::centred);
    }
}
