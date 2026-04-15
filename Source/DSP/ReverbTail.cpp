#include "ReverbTail.h"

ReverbTail::ReverbTail()  = default;
ReverbTail::~ReverbTail() = default;

void ReverbTail::prepare (const juce::dsp::ProcessSpec& spec)
{
    reverb.reset();
    reverb.prepare (spec);
    prepared = true;
}

void ReverbTail::reset()
{
    reverb.reset();
}

void ReverbTail::process (juce::AudioBuffer<float>& buffer,
                           float reverbAmount,
                           float reverbSize)
{
    if (! prepared || buffer.getNumChannels() < 2)
        return;

    juce::dsp::Reverb::Parameters params;
    params.roomSize   = juce::jlimit (0.f, 1.f, reverbSize);
    params.damping    = 0.5f;
    params.wetLevel   = juce::jlimit (0.f, 1.f, reverbAmount);
    params.dryLevel   = 1.0f - params.wetLevel;
    params.width      = 1.0f;
    params.freezeMode = 0.0f;
    reverb.setParameters (params);

    juce::dsp::AudioBlock<float> block (buffer);
    juce::dsp::ProcessContextReplacing<float> ctx (block);
    reverb.process (ctx);
}
