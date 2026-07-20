#pragma once

#include "PerformanceTypes.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

class MotionEngine
{
public:
    void prepare (const juce::dsp::ProcessSpec& spec);
    void reset();

    void process (juce::AudioBuffer<float>& buffer,
                  const EngineState& state,
                  double hostBpm) noexcept;

private:
    static constexpr int kStutterHoldSamples = 2048;
    static constexpr int kFreezeRingSize = 8192;

    double sampleRate = 44100.0;
    float stutterBuffer[8192] {};
    int stutterLen = 0;
    int stutterPos = 0;
    int stutterRepeatsLeft = 0;
    float freezeRing[8192] {};
    int freezeWrite = 0;
    int freezeRead = 0;
    bool freezeActive = false;
    float halfTimePhase = 0.f;
    juce::Random rng;
    bool prepared = false;
};
