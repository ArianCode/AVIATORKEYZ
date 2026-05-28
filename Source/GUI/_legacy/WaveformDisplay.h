#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class AviatorKeyzProcessor;

class WaveformDisplay : public juce::Component
{
public:
    explicit WaveformDisplay (AviatorKeyzProcessor& processorRef);

    void paint (juce::Graphics& g) override;

private:
    AviatorKeyzProcessor& processor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WaveformDisplay)
};
