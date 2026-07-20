#pragma once

#include "MacroKnob.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>
#include <vector>

// =============================================================================
//  MacroDeck — bottom macro control deck: black leather/metal panel, fine
//  gold dividers, eight heavy rotary macros around the Jetsonic brand block.
//
//  THROTTLE/Glide  ENGINE/Gain  WINGS/Brightness  ALTITUDE/Reverb  [brand]
//  CABIN/Tone  TURBULENCE/Filter  ATTACK  RELEASE
// =============================================================================

class MacroDeck : public juce::Component
{
public:
    explicit MacroDeck (juce::AudioProcessorValueTreeState& apvts);

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    std::vector<std::unique_ptr<MacroKnob>> knobs;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MacroDeck)
};
