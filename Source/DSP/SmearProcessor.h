#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

/** Macro highpass filter — amount 0 (bypass) to 1 (aggressive low-cut). */
class SmearProcessor
{
public:
    SmearProcessor();
    ~SmearProcessor();

    void prepare (const juce::dsp::ProcessSpec& spec);
    void reset();

    void process (juce::AudioBuffer<float>& buffer, float amount01);

private:
    static float cutoffHzForAmount (float amount01) noexcept;
    void updateCutoff (float amount01) noexcept;

    bool prepared { false };
    juce::dsp::ProcessSpec spec {};
    juce::dsp::StateVariableTPTFilter<float> filterL;
    juce::dsp::StateVariableTPTFilter<float> filterR;
    float lastAmount { -1.f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SmearProcessor)
};
