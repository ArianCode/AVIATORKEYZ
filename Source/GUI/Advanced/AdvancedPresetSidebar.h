#pragma once

#include "../AviatorTokens.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

class AviatorKeyzProcessor;

/** Compact preset/bank browser for Advanced page side panel. */
class AdvancedPresetSidebar : public juce::Component,
                              private juce::ListBoxModel
{
public:
    explicit AdvancedPresetSidebar (AviatorKeyzProcessor& processor);

    std::function<void()> onPresetChanged;

    void refresh();

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    int getNumRows() override;
    void paintListBoxItem (int row, juce::Graphics& g, int w, int h, bool sel) override;
    void listBoxItemClicked (int row, const juce::MouseEvent&) override;

    void rebuildList();

    AviatorKeyzProcessor& processorRef;
    juce::String activeCategory;
    juce::StringArray presetNames;

    juce::Label titleLabel { "title", "PRESETS" };
    juce::TextEditor searchBox;
    juce::ListBox presetList { "advPresets", this };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AdvancedPresetSidebar)
};
