#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

class AviatorKeyzProcessor;

/** Anchored two-column preset dropdown — categories left, presets right. */
class PresetSelectorOverlay : public juce::Component
{
public:
    explicit PresetSelectorOverlay (AviatorKeyzProcessor& processor);
    ~PresetSelectorOverlay() override;

    std::function<void()> onDismiss;
    std::function<bool (const juce::String& category, const juce::String& name)> isFavourited;

    void showOverlay();
    void dismiss();
    void refreshFromPresetManager();

    void paint (juce::Graphics& g) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent& e) override;
    bool keyPressed (const juce::KeyPress& key) override;

private:
    friend class PresetSelectorCategoryList;
    friend class PresetSelectorPresetList;

    struct PresetEntry
    {
        juce::String category;
        juce::String name;
    };

    juce::Rectangle<int> getPanelBounds() const;
    juce::Rectangle<int> getBodyBounds() const;

    void selectCategory (const juce::String& category);
    void rebuildPresetEntries();
    void selectPresetAt (int index);
    int findCurrentIndexInView() const;

    juce::String categoryLabel (const juce::String& category) const;

    AviatorKeyzProcessor& processorRef;

    juce::String activeCategory;
    juce::String currentCategory;
    juce::String currentPresetName;
    juce::Array<PresetEntry> presetEntries;

    class PresetSelectorCategoryList;
    class PresetSelectorPresetList;
    std::unique_ptr<PresetSelectorCategoryList> categoryList;
    std::unique_ptr<PresetSelectorPresetList> presetList;

    static constexpr int kPanelDesignW = 560;
    static constexpr int kPanelDesignH = 300;
    static constexpr const char* kAllCategory = "All";

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetSelectorOverlay)
};
