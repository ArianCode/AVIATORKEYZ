#pragma once

#include "../AviatorTokens.h"
#include <juce_gui_basics/juce_gui_basics.h>

/** CATEGORY | PRESET_NAME pill centered on the cockpit horizon. */
class PresetHorizonReadout : public juce::Component
{
public:
    PresetHorizonReadout() = default;

    void setText (const juce::String& category, const juce::String& presetName);
    void paint (juce::Graphics& g) override;

private:
    juce::String readoutText;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetHorizonReadout)
};
