#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

/** Three independent LFOs for the Advanced modulation engine. */
class LfoEngine
{
public:
    LfoEngine() = default;
    enum class Shape : int
    {
        sine = 0,
        square,
        triangle,
        rampUp,
        rampDown,
        random
    };

    static constexpr int kNumLfos = 3;

    void prepare (double sampleRate);
    void reset();

    void setRateHz (int lfoIndex, float rateHz);
    void setDepth (int lfoIndex, float depth01);
    void setShape (int lfoIndex, Shape shape);
    void setSyncToHost (int lfoIndex, bool sync, double hostBpm, int syncDivision = 4);
    void setPhaseOffset (int lfoIndex, float phase01);

    /** Advance all LFOs by numSamples; call once per processBlock. */
    void advance (int numSamples);

    /** Current bipolar output scaled by depth, range roughly [-depth, +depth]. */
    float getValue (int lfoIndex) const noexcept;

private:
    static float shapeSample (Shape shape, float phase01, float& randomHold, uint32_t& rng) noexcept;

    double sampleRate { 44100.0 };
    float rateHz[kNumLfos] {};
    float depth[kNumLfos] {};
    Shape shapes[kNumLfos] {};
    bool syncEnabled[kNumLfos] {};
    float phaseOffset[kNumLfos] {};
    float phase[kNumLfos] {};
    float output[kNumLfos] {};
    float randomHold[kNumLfos] {};
    uint32_t rngState[kNumLfos] { 0x12345678u, 0x9abcdef0u, 0x0fedcba9u };
    double hostBpm { 120.0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LfoEngine)
};
