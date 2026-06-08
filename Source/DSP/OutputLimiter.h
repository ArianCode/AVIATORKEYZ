#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

/** Simple output limiter / soft clipper. */
class OutputLimiter
{
public:
    OutputLimiter() = default;

    void prepare (const juce::dsp::ProcessSpec& spec);
    void reset();

    void process (juce::AudioBuffer<float>& buffer, bool enabled);

private:
    juce::dsp::Limiter<float> limiter;
    bool prepared { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OutputLimiter)
};
