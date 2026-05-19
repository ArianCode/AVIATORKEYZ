#pragma once

#include <functional>
#include <juce_gui_basics/juce_gui_basics.h>

class HeaderBar : public juce::Component
{
public:
    HeaderBar();

    void setPresetDisplayName (const juce::String& displayName);
    void setPresetCount (int count);

    std::function<void()> onPreviousPreset;
    std::function<void()> onNextPreset;
    std::function<void()> onLibraryClicked;

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    juce::Label presetNameLabel;
    juce::Label presetCountLabel;
    juce::TextButton prevButton { "<" };
    juce::TextButton nextButton { ">" };
    juce::TextButton libraryButton { "Library" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HeaderBar)
};
