#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace AviatorKeyz
{
/** Full APVTS parameter layout (v1 core + v2/v3 advanced). Shared by processor and unit tests. */
juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

/** Returns every param ID string registered by createParameterLayout() (for unit tests). */
juce::StringArray getRegisteredParameterIds();
}
