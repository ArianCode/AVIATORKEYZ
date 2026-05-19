#pragma once

#include <functional>
#include <juce_gui_basics/juce_gui_basics.h>

class AviatorKeyzProcessor;

class PresetBrowser : public juce::Component,
                      private juce::ComboBox::Listener
{
public:
    PresetBrowser (AviatorKeyzProcessor& processor,
                   std::function<void()> onPresetApplied);

    void resized() override;

    juce::String getSelectedCategory() const { return categoryBox.getText(); }

    void refreshPresetList();

private:
    void comboBoxChanged (juce::ComboBox*) override;
    void applySelectedPreset();

    AviatorKeyzProcessor&        processor;
    std::function<void()>        onPresetApplied;
    juce::ComboBox               categoryBox;
    juce::ComboBox               presetBox;
    juce::Label                  categoryLabel;
    juce::Label                  presetLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetBrowser)
};
