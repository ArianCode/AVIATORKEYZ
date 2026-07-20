#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

// =============================================================================
//  PresetBrowserPanel — left panel: "<CATEGORY> / PRESETS" header, search field,
//  scrollable preset list (cyan selection rail, gold favorite stars, slim
//  bronze scroll thumb). Backed by the real PresetManager list via callbacks.
// =============================================================================

class PresetBrowserPanel : public juce::Component,
                      private juce::ListBoxModel
{
public:
    PresetBrowserPanel();
    ~PresetBrowserPanel() override;

    std::function<void (const juce::String& name)> onPresetChosen;
    std::function<bool (const juce::String& name)> isFavourited;
    std::function<void (const juce::String& name, bool fav)> onFavouriteToggled;

    /** Full (unfiltered) preset list for the active category. */
    void setPresets (const juce::String& category, const juce::StringArray& names);
    void setSelectedPreset (const juce::String& name);

    void paint (juce::Graphics& g) override;
    void paintOverChildren (juce::Graphics& g) override;
    void resized() override;

private:
    // ListBoxModel
    int getNumRows() override;
    void paintListBoxItem (int row, juce::Graphics& g, int width, int height, bool selected) override;
    void listBoxItemClicked (int row, const juce::MouseEvent& e) override;

    void applyFilter();

    class BrowserLookAndFeel;
    std::unique_ptr<BrowserLookAndFeel> lookAndFeel;

    juce::String category;
    juce::StringArray allPresets;
    juce::StringArray filtered;
    juce::String selectedName;

    juce::TextEditor searchBox;
    juce::ListBox listBox;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetBrowserPanel)
};
