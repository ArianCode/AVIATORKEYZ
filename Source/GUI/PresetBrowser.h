#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

// M3: Preset browser — category sidebar + preset list + save button
class PresetBrowser : public juce::Component
{
public:
    PresetBrowser() = default;
    void paint (juce::Graphics&) override {}
    void resized() override {}
private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetBrowser)
};
