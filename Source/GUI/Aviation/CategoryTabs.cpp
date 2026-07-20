#include "CategoryTabs.h"
#include "AviationTheme.h"

namespace
{
constexpr int kTabsLeft   = 27;
constexpr int kVersionW   = 62; // right-aligned version stamp strip
} // namespace

CategoryTabs::CategoryTabs()
{
    setRepaintsOnMouseActivity (true);
}

void CategoryTabs::setCategories (const juce::StringArray& names)
{
    categories = names;
    repaint();
}

void CategoryTabs::setActiveCategory (const juce::String& name)
{
    if (activeCategory != name)
    {
        activeCategory = name;
        repaint();
    }
}

void CategoryTabs::setVersionText (const juce::String& text)
{
    versionText = text;
    repaint();
}

int CategoryTabs::tabsRight() const
{
    return getWidth() - kVersionW - 6;
}

juce::Rectangle<int> CategoryTabs::tabBounds (int index) const
{
    const int count = juce::jmax (1, categories.size());
    const float tabW = (float) (tabsRight() - kTabsLeft) / (float) count;
    const int x0 = kTabsLeft + juce::roundToInt (tabW * (float) index);
    const int x1 = kTabsLeft + juce::roundToInt (tabW * (float) (index + 1));
    return { x0, 2, x1 - x0, getHeight() - 5 };
}

int CategoryTabs::tabIndexAt (juce::Point<int> pos) const
{
    for (int i = 0; i < categories.size(); ++i)
        if (tabBounds (i).contains (pos))
            return i;
    return -1;
}

void CategoryTabs::mouseMove (const juce::MouseEvent& e)
{
    const int tab = tabIndexAt (e.getPosition());
    if (tab != hoveredTab)
    {
        hoveredTab = tab;
        setMouseCursor (tab < 0 ? juce::MouseCursor::NormalCursor
                                : juce::MouseCursor::PointingHandCursor);
        repaint();
    }
}

void CategoryTabs::mouseExit (const juce::MouseEvent&)
{
    hoveredTab = -1;
    repaint();
}

void CategoryTabs::mouseDown (const juce::MouseEvent& e)
{
    const int tab = tabIndexAt (e.getPosition());
    if (tab >= 0 && tab < categories.size() && onCategorySelected)
        onCategorySelected (categories[tab]);
}

void CategoryTabs::paint (juce::Graphics& g)
{
    for (int i = 0; i < categories.size(); ++i)
    {
        const auto tab = tabBounds (i).toFloat();
        const bool active = categories[i].equalsIgnoreCase (activeCategory);
        const bool hover  = (i == hoveredTab);

        if (active)
        {
            // smoky bronze-gold illuminated background
            juce::ColourGradient grad (juce::Colour (0xff54401e), tab.getX(), tab.getY(),
                                       juce::Colour (0xff2b2011), tab.getX(), tab.getBottom(), false);
            grad.addColour (0.5, juce::Colour (0xff473413));
            g.setGradientFill (grad);
            g.fillRoundedRectangle (tab.reduced (1.0f, 1.0f), 4.0f);
            g.setColour (Aviation::goldBright().withAlpha (0.55f));
            g.drawRoundedRectangle (tab.reduced (1.0f, 1.0f), 4.0f, 1.0f);
        }
        else
        {
            juce::ColourGradient grad (juce::Colour (0xff0a1520), tab.getX(), tab.getY(),
                                       juce::Colour (0xff060d15), tab.getX(), tab.getBottom(), false);
            g.setGradientFill (grad);
            g.fillRoundedRectangle (tab.reduced (1.0f, 1.0f), 4.0f);

            // subtle recessed border
            g.setColour (juce::Colours::black.withAlpha (0.5f));
            g.drawRoundedRectangle (tab.reduced (1.0f, 1.0f), 4.0f, 1.0f);
            if (hover)
            {
                g.setColour (Aviation::cyan().withAlpha (0.18f));
                g.fillRoundedRectangle (tab.reduced (1.0f, 1.0f), 4.0f);
            }
        }

        // thin separator between tabs
        if (i > 0)
        {
            g.setColour (juce::Colour (0x30203040));
            g.fillRect (juce::Rectangle<float> (tab.getX() - 0.5f, tab.getY() + 6.0f, 1.0f, tab.getHeight() - 12.0f));
        }

        g.setFont (Aviation::label (12.5f, 0.10f));
        g.setColour (active ? Aviation::textPrimary()
                            : Aviation::textSecondary().withAlpha (hover ? 1.0f : 0.85f));
        g.drawText (categories[i].toUpperCase(), tab.toNearestInt(), juce::Justification::centred);
    }

    // version stamp — far right
    g.setFont (Aviation::body (11.0f));
    g.setColour (Aviation::textSecondary().withAlpha (0.8f));
    g.drawText (versionText, getWidth() - kVersionW - 4, 0, kVersionW, getHeight(),
                juce::Justification::centred);
}
