#include "PitchProbe.h"

#include <cmath>

namespace
{
float midiToHz (float midi) noexcept
{
    return 440.f * std::pow (2.f, (midi - 69.f) / 12.f);
}

float hzToMidi (float hz) noexcept
{
    if (hz <= 0.f)
        return 60.f;
    return 69.f + 12.f * std::log2 (hz / 440.f);
}
} // namespace

float PitchProbe::foldMidiNearTarget (float midi, int target) noexcept
{
    while (midi - static_cast<float> (target) > 6.f)
        midi -= 12.f;
    while (static_cast<float> (target) - midi > 6.f)
        midi += 12.f;
    return midi;
}

PitchProbeResult PitchProbe::analyzeMono (const float* data,
                                          int numSamples,
                                          double sampleRate,
                                          int targetMidiHint) noexcept
{
    PitchProbeResult result;
    if (data == nullptr || numSamples < 64 || sampleRate <= 0.0)
        return result;

    const int maxLag = juce::jmin (numSamples / 2,
                                   static_cast<int> (sampleRate / 50.0));
    const int minLag = juce::jmax (2, static_cast<int> (sampleRate / 2000.0));

    float bestCorr = 0.f;
    int bestLag = 0;

    for (int lag = minLag; lag < maxLag; ++lag)
    {
        float sum = 0.f;
        const int n = numSamples - lag;
        for (int i = 0; i < n; ++i)
            sum += data[i] * data[i + lag];

        const float corr = sum / static_cast<float> (n);
        if (corr > bestCorr)
        {
            bestCorr = corr;
            bestLag = lag;
        }
    }

    if (bestLag <= 0)
        return result;

    const float hz = static_cast<float> (sampleRate) / static_cast<float> (bestLag);
    result.detectedMidi = foldMidiNearTarget (hzToMidi (hz), targetMidiHint);
    result.confidence = juce::jlimit (0.f, 1.f, bestCorr * 4.f);
    return result;
}

int PitchProbe::correctBufferToRoot (juce::AudioBuffer<float>& monoBuffer,
                                     double sampleRate,
                                     int targetMidi) noexcept
{
    if (monoBuffer.getNumChannels() < 1 || monoBuffer.getNumSamples() < 64)
        return targetMidi;

    const auto* data = monoBuffer.getReadPointer (0);
    const int n = monoBuffer.getNumSamples();
    const auto before = analyzeMono (data, n, sampleRate, targetMidi);
    const float shift = static_cast<float> (targetMidi) - before.detectedMidi;

    if (std::abs (shift) < 0.01f)
        return targetMidi;

    const float ratio = std::pow (2.f, shift / 12.f);
    juce::AudioBuffer<float> out (1, n);
    out.clear();

    for (int i = 0; i < n; ++i)
    {
        const float srcPos = static_cast<float> (i) / ratio;
        const int i0 = juce::jlimit (0, n - 1, static_cast<int> (std::floor (srcPos)));
        const int i1 = juce::jmin (n - 1, i0 + 1);
        const float frac = srcPos - static_cast<float> (i0);
        out.setSample (0, i, data[i0] + frac * (data[i1] - data[i0]));
    }

    monoBuffer.makeCopyOf (out);
    return targetMidi;
}
