#include "PhraseChopper.h"
#include <cmath>

void PhraseChopper::prepare (const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;
    reset();
    prepared = true;
}

void PhraseChopper::reset()
{
    sampleCounter = 0.0;
    currentStep = 0;
    previousStep = -1;
    stepGainSmoothed = 1.f;
    rng.setSeedRandomly();
}

int PhraseChopper::stepIndexForClock (double hostBpm, int rateIndex, float swing,
                                      int& stepOut) const noexcept
{
    const float div = kRateDivisors[static_cast<size_t> (juce::jlimit (0, 3, rateIndex))];
    const double beatsPerSec = juce::jmax (20.0, hostBpm) / 60.0;
    const double stepsPerSec = beatsPerSec * (div / 4.0);
    const double stepLen = 1.0 / stepsPerSec;
    const int step = static_cast<int> (std::floor (sampleCounter / stepLen)) % kStepsPerBar;

    float swingOffset = 0.f;
    if (swing > 0.001f && (step % 2) == 1)
        swingOffset = static_cast<float> (stepLen * swing * 0.5);

    stepOut = step;
    return juce::jmax (1, static_cast<int> (stepLen * sampleRate + swingOffset * sampleRate));
}

float PhraseChopper::crossfadeGain (float smooth01, int samplesIntoStep, int stepLenSamples) const noexcept
{
    const int fadeLen = juce::jmax (1, static_cast<int> (smooth01 * sampleRate * 0.05f));
    if (samplesIntoStep < fadeLen)
        return static_cast<float> (samplesIntoStep) / static_cast<float> (fadeLen);
    if (samplesIntoStep > stepLenSamples - fadeLen)
        return static_cast<float> (stepLenSamples - samplesIntoStep) / static_cast<float> (fadeLen);
    return 1.f;
}

ChopPlaybackState PhraseChopper::updatePlayback (const EngineState& state,
                                                  int sampleNumFrames,
                                                  double hostBpm) noexcept
{
    ChopPlaybackState out;
    const auto& chop = state.chop;
    const auto& src = state.source;

    if (! chop.enabled || chop.amount < 0.001f || sampleNumFrames <= 1)
        return out;

    int stepLenSamples = 0;
    int step = 0;
    stepLenSamples = stepIndexForClock (hostBpm, chop.rateIndex, chop.swing, step);

    if (chop.random > 0.001f && rng.nextFloat() < chop.random * 0.02f)
        step = rng.nextInt (kStepsPerBar);

    currentStep = step;
    const auto& stepData = chop.steps[static_cast<size_t> (step)];

    if (! stepData.enabled)
    {
        out.active = true;
        out.gateGain = 0.f;
        return out;
    }

    const int windowStart = static_cast<int> (src.start * static_cast<float> (sampleNumFrames - 1));
    const int windowEnd = static_cast<int> (src.end * static_cast<float> (sampleNumFrames - 1));
    const int windowLen = juce::jmax (1, windowEnd - windowStart);
    const int numSlices = kStepsPerBar;
    const int sliceLen = juce::jmax (1, windowLen / numSlices);
    const int sliceIdx = juce::jlimit (0, numSlices - 1, step);
    int sliceStart = windowStart + sliceIdx * sliceLen;
    int sliceEnd = juce::jmin (windowEnd, sliceStart + sliceLen);

    const int offsetFrames = static_cast<int> (stepData.sliceOffset * static_cast<float> (sliceLen) * 0.5f);
    sliceStart = juce::jlimit (windowStart, windowEnd - 1, sliceStart + offsetFrames);
    sliceEnd = juce::jmax (sliceStart + 1, juce::jmin (windowEnd, sliceEnd + offsetFrames));

    bool rev = stepData.reverse;
    if (! rev && chop.reverseChance > 0.001f && rng.nextFloat() < chop.reverseChance * 0.05f)
        rev = true;

    out.active = true;
    out.sliceStartFrame = sliceStart;
    out.sliceEndFrame = sliceEnd;
    out.stepReverse = rev;
    out.pitchOffsetSemis = stepData.pitchOffset;
    out.gateGain = stepData.volume * juce::jmap (chop.gate, 0.f, 1.f, 0.2f, 1.f) * chop.amount;

    juce::ignoreUnused (stepLenSamples);
    return out;
}

void PhraseChopper::process (juce::AudioBuffer<float>& buffer,
                              const EngineState& state,
                              double hostBpm) noexcept
{
    if (! prepared)
        return;

    const auto& chop = state.chop;
    if (! chop.enabled || chop.amount < 0.001f)
        return;

    const int n = buffer.getNumSamples();
    if (n <= 0)
        return;

    int stepLenSamples = 0;
    int step = 0;
    stepLenSamples = stepIndexForClock (hostBpm, chop.rateIndex, chop.swing, step);

    if (step != previousStep)
    {
        previousStep = step;
        const auto& stepData = chop.steps[static_cast<size_t> (step)];
        const float target = stepData.enabled
                                 ? stepData.volume * juce::jmap (chop.gate, 0.f, 1.f, 0.15f, 1.f) * chop.amount
                                 : 0.f;
        stepGainSmoothed = target;
    }

    const int samplesIntoStep = static_cast<int> (std::fmod (sampleCounter,
        static_cast<double> (stepLenSamples) / sampleRate * sampleRate));
    const float xf = crossfadeGain (chop.smooth, samplesIntoStep, stepLenSamples);

    float* L = buffer.getWritePointer (0);
    float* R = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : L;

    const float fadeInc = 1.f / static_cast<float> (juce::jmax (1, n));
    float gain = stepGainSmoothed * xf;

    for (int i = 0; i < n; ++i)
    {
        L[i] *= gain;
        R[i] *= gain;
        sampleCounter += 1.0;
        if ((i & 63) == 0)
            gain = stepGainSmoothed * crossfadeGain (chop.smooth,
                static_cast<int> (std::fmod (sampleCounter, static_cast<double> (stepLenSamples))),
                stepLenSamples);
    }
}
