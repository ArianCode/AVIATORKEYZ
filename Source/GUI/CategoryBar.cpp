#include "CategoryBar.h"
#include "DesignTokens.h"

CategoryBar::CategoryBar()
{
    setOpaque (true);
    categories.add ("All");
}

void CategoryBar::setCategories (const juce::StringArray& cats)
{
    categories.clear();
    categories.add ("All");
    for (const auto& c : cats)
        if (c != "All")
            categories.add (c);
    resized();
    repaint();
}

void CategoryBar::setActiveCategory (const juce::String& category)
{
    activeCategory = category;
    repaint();
}

void CategoryBar::resized()
{
    const float s = DesignTokens::scaleFactorFor (*this);

    tabs.clear();
    auto area = getLocalBounds().reduced (DesignTokens::scaled (22, s), 0);
    const int gap = DesignTokens::scaled (1, s);

    for (const auto& name : categories)
    {
        const int w = juce::jmax (DesignTokens::scaled (44, s),
                                  (int) (name.length() * 6.5f * s) + DesignTokens::scaled (26, s));
        Tab tab;
        tab.name   = name;
        tab.bounds = area.removeFromLeft (w);
        tabs.push_back (tab);
        area.removeFromLeft (gap);
    }
}

void CategoryBar::paint (juce::Graphics& g)
{
    const float s = DesignTokens::scaleFactorFor (*this);

    juce::ColourGradient bg (juce::Colour (0xff0c0b12), 0.f, 0.f,
                             juce::Colour (0xff0a0910), 0.f, (float) getHeight(), false);
    g.setGradientFill (bg);
    g.fillAll();

    g.setColour (DesignTokens::border1());
    g.drawHorizontalLine (getHeight() - 1, 0.f, (float) getWidth());

    for (const auto& tab : tabs)
    {
        const bool active = tab.name.equalsIgnoreCase (activeCategory)
                            || (activeCategory.isEmpty() && tab.name == "All");

        g.setFont (DesignTokens::labelFont (8.f * s, juce::Font::bold));
        g.setColour (active ? DesignTokens::champagne() : DesignTokens::textFaint());
        g.drawText (tab.name.toUpperCase(), tab.bounds, juce::Justification::centred);

        if (active)
        {
            g.setColour (DesignTokens::champagne().withAlpha (0.5f));
            g.drawHorizontalLine (getHeight() - 1,
                                  (float) tab.bounds.getX(),
                                  (float) tab.bounds.getRight());
            g.fillRect (tab.bounds.getX(), tab.bounds.getCentreY() - DesignTokens::scaled (5, s),
                        DesignTokens::scaled (1, s), DesignTokens::scaled (10, s));
        }
        else if (&tab != &tabs.back())
        {
            g.setColour (DesignTokens::border1());
            g.fillRect (tab.bounds.getRight(), tab.bounds.getCentreY() - DesignTokens::scaled (4, s),
                        DesignTokens::scaled (1, s), DesignTokens::scaled (8, s));
        }
    }
}

void CategoryBar::mouseUp (const juce::MouseEvent& e)
{
    for (const auto& tab : tabs)
    {
        if (tab.bounds.contains (e.getPosition()))
        {
            activeCategory = tab.name;
            repaint();

            if (onCategorySelected)
                onCategorySelected (tab.name);

            return;
        }
    }
}
