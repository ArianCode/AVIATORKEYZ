#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <array>

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
    static float shapeSample (Shape shape, float phase01, float& randomHold) noexcept;

    double sampleRate { 44100.0 };
    std::array<float, kNumLfos> rateHz {};
    std::array<float, kNumLfos> depth {};
    std::array<Shape, kNumLfos> shapes {};
    std::array<bool, kNumLfos> syncEnabled {};
    std::array<float, kNumLfos> phaseOffset {};
    std::array<float, kNumLfos> phase {};
    std::array<float, kNumLfos> output {};
    std::array<float, kNumLfos> randomHold {};
    double hostBpm { 120.0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LfoEngine)
};
