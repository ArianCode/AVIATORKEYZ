#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

// =============================================================================
//  CategoryTabs — full-width category tab bar (LEADS ... BELLS).
//  Selected tab gets the smoky bronze-gold illuminated background.
//  Version stamp is drawn at the far right.
// =============================================================================

class CategoryTabs : public juce::Component
{
public:
    CategoryTabs();

    std::function<void (const juce::String& category)> onCategorySelected;

    void setCategories (const juce::StringArray& names);
    void setActiveCategory (const juce::String& name);
    void setVersionText (const juce::String& text);

    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseMove (const juce::MouseEvent& e) override;
    void mouseExit (const juce::MouseEvent& e) override;

private:
    int tabIndexAt (juce::Point<int> pos) const;
    juce::Rectangle<int> tabBounds (int index) const;
    int tabsRight() const;

    juce::StringArray categories;
    juce::String activeCategory;
    juce::String versionText;
    int hoveredTab { -1 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CategoryTabs)
};
