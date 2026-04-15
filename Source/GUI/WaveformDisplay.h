#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

// M4: Live waveform display — shows loaded sample, highlights triggered zone
class WaveformDisplay : public juce::Component
{
public:
    WaveformDisplay() = default;
    void paint (juce::Graphics& g) override
    {
        g.fillAll (juce::Colour (0xff0d0d0d));
        g.setColour (juce::Colour (0xff333333));
        g.drawRect (getLocalBounds());
        // M4: draw thumbnail waveform + playhead
    }
    void resized() override {}
private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WaveformDisplay)
};
