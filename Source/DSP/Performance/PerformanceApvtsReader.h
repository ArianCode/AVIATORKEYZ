#pragma once

#include "PerformanceTypes.h"
#include <juce_audio_processors/juce_audio_processors.h>

class PerformanceApvtsReader
{
public:
    static EngineState readBaseState (const juce::AudioProcessorValueTreeState& apvts) noexcept;

    static float readMacroValue (const juce::AudioProcessorValueTreeState& apvts, int macroIndex) noexcept;

    /** Apply category playback locks. Factory presets always force policy. */
    static void applyCategoryPlaybackDefaults (juce::AudioProcessorValueTreeState& apvts,
                                               const juce::ValueTree& loadedState,
                                               const juce::String& category,
                                               const juce::String& presetName,
                                               const juce::String& soundTypeAttr,
                                               bool isFactoryPreset) noexcept;
};
