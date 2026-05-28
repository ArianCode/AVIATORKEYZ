#pragma once

#include "PrecisionKnob.h"
#include "ReverseToggle.h"
#include <juce_gui_basics/juce_gui_basics.h>

class AviatorKeyzProcessor;

class MacroSection : public juce::Component
{
public:
    explicit MacroSection (AviatorKeyzProcessor& processor);

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    ReverseToggle reverseToggle;
    PrecisionKnob   glideKnob;
    PrecisionKnob   smearKnob;
    PrecisionKnob   toneKnob;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MacroSection)
};
