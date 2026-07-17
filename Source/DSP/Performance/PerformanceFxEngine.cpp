#include "PerformanceFxEngine.h"
#include <cmath>

void PerformanceFxEngine::prepare (const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;
    reset();
    prepared = true;
}

void PerformanceFxEngine::reset()
{
    tapeStopSamplesLeft = 0;
    pitchDropSamplesLeft = 0;
    filterSweepSamplesLeft = 0;
}

PerformanceFxState PerformanceFxEngine::update (const PerformanceSettings& settings) noexcept
{
    PerformanceFxState s;

    if (settings.tapeStop && tapeStopSamplesLeft <= 0)
        tapeStopSamplesLeft = static_cast<int> (sampleRate * 0.6);

    if (settings.pitchDrop && pitchDropSamplesLeft <= 0)
        pitchDropSamplesLeft = static_cast<int> (sampleRate * 0.4);

    if (settings.filterSweep && filterSweepSamplesLeft <= 0)
        filterSweepSamplesLeft = static_cast<int> (sampleRate * 0.8);

    if (tapeStopSamplesLeft > 0)
    {
        s.tapeStopFactor = static_cast<float> (tapeStopSamplesLeft)
                           / static_cast<float> (sampleRate * 0.6);
        --tapeStopSamplesLeft;
    }

    if (pitchDropSamplesLeft > 0)
    {
        s.pitchDropSemitones = -6.f * (1.f - static_cast<float> (pitchDropSamplesLeft)
                                       / static_cast<float> (sampleRate * 0.4));
        --pitchDropSamplesLeft;
    }

    if (filterSweepSamplesLeft > 0)
    {
        s.filterSweepAmount = 1.f - static_cast<float> (filterSweepSamplesLeft)
                            / static_cast<float> (sampleRate * 0.8);
        --filterSweepSamplesLeft;
    }

    return s;
}

void PerformanceFxEngine::process (juce::AudioBuffer<float>& buffer,
                                    PerformanceFxState& fxState) noexcept
{
    if (! prepared)
        return;

    const int n = buffer.getNumSamples();
    if (n <= 0)
        return;

    if (fxState.tapeStopFactor < 0.999f)
    {
        float* L = buffer.getWritePointer (0);
        float* R = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : L;
        for (int i = 0; i < n; ++i)
        {
            const float g = fxState.tapeStopFactor;
            L[i] *= g;
            R[i] *= g;
        }
    }

    if (std::abs (fxState.filterSweepAmount) > 0.001f)
    {
        const float darkening = fxState.filterSweepAmount * 0.35f;
        float* L = buffer.getWritePointer (0);
        float* R = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : L;
        float lpL = 0.f;
        float lpR = 0.f;
        const float coef = juce::jlimit (0.01f, 0.5f, darkening);
        for (int i = 0; i < n; ++i)
        {
            lpL += coef * (L[i] - lpL);
            lpR += coef * (R[i] - lpR);
            L[i] = lpL;
            R[i] = lpR;
        }
    }

    juce::ignoreUnused (fxState.pitchDropSemitones);
}
