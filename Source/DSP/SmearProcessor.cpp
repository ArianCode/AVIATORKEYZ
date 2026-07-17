#include "SmearProcessor.h"

namespace
{
constexpr float kMinCutoffHz = 80.f;
constexpr float kMaxCutoffHz = 16000.f;
constexpr float kResonance = 0.707f;
} // namespace

SmearProcessor::SmearProcessor()  = default;
SmearProcessor::~SmearProcessor() = default;

float SmearProcessor::cutoffHzForAmount (float amount01) noexcept
{
    const float amount = juce::jlimit (0.f, 1.f, amount01);
    const float logMin = std::log (kMinCutoffHz);
    const float logMax = std::log (kMaxCutoffHz);
    // 0 = open (high cutoff), 1 = aggressive low-cut
    return std::exp (juce::jmap (amount, 0.f, 1.f, logMax, logMin));
}

void SmearProcessor::prepare (const juce::dsp::ProcessSpec& s)
{
    spec = s;
    filterL.prepare (s);
    filterR.prepare (s);
    filterL.setType (juce::dsp::StateVariableTPTFilterType::highpass);
    filterR.setType (juce::dsp::StateVariableTPTFilterType::highpass);
    filterL.setCutoffFrequency (kMaxCutoffHz);
    filterR.setCutoffFrequency (kMaxCutoffHz);
    filterL.setResonance (kResonance);
    filterR.setResonance (kResonance);
    lastAmount = -1.f;
    reset();
    prepared = true;
}

void SmearProcessor::reset()
{
    filterL.reset();
    filterR.reset();
    lastAmount = -1.f;
}

void SmearProcessor::updateCutoff (float amount01) noexcept
{
    const float cutoffHz = cutoffHzForAmount (amount01);
    filterL.setCutoffFrequency (cutoffHz);
    filterR.setCutoffFrequency (cutoffHz);
    filterL.setResonance (kResonance);
    filterR.setResonance (kResonance);
    lastAmount = amount01;
}

void SmearProcessor::process (juce::AudioBuffer<float>& buffer, float amount01)
{
    if (! prepared || buffer.getNumChannels() < 2)
        return;

    if (amount01 < 0.001f)
        return;

    if (std::abs (amount01 - lastAmount) > 0.002f || lastAmount < 0.f)
        updateCutoff (amount01);

    const float wet = juce::jlimit (0.f, 1.f, amount01);
    const float dry = 1.f - wet;

    auto* L = buffer.getWritePointer (0);
    auto* R = buffer.getWritePointer (1);
    const int n = buffer.getNumSamples();

    for (int i = 0; i < n; ++i)
    {
        const float inL = L[i];
        const float inR = R[i];
        const float fL = filterL.processSample (0, inL);
        const float fR = filterR.processSample (0, inR);
        L[i] = inL * dry + fL * wet;
        R[i] = inR * dry + fR * wet;
    }
}
