#include "SmearProcessor.h"
#include <cmath>

SmearProcessor::SmearProcessor()  = default;
SmearProcessor::~SmearProcessor() = default;

void SmearProcessor::prepare (const juce::dsp::ProcessSpec& s)
{
    spec = s;
    maxDelaySamples = juce::jmax (8, static_cast<int> (std::ceil (0.085 * s.sampleRate)));
    delayL.assign (static_cast<size_t> (maxDelaySamples), 0.f);
    delayR.assign (static_cast<size_t> (maxDelaySamples), 0.f);
    writeL = writeR = 0;
    prepared = true;
}

void SmearProcessor::reset()
{
    std::fill (delayL.begin(), delayL.end(), 0.f);
    std::fill (delayR.begin(), delayR.end(), 0.f);
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
    const float wet = juce::jlimit (0.f, 1.f, smearValue);

    for (int i = 0; i < n; ++i)
    {
        const float inL = L[i];
        const float inR = R[i];

        int rL = writeL - delay;
        while (rL < 0) rL += maxDelaySamples;
        int rR = writeR - delay;
        while (rR < 0) rR += maxDelaySamples;

        const float dL = delayL[static_cast<size_t> (rL)];
        const float dR = delayR[static_cast<size_t> (rR)];

        delayL[static_cast<size_t> (writeL)] = inL;
        delayR[static_cast<size_t> (writeR)] = inR;

        writeL = (writeL + 1) % maxDelaySamples;
        writeR = (writeR + 1) % maxDelaySamples;

        L[i] = inL * (1.f - wet) + dL * wet;
        R[i] = inR * (1.f - wet) + dR * wet;
    }
}
