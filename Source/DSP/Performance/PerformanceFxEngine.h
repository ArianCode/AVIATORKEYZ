#pragma once

#include "PerformanceTypes.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

struct PerformanceFxState
{
    float filterSweepAmount = 0.f;
    float pitchDropSemitones = 0.f;
    float tapeStopFactor = 1.f;
};

class PerformanceFxEngine
{
public:
    void prepare (const juce::dsp::ProcessSpec& spec);
    void reset();

    PerformanceFxState update (const PerformanceSettings& settings) noexcept;

    void process (juce::AudioBuffer<float>& buffer,
                  PerformanceFxState& fxState) noexcept;

private:
    double sampleRate = 44100.0;
    int tapeStopSamplesLeft = 0;
    int pitchDropSamplesLeft = 0;
    int filterSweepSamplesLeft = 0;
    bool prepared = false;
};
