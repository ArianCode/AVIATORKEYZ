#include "PresetHeader.h"
#include "JetsonicIcons.h"
#include "JetsonicTheme.h"

namespace
{
constexpr int kNameHalfWidth = 250; // preset name field half-width around center
} // namespace

PresetHeader::PresetHeader()
{
    setRepaintsOnMouseActivity (true);
}

juce::Rectangle<int> PresetHeader::prevArea() const
{
    return { getWidth() / 2 - kNameHalfWidth - 26, getHeight() / 2 - 9, 18, 18 };
}

juce::Rectangle<int> PresetHeader::nextArea() const
{
    return { getWidth() / 2 + kNameHalfWidth + 8, getHeight() / 2 - 9, 18, 18 };
}

juce::Rectangle<int> PresetHeader::heartArea() const
{
    return { getWidth() / 2 + kNameHalfWidth + 44, getHeight() / 2 - 9, 19, 18 };
}

PresetHeader::Hit PresetHeader::hitAt (juce::Point<int> pos) const
{
    if (prevArea().expanded (6).contains (pos))  return Hit::prev;
    if (nextArea().expanded (6).contains (pos))  return Hit::next;
    if (heartArea().expanded (6).contains (pos)) return Hit::heart;
    return Hit::none;
}

void PresetHeader::mouseMove (const juce::MouseEvent& e)
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

void PresetHeader::mouseExit (const juce::MouseEvent&)
{
    hovered = Hit::none;
    repaint();
}

void PresetHeader::mouseDown (const juce::MouseEvent& e)
{
    switch (hitAt (e.getPosition()))
    {
        case Hit::prev:  if (onPrevPreset) onPrevPreset(); break;
        case Hit::next:  if (onNextPreset) onNextPreset(); break;
        case Hit::heart:
            favourited = ! favourited;
            if (onFavoriteToggled) onFavoriteToggled (favourited);
            repaint();
            break;
        case Hit::none: break;
    }
}

void PresetHeader::setPresetName (const juce::String& name)
{
    if (presetName != name)
    {
        presetName = name;
        repaint();
    }
}

void PresetHeader::setFavourited (bool fav)
{
    if (favourited != fav)
    {
        favourited = fav;
        repaint();
    }
}

void PresetHeader::paint (juce::Graphics& g)
{
    auto r = getLocalBounds();

    // FACTORY PRESETS — upper left, underlined section label
    g.setFont (Jetsonic::label (11.0f, 0.16f));
    g.setColour (Jetsonic::textPrimary().withAlpha (0.85f));
    const juce::Rectangle<int> fpArea (27, 0, 150, r.getHeight());
    g.drawText ("FACTORY PRESETS", fpArea, juce::Justification::centredLeft);
    g.setColour (Jetsonic::textSecondary().withAlpha (0.6f));
    g.fillRect (27, r.getHeight() / 2 + 10, 112, 1);

    // Centered preset name
    g.setFont (Jetsonic::body (15.0f));
    g.setColour (Jetsonic::textPrimary());
    const juce::Rectangle<int> nameArea (r.getCentreX() - kNameHalfWidth, 0,
                                         kNameHalfWidth * 2, r.getHeight());
    const auto fitted = [&]() -> juce::String
    {
        auto font = g.getCurrentFont();
        const auto width = [&font] (const juce::String& s)
        { return juce::GlyphArrangement::getStringWidth (font, s); };
        if (width (presetName) <= (float) nameArea.getWidth())
            return presetName;
        auto t = presetName;
        while (t.isNotEmpty() && width (t + "...") > (float) nameArea.getWidth())
            t = t.dropLastCharacters (1);
        return t + "...";
    }();
    g.drawText (fitted, nameArea, juce::Justification::centred);

    // Prev / next triangles
    JetsonicIcons::fill (g, JetsonicIcons::triangle (true), prevArea().toFloat().reduced (3.0f),
                         Jetsonic::textPrimary().withAlpha (hovered == Hit::prev ? 1.0f : 0.7f));
    JetsonicIcons::fill (g, JetsonicIcons::triangle (false), nextArea().toFloat().reduced (3.0f),
                         Jetsonic::textPrimary().withAlpha (hovered == Hit::next ? 1.0f : 0.7f));

    // Favorite heart
    const auto heartCol = favourited ? Jetsonic::goldBright()
                                     : Jetsonic::textSecondary().withAlpha (hovered == Hit::heart ? 1.0f : 0.8f);
    if (favourited)
        JetsonicIcons::fill (g, JetsonicIcons::heart(), heartArea().toFloat(), heartCol);
    else
        JetsonicIcons::stroke (g, JetsonicIcons::heart(), heartArea().toFloat().reduced (1.0f), heartCol, 1.3f);
}
