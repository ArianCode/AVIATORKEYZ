#pragma once

#include "PerformanceTypes.h"
#include <array>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

class PhraseChopper
{
public:
    void prepare (const juce::dsp::ProcessSpec& spec);
    void reset();

    /** Advance step clock and fill playback state for sampler. */
    ChopPlaybackState updatePlayback (const EngineState& state,
                                      int sampleNumFrames,
                                      double hostBpm) noexcept;

    /** Apply rhythmic gate / crossfade on rendered buffer. */
    void process (juce::AudioBuffer<float>& buffer,
                  const EngineState& state,
                  double hostBpm) noexcept;

private:
    static constexpr int kStepsPerBar = 16;
    static constexpr std::array<float, 4> kRateDivisors { 4.f, 8.f, 16.f, 32.f };

    int stepIndexForClock (double hostBpm, int rateIndex, float swing, int& stepOut) const noexcept;
    float crossfadeGain (float smooth01, int samplesIntoStep, int stepLenSamples) const noexcept;

    double sampleRate = 44100.0;
    double sampleCounter = 0.0;
    int currentStep = 0;
    int previousStep = -1;
    float stepGainSmoothed = 1.f;
    juce::Random rng;
    bool prepared = false;
};
