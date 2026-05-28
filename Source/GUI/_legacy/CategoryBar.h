#pragma once

#include <functional>
#include <juce_gui_basics/juce_gui_basics.h>

class CategoryBar : public juce::Component
{
public:
    CategoryBar();

    void setCategories (const juce::StringArray& categories);
    void setActiveCategory (const juce::String& category);
    juce::String getActiveCategory() const { return activeCategory; }

    std::function<void (const juce::String& category)> onCategorySelected;

    void paint (juce::Graphics& g) override;
    void resized() override;
    void mouseUp (const juce::MouseEvent& e) override;

private:
    struct Tab
    {
        juce::String name;
        juce::Rectangle<int> bounds;
    };

    juce::StringArray categories;
    juce::String      activeCategory;
    std::vector<Tab>  tabs;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CategoryBar)
};
