#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

// M4: Top bar — logo, active preset name, nav tabs (INPUT / TONE / SPACE)
class HeaderBar : public juce::Component
{
public:
    HeaderBar() = default;
    void paint (juce::Graphics&) override {}
    void resized() override {}
private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HeaderBar)
};
