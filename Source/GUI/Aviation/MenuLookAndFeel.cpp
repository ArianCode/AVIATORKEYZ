#include "MenuLookAndFeel.h"
#include "AviationTheme.h"

MenuLookAndFeel::MenuLookAndFeel()
{
    setColour (juce::PopupMenu::backgroundColourId, juce::Colours::transparentBlack);
}

void MenuLookAndFeel::drawPopupMenuBackgroundWithOptions (juce::Graphics& g, int width, int height,
                                                          const juce::PopupMenu::Options&)
{
    auto r = juce::Rectangle<float> (0.0f, 0.0f, (float) width, (float) height);

    // Cabin panel base
    juce::ColourGradient grad (juce::Colour (0xfa0b1620), r.getX(), r.getY(),
                               juce::Colour (0xfc050c13), r.getX(), r.getBottom(), false);
    g.setGradientFill (grad);
    g.fillRoundedRectangle (r, 9.0f);

    // Overhead bin doors: faint curved panels stacked down the compartment,
    // each with a soft top highlight and a slim recessed handle.
    const float doorH = 92.0f;
    const int doorCount = juce::jmax (1, (int) std::ceil ((r.getHeight() - 16.0f) / doorH));
    for (int i = 0; i < doorCount; ++i)
    {
        juce::Rectangle<float> door (14.0f, 8.0f + (float) i * doorH,
                                     r.getWidth() - 28.0f, doorH - 8.0f);
        if (door.getY() > r.getBottom() - 12.0f)
            break;

        juce::Path doorPath;
        doorPath.addRoundedRectangle (door.getX(), door.getY(), door.getWidth(), door.getHeight(),
                                      6.0f, 16.0f, true, true, true, true);
        g.setColour (juce::Colours::white.withAlpha (0.045f));
        g.strokePath (doorPath, juce::PathStrokeType (1.0f));

        // soft cabin-light reflection along the door's upper edge
        g.setColour (juce::Colours::white.withAlpha (0.025f));
        g.fillRect (juce::Rectangle<float> (door.getX() + 8.0f, door.getY() + 2.0f,
                                            door.getWidth() - 16.0f, 1.2f));

        // recessed door handle, centred near the door's lower edge
        juce::Rectangle<float> handle (door.getCentreX() - 17.0f, door.getBottom() - 9.0f, 34.0f, 4.5f);
        g.setColour (juce::Colours::black.withAlpha (0.30f));
        g.fillRoundedRectangle (handle.expanded (1.5f), 3.0f);
        g.setColour (Aviation::gold().withAlpha (0.16f));
        g.fillRoundedRectangle (handle, 2.5f);
    }

    // LED cabin light strip along the left edge
    juce::ColourGradient strip (Aviation::cyan().withAlpha (0.16f), 3.0f, r.getCentreY(),
                                juce::Colours::transparentBlack, 16.0f, r.getCentreY(), false);
    g.setGradientFill (strip);
    g.fillRoundedRectangle (r.withWidth (16.0f), 9.0f);
    g.setColour (Aviation::cyanBright().withAlpha (0.35f));
    g.fillRoundedRectangle (juce::Rectangle<float> (3.0f, 10.0f, 1.5f, r.getHeight() - 20.0f), 0.75f);

    // fine gold trim
    g.setColour (Aviation::goldDeep().withAlpha (0.55f));
    g.drawRoundedRectangle (r.reduced (0.5f), 9.0f, 1.0f);
}

void MenuLookAndFeel::drawSuitcase (juce::Graphics& g, juce::Rectangle<float> box, juce::Colour colour)
{
    // handle
    const juce::Rectangle<float> handle (box.getCentreX() - box.getWidth() * 0.18f, box.getY(),
                                         box.getWidth() * 0.36f, box.getHeight() * 0.30f);
    g.setColour (colour);
    g.drawRoundedRectangle (handle, 2.0f, 1.2f);

    // body
    const juce::Rectangle<float> body (box.getX(), box.getY() + box.getHeight() * 0.22f,
                                       box.getWidth(), box.getHeight() * 0.78f);
    g.fillRoundedRectangle (body, 2.0f);

    // straps
    g.setColour (colour.darker (0.85f));
    const float strapY0 = body.getY() + 1.0f, strapY1 = body.getBottom() - 1.0f;
    g.drawLine (box.getX() + box.getWidth() * 0.30f, strapY0,
                box.getX() + box.getWidth() * 0.30f, strapY1, 1.0f);
    g.drawLine (box.getX() + box.getWidth() * 0.70f, strapY0,
                box.getX() + box.getWidth() * 0.70f, strapY1, 1.0f);
}

void MenuLookAndFeel::drawPopupMenuItem (juce::Graphics& g, const juce::Rectangle<int>& area,
                                         bool isSeparator, bool isActive, bool isHighlighted,
                                         bool isTicked, bool /*hasSubMenu*/,
                                         const juce::String& text, const juce::String& /*shortcut*/,
                                         const juce::Drawable* /*icon*/, const juce::Colour* textColour)
{
    if (isSeparator)
    {
        g.setColour (Aviation::gold().withAlpha (0.18f));
        g.fillRect (area.reduced (18, 0).withHeight (1).withY (area.getCentreY()));
        return;
    }

    auto r = area.toFloat().reduced (10.0f, 1.0f);

    if (isHighlighted && isActive)
    {
        g.setColour (Aviation::gold().withAlpha (0.10f));
        g.fillRoundedRectangle (r, 5.0f);
        g.setColour (Aviation::cyan().withAlpha (0.85f));
        g.fillRoundedRectangle (juce::Rectangle<float> (r.getX(), r.getY() + 3.0f, 2.5f, r.getHeight() - 6.0f), 1.25f);
    }

    // The loaded preset is the suitcase already "on board".
    if (isTicked)
        drawSuitcase (g, { r.getX() + 8.0f, r.getCentreY() - 6.5f, 12.0f, 13.0f },
                      Aviation::goldBright());

    g.setFont (getPopupMenuFont());
    const auto base = textColour != nullptr ? *textColour : Aviation::textPrimary();
    g.setColour (! isActive ? base.withAlpha (0.4f)
                            : (isTicked ? Aviation::goldBright()
                                        : (isHighlighted ? juce::Colours::white : base)));
    g.drawText (text, area.withTrimmedLeft (36).withTrimmedRight (16), juce::Justification::centredLeft);
}

juce::Font MenuLookAndFeel::getPopupMenuFont()
{
    return Aviation::body (13.5f);
}

void MenuLookAndFeel::getIdealPopupMenuItemSizeWithOptions (const juce::String& text, bool isSeparator,
                                                            int, int& idealWidth, int& idealHeight,
                                                            const juce::PopupMenu::Options&)
{
    if (isSeparator)
    {
        idealWidth = 60;
        idealHeight = 9;
        return;
    }

    const auto font = Aviation::body (13.5f);
    idealWidth = (int) std::ceil (juce::GlyphArrangement::getStringWidth (font, text)) + 56;
    idealHeight = 30;
}

int MenuLookAndFeel::getPopupMenuBorderSizeWithOptions (const juce::PopupMenu::Options&)
{
    return 10;
}
