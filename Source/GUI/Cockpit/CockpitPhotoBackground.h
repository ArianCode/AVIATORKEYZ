#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

/** Layer 0: draws embedded cockpit photo at full bounds; no tint or blur. */
class CockpitPhotoBackground : public juce::Component
{
public:
    CockpitPhotoBackground();

    void paint (juce::Graphics& g) override;
    void resized() override;

    void setPhotoVisible (bool visible) { photoVisible = visible; repaint(); }
    bool isPhotoVisible() const noexcept { return photoVisible; }

private:
    juce::Image cockpitImage;
    bool photoVisible = true;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CockpitPhotoBackground)
};
