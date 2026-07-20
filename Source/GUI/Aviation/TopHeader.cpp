#include "TopHeader.h"
#include "AviationIcons.h"
#include "AviationTheme.h"

TopHeader::TopHeader()
{
    setRepaintsOnMouseActivity (true);
}

void TopHeader::resized()
{
    const int w = getWidth();
    const int h = getHeight();

    // Navigation mirrored around the header center: MAIN's center sits 150px
    // left of center, PERFORMANCE's center 150px right.
    mainTabArea = { w / 2 - 150 - 120, 0, 240, h };
    perfTabArea = { w / 2 + 150 - 150, 0, 300, h };

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
        juce::ColourGradient glow (Aviation::cyan().withAlpha (0.10f), r.getCentreX(), r.getBottom(),
                                   juce::Colours::transparentBlack, r.getCentreX(), r.getBottom() - 26.0f, false);
        g.setGradientFill (glow);
        g.fillRect (r.withTop (r.getBottom() - 26.0f));

        // warm gold edge highlight
        g.setColour (Aviation::goldDeep().withAlpha (0.45f));
        g.strokePath (panel, juce::PathStrokeType (1.0f));
        Aviation::topSpecular (g, r, 0.08f);
    }

    // --- Branding (left) ---------------------------------------------------
    {
        const juce::Rectangle<float> logoArea (30.0f, r.getCentreY() - 22.0f, 52.0f, 44.0f);
        AviationIcons::fill (g, AviationIcons::wingLogo(), logoArea, Aviation::goldBright());

        g.setFont (Aviation::label (21.0f, 0.26f));
        g.setColour (Aviation::goldBright());
        g.drawText ("AVIATION", 96, (int) r.getCentreY() - 21, 190, 24, juce::Justification::centredLeft);
        g.setFont (Aviation::label (10.0f, 0.55f));
        g.setColour (Aviation::gold().withAlpha (0.75f));
        g.drawText ("A U D I O", 99, (int) r.getCentreY() + 3, 190, 13, juce::Justification::centredLeft);
    }

    // --- Center navigation ---------------------------------------------------
    auto drawNav = [&] (juce::Rectangle<int> area, const juce::String& text, bool active, bool hover)
    {
        g.setFont (Aviation::label (15.0f, 0.18f));
        g.setColour (active ? Aviation::textPrimary()
                            : (hover ? Aviation::textSecondary().brighter (0.35f)
                                     : Aviation::textSecondary()));
        g.drawText (text, area, juce::Justification::centred);

        if (active)
        {
            // narrow cyan-white illuminated underline
            const int uw = 64;
            juce::Rectangle<float> underline ((float) area.getCentreX() - uw * 0.5f,
                                              (float) area.getBottom() - 5.0f, (float) uw, 3.0f);
            g.setColour (Aviation::cyan().withAlpha (0.35f));
            g.fillRoundedRectangle (underline.expanded (3.0f, 2.0f), 3.0f);
            g.setColour (juce::Colour (0xffd9f2ff));
            g.fillRoundedRectangle (underline, 1.5f);
        }
    };
    drawNav (mainTabArea, "MAIN", ! performanceSelected, hovered == Hit::mainTab);
    drawNav (perfTabArea, "PERFORMANCE", performanceSelected, hovered == Hit::perfTab);

    // --- Right controls --------------------------------------------------------
    AviationIcons::fill (g, AviationIcons::gear(), gearArea.toFloat(),
                         Aviation::gold().withAlpha (hovered == Hit::gear ? 1.0f : 0.8f));
    AviationIcons::fill (g, AviationIcons::ringDot(), utilityArea.toFloat(),
                         Aviation::gold().withAlpha (hovered == Hit::utility ? 1.0f : 0.8f));

    {
        auto sr = saveArea.toFloat();
        g.setColour (juce::Colour (0xff060d14).withAlpha (hovered == Hit::save ? 0.6f : 0.9f));
        g.fillRoundedRectangle (sr, 7.0f);
        g.setColour (Aviation::gold().withAlpha (hovered == Hit::save ? 1.0f : 0.75f));
        g.drawRoundedRectangle (sr, 7.0f, 1.2f);
        g.setFont (Aviation::label (13.0f, 0.16f));
        g.setColour (Aviation::textPrimary());
        g.drawText ("SAVE", saveArea, juce::Justification::centred);
    }
}
