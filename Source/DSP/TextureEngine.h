#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

/**
 * Parallel granular texture engine.
 * Captures the dry source output into a rolling stereo buffer and generates
 * tempo-synced grains mixed in parallel with the dry signal.
 */
class TextureEngine
{
public:
    TextureEngine() = default;

    void prepare (const juce::dsp::ProcessSpec& spec);
    void reset();

    void process (juce::AudioBuffer<float>& buffer,
                  bool enabled,
                  float wetMix,
                  bool freeze,
                  float grainRate,
                  float grainSize,
                  int grainPitchSemis,
                  float density,
                  float spread,
                  float pan,
                  float motion,
                  float drift,
                  float air,
                  bool grainReverse,
                  float stereoWidth,
                  float grainScan,
                  double hostBpm);

private:
    struct Grain
    {
        bool active = false;
        float readPos = 0.f;
        float increment = 1.f;
        float windowPhase = 0.f;
        float windowInc = 0.f;
        float panL = 0.707f;
        float panR = 0.707f;
        float gain = 1.f;
    };

    static constexpr int kMaxGrains = 16;
    static constexpr float kMaxBufferSec = 4.f;

    void writeCapture (const juce::AudioBuffer<float>& buffer) noexcept;
    void spawnGrain (float rate01, float size01, int pitchSemis, float spread01, float pan01, bool reverse, float scan01) noexcept;
    float readBuffer (float pos, int channel) const noexcept;
    float tempoGrainRateHz (float rate01, double bpm) const noexcept;
    float grainLengthSamples (float size01) const noexcept;

    juce::HeapBlock<float> captureL;
    juce::HeapBlock<float> captureR;
    int captureLength = 0;
    int writeIndex = 0;
    bool frozen = false;
    int frozenWriteIndex = 0;

    Grain grains[kMaxGrains] {};
    double sampleRate { 44100.0 };
    float spawnAccumulator = 0.f;
    float scanPos = 0.f;
    float driftPhase = 0.f;
    juce::Random rng;
    bool prepared { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TextureEngine)
};
