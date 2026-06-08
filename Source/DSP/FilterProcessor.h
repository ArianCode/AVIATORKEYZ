#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

/** Stereo state-variable filter with drive and ADSR-style envelope modulation. */
class FilterProcessor
{
public:
    enum class Type : int
    {
        lowPass = 0,
        highPass,
        bandPass,
        notch
    };

    FilterProcessor() = default;

    void prepare (const juce::dsp::ProcessSpec& spec);
    void reset();

    void setParameters (float cutoffHz,
                          float resonance01,
                          Type type,
                          float drive01,
                          float envAmount,
                          float envLevel01) noexcept;

    void process (juce::AudioBuffer<float>& buffer);

private:
    static float applyDrive (float x, float drive01) noexcept;

    juce::dsp::StateVariableTPTFilter<float> filterL;
    juce::dsp::StateVariableTPTFilter<float> filterR;
    double sampleRate { 44100.0 };
    float drive01 { 0.f };
    Type currentType { Type::lowPass };
    bool prepared { false };
};
