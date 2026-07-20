#include "PerformanceTexturePipeline.h"

void PerformanceTexturePipeline::prepare (const juce::dsp::ProcessSpec& spec)
{
    phraseChopper.prepare (spec);
    motionEngine.prepare (spec);
    textureBlend.prepare (spec);
    performanceFx.prepare (spec);
    prepared = true;
}

void PerformanceTexturePipeline::reset()
{
    phraseChopper.reset();
    motionEngine.reset();
    textureBlend.reset();
    performanceFx.reset();
}

ChopPlaybackState PerformanceTexturePipeline::advanceChopPlayback (const EngineState& state,
                                                                    int sampleNumFrames,
                                                                    double hostBpm) noexcept
{
    return phraseChopper.updatePlayback (state, sampleNumFrames, hostBpm);
}

void PerformanceTexturePipeline::process (juce::AudioBuffer<float>& buffer,
                                           const EngineState& state,
                                           double hostBpm) noexcept
{
    if (! prepared)
        return;

    phraseChopper.process (buffer, state, hostBpm);
    motionEngine.process (buffer, state, hostBpm);
    textureBlend.process (buffer, state.texture, hostBpm);
}

PerformanceFxState PerformanceTexturePipeline::updatePerformanceFx (const EngineState& state) noexcept
{
    return performanceFx.update (state.performance);
}

void PerformanceTexturePipeline::applyPerformanceFx (juce::AudioBuffer<float>& buffer,
                                                      PerformanceFxState& fx) noexcept
{
    performanceFx.process (buffer, fx);
}
