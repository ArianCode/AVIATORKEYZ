#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

/** Post-reverb FX: delay, chorus, lo-fi decimator, soft distortion. */
class FxChain
{
public:
    FxChain() = default;
    void prepare (const juce::dsp::ProcessSpec& spec);
    void reset();

    void process (juce::AudioBuffer<float>& buffer,
                  bool delayOn, float delayTimeSec, float delayFeedback, float delayMix, bool delaySync,
                  bool chorusOn, float chorusRate, float chorusDepth, float chorusMix,
                  bool lofiOn, float lofiAmount,
                  bool distOn, float distDrive,
                  double hostBpm);

private:
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> delayL { 96000 };
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> delayR { 96000 };
    juce::dsp::Chorus<float> chorus;
    juce::dsp::ProcessSpec spec {};
    bool prepared { false };

    float lofiPhaseL { 0.f };
    float lofiPhaseR { 0.f };
    float lofiHoldL { 0.f };
    float lofiHoldR { 0.f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FxChain)
};
