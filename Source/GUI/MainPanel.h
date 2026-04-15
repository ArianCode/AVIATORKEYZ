#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

// M4: Root layout component — contains HeaderBar, center controls, footer
class MainPanel : public juce::Component
{
public:
    MainPanel() = default;
    void paint (juce::Graphics&) override {}
    void resized() override {}
private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainPanel)
};
