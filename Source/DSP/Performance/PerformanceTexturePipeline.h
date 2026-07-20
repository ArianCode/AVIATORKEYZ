#pragma once

#include "PerformanceTypes.h"
#include "PhraseChopper.h"
#include "MotionEngine.h"
#include "TextureBlendEngine.h"
#include "PerformanceFxEngine.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

class PerformanceTexturePipeline
{
public:
    void prepare (const juce::dsp::ProcessSpec& spec);
    void reset();

    ChopPlaybackState advanceChopPlayback (const EngineState& state,
                                           int sampleNumFrames,
                                           double hostBpm) noexcept;

    void process (juce::AudioBuffer<float>& buffer,
                  const EngineState& state,
                  double hostBpm) noexcept;

    PerformanceFxState updatePerformanceFx (const EngineState& state) noexcept;
    void applyPerformanceFx (juce::AudioBuffer<float>& buffer, PerformanceFxState& fx) noexcept;

private:
    PhraseChopper phraseChopper;
    MotionEngine motionEngine;
    TextureBlendEngine textureBlend;
    PerformanceFxEngine performanceFx;
    bool prepared = false;
};
