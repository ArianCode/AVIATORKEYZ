#pragma once

#include "HorizontalFader.h"
#include <juce_gui_basics/juce_gui_basics.h>

class AviatorKeyzProcessor;

class SecondaryPanel : public juce::Component
{
public:
    explicit SecondaryPanel (AviatorKeyzProcessor& processor);

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    juce::Label sectionAmp;
    juce::Label sectionSpace;
    juce::Label sectionOutput;

    HorizontalFader attackFader;
    HorizontalFader releaseFader;
    HorizontalFader roomFader;
    HorizontalFader widthFader;
    HorizontalFader levelFader;
    HorizontalFader panFader;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SecondaryPanel)
};
