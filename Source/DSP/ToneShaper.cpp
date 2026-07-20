#include "ToneShaper.h"

ToneShaper::ToneShaper()  = default;
ToneShaper::~ToneShaper() = default;

void ToneShaper::prepare (const juce::dsp::ProcessSpec& s)
{
    spec = s;
    juce::dsp::ProcessSpec mono = s;
    mono.numChannels = 1;

    // Allocate one shared Coefficients object per shelf pair here (message
    // thread). updateCoeffs() then rewrites them in place from the audio
    // thread with no heap allocation.
    lowShelfL.coefficients = juce::dsp::IIR::Coefficients<float>::makeLowShelf (s.sampleRate, 180.0f, 0.707f, 1.f);
    lowShelfR.coefficients = lowShelfL.coefficients;
    highShelfL.coefficients = juce::dsp::IIR::Coefficients<float>::makeHighShelf (s.sampleRate, 6500.0f, 0.707f, 1.f);
    highShelfR.coefficients = highShelfL.coefficients;

    lowShelfL.prepare (mono);
    lowShelfR.prepare (mono);
    highShelfL.prepare (mono);
    highShelfR.prepare (mono);

    lastTone = 999.f;
    updateCoeffs (0.f);
    lastTone = 0.f;

    prepared = true;
}

void ToneShaper::reset()
{
    lowShelfL.reset();
    lowShelfR.reset();
    highShelfL.reset();
    highShelfR.reset();
}

void ToneShaper::updateCoeffs (float toneValue)
{
    // Audio-thread safe: ArrayCoefficients returns a stack std::array and
    // Coefficients::operator= reuses the storage allocated in prepare().
    // L/R share one Coefficients object per shelf, so one write updates both.
    const double sr = spec.sampleRate;

    const float lowGainDb  = -toneValue * 5.f;
    const float highGainDb = toneValue * 5.f;

    const float lowG  = juce::Decibels::decibelsToGain (lowGainDb);
    const float highG = juce::Decibels::decibelsToGain (highGainDb);

    *lowShelfL.coefficients  = juce::dsp::IIR::ArrayCoefficients<float>::makeLowShelf (sr, 180.0f, 0.707f, lowG);
    *highShelfL.coefficients = juce::dsp::IIR::ArrayCoefficients<float>::makeHighShelf (sr, 6500.0f, 0.707f, highG);
}

void ToneShaper::process (juce::AudioBuffer<float>& buffer, float toneValue)
{
    if (! prepared || buffer.getNumChannels() < 2)
        return;

    if (std::abs (toneValue) < 0.001f)
        return;

    if (std::abs (toneValue - lastTone) > 0.002f)
    {
        updateCoeffs (toneValue);
        lastTone = toneValue;
    }

    auto* L = buffer.getWritePointer (0);
    auto* R = buffer.getWritePointer (1);
    const int n = buffer.getNumSamples();

    for (int i = 0; i < n; ++i)
    {
        float l = lowShelfL.processSample (L[i]);
        l = highShelfL.processSample (l);
        float r = lowShelfR.processSample (R[i]);
        r = highShelfR.processSample (r);
        L[i] = l;
        R[i] = r;
    }
}
