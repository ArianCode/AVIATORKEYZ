#include "SmearProcessor.h"
#include <cmath>
#include <cstring>

SmearProcessor::SmearProcessor()  = default;
SmearProcessor::~SmearProcessor() = default;

void SmearProcessor::prepare (const juce::dsp::ProcessSpec& s)
{
    spec = s;
    maxDelaySamples = juce::jmax (8, static_cast<int> (std::ceil (0.085 * s.sampleRate)));
    delayL.malloc (static_cast<size_t> (maxDelaySamples));
    delayR.malloc (static_cast<size_t> (maxDelaySamples));
    reset();
    prepared = true;
}

void SmearProcessor::reset()
{
    const auto bytes = static_cast<size_t> (maxDelaySamples) * sizeof (float);
    std::memset (delayL.getData(), 0, bytes);
    std::memset (delayR.getData(), 0, bytes);
    writeL = writeR = 0;
}

void SmearProcessor::process (juce::AudioBuffer<float>& buffer, float smearValue)
{
    if (! prepared || buffer.getNumChannels() < 2)
        return;

    if (smearValue < 0.001f)
        return;

    auto* L = buffer.getWritePointer (0);
    auto* R = buffer.getWritePointer (1);
    const int n = buffer.getNumSamples();

    const int delay = juce::jlimit (1, maxDelaySamples - 1,
                                   static_cast<int> (smearValue * static_cast<float> (maxDelaySamples - 1)));
    const float dry = 1.f - juce::jlimit (0.f, 1.f, smearValue);
    const float wet = 1.f - dry;

    float* dL = delayL.getData();
    float* dR = delayR.getData();
    int wL = writeL;
    int wR = writeR;
    const int mask = maxDelaySamples;

    for (int i = 0; i < n; ++i)
    {
        const float inL = L[i];
        const float inR = R[i];

        const int rL = (wL - delay + mask) % mask;
        const int rR = (wR - delay + mask) % mask;

        const float tapL = dL[static_cast<size_t> (rL)];
        const float tapR = dR[static_cast<size_t> (rR)];

        dL[static_cast<size_t> (wL)] = inL;
        dR[static_cast<size_t> (wR)] = inR;

        wL = (wL + 1) % mask;
        wR = (wR + 1) % mask;

        L[i] = inL * dry + tapL * wet;
        R[i] = inR * dry + tapR * wet;
    }

    writeL = wL;
    writeR = wR;
}
