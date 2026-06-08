#pragma once

#include "../AviatorTokens.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

class AviatorKeyzProcessor;

/** Analog Lab–style two-column preset browse overlay. */
class PresetSearchOverlay : public juce::Component
{
public:
    explicit PresetSearchOverlay (AviatorKeyzProcessor& processor);

    std::function<void()> onDismiss;
    std::function<void()> onPresetLoaded;

    void showForCategory (const juce::String& category);
    void dismiss();

    void paint (juce::Graphics& g) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent& e) override;

    ~PresetSearchOverlay() override;

private:
    friend class CategoryList;
    friend class PresetList;

    class CategoryList;
    class PresetList;

    void rebuildPresetList();
    juce::Rectangle<int> getPanelBounds() const;

    AviatorKeyzProcessor& processorRef;
    juce::String activeCategory;
    juce::StringArray filteredNames;
    juce::String currentPresetName;

    juce::TextEditor searchField;
    std::unique_ptr<CategoryList> categoryList;
    std::unique_ptr<PresetList> presetList;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetSearchOverlay)
};
