#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

class ToneShaper
{
public:
    ToneShaper();
    ~ToneShaper();

    void prepare (const juce::dsp::ProcessSpec& spec);
    void reset();

    void process (juce::AudioBuffer<float>& buffer, float toneValue);

private:
    void updateCoeffs (float toneValue);

    juce::dsp::ProcessSpec spec {};
    bool prepared { false };
    float lastTone { 0.f };

    juce::dsp::IIR::Filter<float> lowShelfL;
    juce::dsp::IIR::Filter<float> lowShelfR;
    juce::dsp::IIR::Filter<float> highShelfL;
    juce::dsp::IIR::Filter<float> highShelfR;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ToneShaper)
};
