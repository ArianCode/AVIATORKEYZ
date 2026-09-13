#include "ReverbTail.h"
#include <cmath>

ReverbTail::ReverbTail()  = default;
ReverbTail::~ReverbTail() = default;

void ReverbTail::prepare (const juce::dsp::ProcessSpec& spec)
{
    engine.prepare (spec.sampleRate, (int) spec.maximumBlockSize);
    wet.setSize (2, (int) spec.maximumBlockSize, false, true, true);
    mix.reset (spec.sampleRate, 0.03);
    mix.setCurrentAndTargetValue (0.f);
    active = false;
    prepared = true;
}

void ReverbTail::reset()
{
    engine.reset();
    wet.clear();
    mix.setCurrentAndTargetValue (0.f);
    active = false;
}

void ReverbTail::process (juce::AudioBuffer<float>& buffer,
                           float reverbAmount,
                           float reverbSize,
                           bool reverbOn,
                           float damping,
                           AviationReverb::Algorithm algorithm,
                           AviationReverb::Color color)
{
    if (! prepared || buffer.getNumChannels() < 2)
        return;

    const int n = buffer.getNumSamples();
    if (n <= 0 || n > wet.getNumSamples())
        return;

    const float amount = reverbOn ? juce::jlimit (0.f, 1.f, reverbAmount) : 0.f;
    mix.setTargetValue (amount < 0.001f ? 0.f : amount);

    if (! mix.isSmoothing() && mix.getCurrentValue() < 0.001f)
    {
        if (active)
        {
            engine.reset();
            active = false;
        }
        return;
    }
    active = true;

    AviationReverb::Settings settings;
    settings.algorithm = algorithm;
    settings.color     = color;
    settings.size      = juce::jlimit (0.f, 1.f, reverbSize);
    settings.damping   = juce::jlimit (0.f, 1.f, damping);
    settings.width     = 1.0f;
    engine.setSettings (settings);

    float* L = buffer.getWritePointer (0);
    float* R = buffer.getWritePointer (1);
    float* wL = wet.getWritePointer (0);
    float* wR = wet.getWritePointer (1);
    engine.process (L, R, wL, wR, n);

    const float halfPi = juce::MathConstants<float>::halfPi;
    if (mix.isSmoothing())
    {
        for (int i = 0; i < n; ++i)
        {
            const float m = mix.getNextValue() * halfPi;
            const float dryGain = std::cos (m);
            const float wetGain = std::sin (m);
            L[i] = L[i] * dryGain + wL[i] * wetGain;
            R[i] = R[i] * dryGain + wR[i] * wetGain;
        }
    }
    else
    {
        const float m = mix.getCurrentValue() * halfPi;
        const float dryGain = std::cos (m);
        const float wetGain = std::sin (m);
        for (int i = 0; i < n; ++i)
        {
            L[i] = L[i] * dryGain + wL[i] * wetGain;
            R[i] = R[i] * dryGain + wR[i] * wetGain;
        }
    }
}
