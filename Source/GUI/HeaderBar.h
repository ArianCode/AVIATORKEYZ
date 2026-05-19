#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class HeaderBar : public juce::Component
{
public:
    HeaderBar();

    void setPresetInfo (const juce::String& category, const juce::String& presetName);
    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    juce::Label titleLabel;
    juce::Label subtitleLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HeaderBar)
};
