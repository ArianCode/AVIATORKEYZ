#include "LfoEngine.h"
#include "FastMath.h"

namespace
{
float divisionToHz (double bpm, int division)
{
    const double beatsPerSecond = bpm / 60.0;
    const double div = juce::jmax (1, division);
    return static_cast<float> (beatsPerSecond / div);
}
} // namespace

void LfoEngine::prepare (double sr)
{
    sampleRate = juce::jmax (1.0, sr);
    reset();
}

void LfoEngine::reset()
{
    for (int i = 0; i < kNumLfos; ++i)
    {
        phase[i] = 0.f;
        output[i] = 0.f;
        randomHold[i] = 0.f;
    }
}

void LfoEngine::setRateHz (int lfoIndex, float hz)
{
    if (juce::isPositiveAndBelow (lfoIndex, kNumLfos))
        rateHz[lfoIndex] = juce::jlimit (0.01f, 20.f, hz);
}

void LfoEngine::setDepth (int lfoIndex, float depth01)
{
    if (juce::isPositiveAndBelow (lfoIndex, kNumLfos))
        depth[lfoIndex] = juce::jlimit (0.f, 1.f, depth01);
}

void LfoEngine::setShape (int lfoIndex, Shape shape)
{
    if (juce::isPositiveAndBelow (lfoIndex, kNumLfos))
        shapes[lfoIndex] = shape;
}

void LfoEngine::setPhaseOffset (int lfoIndex, float phase01)
{
    if (juce::isPositiveAndBelow (lfoIndex, kNumLfos))
        phaseOffset[lfoIndex] = juce::jlimit (0.f, 1.f, phase01);
}

void LfoEngine::setSyncToHost (int lfoIndex, bool sync, double bpm, int syncDivision)
{
    if (! juce::isPositiveAndBelow (lfoIndex, kNumLfos))
        return;

    hostBpm = juce::jmax (20.0, bpm);
    syncEnabled[lfoIndex] = sync;

    if (sync)
        rateHz[lfoIndex] = divisionToHz (hostBpm, syncDivision);
}

float LfoEngine::shapeSample (Shape shape, float phase01, float& randomHold, uint32_t& rng) noexcept
{
    switch (shape)
    {
        case Shape::sine:
            return AviatorFastMath::fastSinPhase01 (phase01);

        case Shape::square:
            return phase01 < 0.5f ? 1.f : -1.f;

        case Shape::triangle:
        {
            const float t = phase01 < 0.5f ? phase01 * 2.f : 2.f - phase01 * 2.f;
            return t * 2.f - 1.f;
        }

        case Shape::rampUp:
            return phase01 * 2.f - 1.f;

        case Shape::rampDown:
            return 1.f - phase01 * 2.f;

        case Shape::random:
        default:
            if (phase01 < 0.001f)
            {
                rng ^= rng << 13;
                rng ^= rng >> 17;
                rng ^= rng << 5;
                randomHold = (static_cast<float> (rng) / static_cast<float> (0x7fffffff)) * 2.f - 1.f;
            }
            return randomHold;
    }
}

void LfoEngine::advance (int numSamples)
{
    const float blockSec = static_cast<float> (numSamples) / static_cast<float> (sampleRate);

    for (int i = 0; i < kNumLfos; ++i)
    {
        float hz = rateHz[i];
        if (syncEnabled[i])
            hz = divisionToHz (hostBpm, 4);

        hz = juce::jlimit (0.01f, 20.f, hz);
        phase[i] += hz * blockSec;
        phase[i] = AviatorFastMath::wrapPhase01 (phase[i] + phaseOffset[i]);

        const float raw = shapeSample (shapes[i], phase[i], randomHold[i], rngState[i]);
        output[i] = raw * depth[i];
    }
}

float LfoEngine::getValue (int lfoIndex) const noexcept
{
    if (! juce::isPositiveAndBelow (lfoIndex, kNumLfos))
        return 0.f;

    return output[lfoIndex];
}
