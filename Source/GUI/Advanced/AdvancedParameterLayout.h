#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>

namespace AdvancedParameterLayout
{
void appendParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params);
}
