#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

class SmearProcessor
{
public:
    SmearProcessor();
    ~SmearProcessor();

    void prepare (const juce::dsp::ProcessSpec& spec);
    void reset();

    void process (juce::AudioBuffer<float>& buffer, float smearValue);

private:
    bool prepared { false };
    juce::dsp::ProcessSpec spec {};
    int maxDelaySamples { 1 };
    juce::HeapBlock<float> delayL;
    juce::HeapBlock<float> delayR;
    int writeL { 0 };
    int writeR { 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SmearProcessor)
};
