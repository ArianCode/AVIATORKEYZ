#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

// =============================================================================
//  CockpitBackground — the dominant central cockpit artwork.
//
//  Loads the embedded cockpit photo (BinaryData). A resource named
//  "cockpit_sunset_*" takes priority when present in Resources/UI/Cockpit so
//  the final sunset artwork can be dropped in without code changes; until
//  then the shipped cockpit photo is used with a warm sunset grade so it
//  sits correctly under the gold/bronze interface chrome.
//
//  A dark translucent treatment is applied where overlaid labels/controls
//  need readability (top band, center console, lower console).
// =============================================================================

class CockpitBackground : public juce::Component
{
public:
    CockpitBackground();

    void paint (juce::Graphics& g) override;

private:
    juce::Image photo;
    bool usingSunsetAsset { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CockpitBackground)
};
