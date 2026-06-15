#include "FxChain.h"

namespace
{
float syncedDelaySeconds (float normTime, double bpm)
{
    const double beatsPerSecond = juce::jmax (20.0, bpm) / 60.0;
    const int divisions[] { 1, 2, 3, 4, 6, 8, 12, 16 };
    const int idx = juce::jlimit (0, 7, static_cast<int> (normTime * 7.99f));
    return static_cast<float> (1.0 / (beatsPerSecond * static_cast<double> (divisions[idx])));
}
} // namespace

void FxChain::prepare (const juce::dsp::ProcessSpec& s)
{
    spec = s;
    delayL.prepare (s);
    delayR.prepare (s);
    delayL.setMaximumDelayInSamples (static_cast<int> (s.sampleRate * 2.0));
    delayR.setMaximumDelayInSamples (static_cast<int> (s.sampleRate * 2.0));
    chorus.prepare (s);
    chorus.reset();
    prepared = true;
}

void FxChain::reset()
{
    delayL.reset();
    delayR.reset();
    chorus.reset();
    lofiPhaseL = lofiPhaseR = 0.f;
    lofiHoldL = lofiHoldR = 0.f;
}

void FxChain::process (juce::AudioBuffer<float>& buffer,
                       bool delayOn, float delayTimeSec, float delayFeedback, float delayMix, bool delaySync,
                       bool chorusOn, float chorusRate, float chorusDepth, float chorusMix,
                       bool lofiOn, float lofiAmount,
                       bool distOn, float distDrive,
                       double hostBpm)
{
    if (! prepared || buffer.getNumChannels() < 2)
        return;

    const float dMix = juce::jlimit (0.f, 1.f, delayMix);
    const bool anyFx = (delayOn && dMix > 0.001f)
                       || (chorusOn && chorusMix > 0.001f)
                       || (lofiOn && lofiAmount > 0.001f)
                       || (distOn && distDrive > 0.001f);
    if (! anyFx)
        return;

    auto* L = buffer.getWritePointer (0);
    auto* R = buffer.getWritePointer (1);
    const int n = buffer.getNumSamples();

    const float fb = juce::jlimit (0.f, 0.95f, delayFeedback);
    float delaySec = juce::jlimit (0.01f, 2.f, delayTimeSec);
    if (delaySync)
        delaySec = syncedDelaySeconds (delayTimeSec / 2.f, hostBpm);

    const int delaySamples = juce::jlimit (1, static_cast<int> (spec.sampleRate * 2.0),
                                         static_cast<int> (delaySec * spec.sampleRate));

    if (delayOn && dMix > 0.001f)
    {
        delayL.setDelay (static_cast<float> (delaySamples));
        delayR.setDelay (static_cast<float> (delaySamples));

        for (int i = 0; i < n; ++i)
        {
            const float inL = L[i];
            const float inR = R[i];
            const float dl = delayL.popSample (0);
            const float dr = delayR.popSample (0);
            const float wetL = inL + dl * fb;
            const float wetR = inR + dr * fb;
            delayL.pushSample (0, wetL);
            delayR.pushSample (0, wetR);
            L[i] = inL * (1.f - dMix) + dl * dMix;
            R[i] = inR * (1.f - dMix) + dr * dMix;
        }
    }

    if (chorusOn && chorusMix > 0.001f)
    {
        chorus.setRate (juce::jmap (chorusRate, 0.f, 1.f, 0.1f, 8.f));
        chorus.setDepth (juce::jmap (chorusDepth, 0.f, 1.f, 0.f, 0.9f));
        chorus.setCentreDelay (7.f);
        chorus.setFeedback (0.15f);
        chorus.setMix (juce::jlimit (0.f, 1.f, chorusMix));

        juce::dsp::AudioBlock<float> block (buffer);
        juce::dsp::ProcessContextReplacing<float> ctx (block);
        chorus.process (ctx);
    }

    if (lofiOn && lofiAmount > 0.001f)
    {
        const float crush = juce::jmap (lofiAmount, 0.f, 1.f, 1.f, 32.f);
        const float holdRate = juce::jmap (lofiAmount, 0.f, 1.f, 1.f, 0.04f);

        for (int i = 0; i < n; ++i)
        {
            lofiPhaseL += holdRate;
            lofiPhaseR += holdRate * 1.07f;

            if (lofiPhaseL >= 1.f)
            {
                lofiPhaseL = 0.f;
                const float steps = crush;
                lofiHoldL = std::round (L[i] * steps) / steps;
            }
            if (lofiPhaseR >= 1.f)
            {
                lofiPhaseR = 0.f;
                const float steps = crush;
                lofiHoldR = std::round (R[i] * steps) / steps;
            }

            L[i] = juce::jmap (lofiAmount, L[i], lofiHoldL);
            R[i] = juce::jmap (lofiAmount, R[i], lofiHoldR);
        }
    }

    if (distOn && distDrive > 0.001f)
    {
        const float gain = 1.f + distDrive * 12.f;
        const float norm = 1.f / std::tanh (gain);
        for (int i = 0; i < n; ++i)
        {
            L[i] = std::tanh (L[i] * gain) * norm;
            R[i] = std::tanh (R[i] * gain) * norm;
        }
    }
}
