#include "MotionEngine.h"
#include <algorithm>
#include <cmath>

void MotionEngine::prepare (const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;
    reset();
    prepared = true;
}

void MotionEngine::reset()
{
    std::fill (std::begin (stutterBuffer), std::end (stutterBuffer), 0.f);
    std::fill (std::begin (freezeRing), std::end (freezeRing), 0.f);
    stutterLen = 0;
    stutterPos = 0;
    stutterRepeatsLeft = 0;
    freezeWrite = 0;
    freezeRead = 0;
    freezeActive = false;
    halfTimePhase = 0.f;
    rng.setSeedRandomly();
}

void MotionEngine::process (juce::AudioBuffer<float>& buffer,
                             const EngineState& state,
                             double hostBpm) noexcept
{
    juce::ignoreUnused (hostBpm);
    if (! prepared)
        return;

    const int n = buffer.getNumSamples();
    if (n <= 0)
        return;

    float* L = buffer.getWritePointer (0);
    float* R = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : L;

    const auto mode = state.performance.mode;
    const auto& perf = state.performance;

    if (mode == PerformanceMode::Stutter || perf.stutter)
    {
        if (stutterRepeatsLeft <= 0 && n > 0)
        {
            stutterLen = juce::jmin (kStutterHoldSamples, n);
            for (int i = 0; i < stutterLen; ++i)
                stutterBuffer[i] = L[i];
            stutterPos = 0;
            stutterRepeatsLeft = 8;
        }

        for (int i = 0; i < n; ++i)
        {
            if (stutterLen > 0)
            {
                L[i] = stutterBuffer[stutterPos];
                R[i] = stutterBuffer[stutterPos];
                ++stutterPos;
                if (stutterPos >= stutterLen)
                {
                    stutterPos = 0;
                    --stutterRepeatsLeft;
                }
            }
        }
    }

    if (mode == PerformanceMode::Reverse || perf.reverse)
    {
        for (int i = 0; i < n / 2; ++i)
        {
            std::swap (L[i], L[n - 1 - i]);
            std::swap (R[i], R[n - 1 - i]);
        }
    }

    if (mode == PerformanceMode::Gate || mode == PerformanceMode::Chop)
    {
        const float gateRate = juce::jmap (static_cast<float> (state.chop.rateIndex), 0.f, 3.f, 4.f, 32.f);
        const double beatsPerSec = juce::jmax (20.0, hostBpm) / 60.0;
        const double period = sampleRate / (beatsPerSec * gateRate / 4.0);
        for (int i = 0; i < n; ++i)
        {
            const float phase = static_cast<float> (std::fmod (halfTimePhase + i, period) / period);
            const float g = phase < 0.5f ? 1.f : (mode == PerformanceMode::Gate ? 0.1f : 0.5f);
            L[i] *= g;
            R[i] *= g;
        }
        halfTimePhase += static_cast<float> (n);
    }

    if (mode == PerformanceMode::HalfTime || perf.halfTime)
    {
        for (int i = 1; i < n; ++i)
        {
            L[i] = L[i / 2];
            R[i] = R[i / 2];
        }
    }

    if (mode == PerformanceMode::Scatter || perf.scatter)
    {
        for (int i = 0; i < n; ++i)
        {
            if (rng.nextFloat() < state.chop.random * 0.01f)
            {
                const int j = rng.nextInt (n);
                L[i] = L[j];
                R[i] = R[j];
            }
        }
    }

    if (mode == PerformanceMode::Freeze || perf.freeze || state.texture.freeze)
    {
        if (! freezeActive)
        {
            freezeActive = true;
            freezeWrite = 0;
            freezeRead = 0;
        }

        for (int i = 0; i < n; ++i)
        {
            freezeRing[freezeWrite % kFreezeRingSize] = L[i];
            ++freezeWrite;
            L[i] = freezeRing[freezeRead % kFreezeRingSize];
            R[i] = freezeRing[freezeRead % kFreezeRingSize];
            ++freezeRead;
        }
    }
    else
    {
        freezeActive = false;
    }
}
