#include "FilterProcessor.h"

void FilterProcessor::prepare (const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;
    filterL.prepare (spec);
    filterR.prepare (spec);
    filterL.setType (juce::dsp::StateVariableTPTFilterType::lowpass);
    filterR.setType (juce::dsp::StateVariableTPTFilterType::lowpass);
    reset();
    prepared = true;
}

void FilterProcessor::reset()
{
    filterL.reset();
    filterR.reset();
}

float FilterProcessor::applyDrive (float x, float drive01) noexcept
{
    const float drive = 1.f + drive01 * 4.f;
    return std::tanh (x * drive) / std::tanh (drive);
}

void FilterProcessor::setParameters (float cutoffHz,
                                     float resonance01,
                                     Type type,
                                     float drive,
                                     float envAmount,
                                     float envLevel01) noexcept
{
    if (! prepared)
        return;

    const float envMod = envAmount * envLevel01;
    const float modCutoff = juce::jlimit (20.f, 20000.f, cutoffHz * std::pow (2.f, envMod * 4.f));
    const float q = juce::jmap (juce::jlimit (0.f, 1.f, resonance01), 0.707f, 8.f);

    juce::dsp::StateVariableTPTFilterType juceType;
    switch (type)
    {
        case Type::highPass: juceType = juce::dsp::StateVariableTPTFilterType::highpass; break;
        case Type::bandPass: juceType = juce::dsp::StateVariableTPTFilterType::bandpass; break;
        case Type::notch:    juceType = juce::dsp::StateVariableTPTFilterType::bandpass; break;
        case Type::lowPass:
        default:             juceType = juce::dsp::StateVariableTPTFilterType::lowpass; break;
    }

    filterL.setType (juceType);
    filterR.setType (juceType);
    filterL.setCutoffFrequency (modCutoff);
    filterR.setCutoffFrequency (modCutoff);
    filterL.setResonance (q);
    filterR.setResonance (q);
    drive01 = juce::jlimit (0.f, 1.f, drive);
    currentType = type;
}

void FilterProcessor::process (juce::AudioBuffer<float>& buffer)
{
    if (! prepared || buffer.getNumChannels() < 2)
        return;

    auto* L = buffer.getWritePointer (0);
    auto* R = buffer.getWritePointer (1);
    const int n = buffer.getNumSamples();
    const bool notchMode = currentType == Type::notch;

    for (int i = 0; i < n; ++i)
    {
        const float inL = L[i];
        const float inR = R[i];
        float outL = filterL.processSample (0, inL);
        float outR = filterR.processSample (0, inR);

        if (notchMode)
        {
            outL = inL - outL;
            outR = inR - outR;
        }

        L[i] = applyDrive (outL, drive01);
        R[i] = applyDrive (outR, drive01);
    }
}
