#pragma once

#include "PerformanceTypes.h"
#include "../TextureEngine.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

class TextureBlendEngine
{
public:
    void prepare (const juce::dsp::ProcessSpec& spec);
    void reset();

    void process (juce::AudioBuffer<float>& buffer,
                  const TextureSettings& settings,
                  double hostBpm) noexcept;

private:
    TextureEngine textureEngine;
    juce::AudioBuffer<float> dryCopy;
    bool prepared = false;
};
