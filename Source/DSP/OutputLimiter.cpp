#include "OutputLimiter.h"

void OutputLimiter::prepare (const juce::dsp::ProcessSpec& spec)
{
    limiter.prepare (spec);
    limiter.setThreshold (-0.5f);
    limiter.setRelease (80.f);
    reset();
    prepared = true;
}

void OutputLimiter::reset()
{
    limiter.reset();
}

void OutputLimiter::process (juce::AudioBuffer<float>& buffer, bool enabled)
{
    if (! prepared || ! enabled || buffer.getNumSamples() <= 0)
        return;

    juce::dsp::AudioBlock<float> block (buffer);
    juce::dsp::ProcessContextReplacing<float> ctx (block);
    limiter.process (ctx);
}
