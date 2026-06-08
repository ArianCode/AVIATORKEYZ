#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>
#include <memory>
#include <vector>

/** Fixed top dock: factory category tabs. */
class CockpitTopPresetBar : public juce::Component
{
public:
    CockpitTopPresetBar();

    std::function<void (const juce::String& category)> onCategorySelected;

    void setCategories (const juce::StringArray& categories);
    void setActiveCategory (const juce::String& category);
    void setBuildStampText (const juce::String& text);

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    void styleCategoryButton (juce::TextButton& btn, bool active);

    std::vector<std::unique_ptr<juce::TextButton>> categoryButtons;
    juce::Label buildStamp { "buildStamp", {} };
    juce::String activeCategory;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CockpitTopPresetBar)
};
