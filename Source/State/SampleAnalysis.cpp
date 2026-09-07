#include "SampleAnalysis.h"
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <cmath>
#include <vector>

namespace SampleAnalysis
{
namespace
{
constexpr double kMaxAnalysisSeconds = 30.0;

// Krumhansl–Kessler key profiles
constexpr std::array<float, 12> kMajorProfile { 6.35f, 2.23f, 3.48f, 2.33f, 4.38f, 4.09f,
                                                2.52f, 5.19f, 2.39f, 3.66f, 2.29f, 2.88f };
constexpr std::array<float, 12> kMinorProfile { 6.33f, 2.68f, 3.52f, 5.38f, 2.60f, 3.53f,
                                                2.54f, 4.75f, 3.98f, 2.69f, 3.34f, 3.17f };

float pearson (const std::array<float, 12>& a, const std::array<float, 12>& b, int rotation)
{
    float meanA = 0.f, meanB = 0.f;
    for (int i = 0; i < 12; ++i) { meanA += a[(size_t) i]; meanB += b[(size_t) i]; }
    meanA /= 12.f; meanB /= 12.f;

    float num = 0.f, da = 0.f, db = 0.f;
    for (int i = 0; i < 12; ++i)
    {
        const float x = a[(size_t) ((i + rotation) % 12)] - meanA;
        const float y = b[(size_t) i] - meanB;
        num += x * y; da += x * x; db += y * y;
    }
    const float den = std::sqrt (da * db);
    return den > 1.0e-9f ? num / den : 0.f;
}

void estimateTempo (const float* mono, int numFrames, double sampleRate, Result& out)
{
    const int hop = juce::jmax (1, (int) std::lround (sampleRate / 100.0)); // 10 ms frames
    const int numEnvFrames = numFrames / hop;
    if (numEnvFrames < 200) // < 2 s of audio: tempo is meaningless
        return;

    std::vector<float> energy ((size_t) numEnvFrames, 0.f);
    for (int f = 0; f < numEnvFrames; ++f)
    {
        const float* p = mono + (size_t) f * (size_t) hop;
        float acc = 0.f;
        for (int i = 0; i < hop; ++i)
            acc += p[i] * p[i];
        energy[(size_t) f] = std::sqrt (acc / (float) hop);
    }

    // onset strength: half-wave rectified log-energy difference
    std::vector<float> onset ((size_t) numEnvFrames, 0.f);
    float meanOnset = 0.f;
    for (int f = 1; f < numEnvFrames; ++f)
    {
        const float d = std::log1p (energy[(size_t) f] * 40.f) - std::log1p (energy[(size_t) (f - 1)] * 40.f);
        onset[(size_t) f] = d > 0.f ? d : 0.f;
        meanOnset += onset[(size_t) f];
    }
    meanOnset /= (float) numEnvFrames;
    for (auto& v : onset)
        v -= meanOnset;

    // autocorrelation over 200 → 60 BPM (lags 30 .. 100 frames of 10 ms)
    const int minLag = 30, maxLag = 100;
    std::vector<float> ac ((size_t) maxLag + 1, 0.f);
    float ac0 = 0.f;
    for (int f = 0; f < numEnvFrames; ++f)
        ac0 += onset[(size_t) f] * onset[(size_t) f];
    if (ac0 <= 1.0e-9f)
        return;

    for (int lag = minLag; lag <= maxLag; ++lag)
    {
        float s = 0.f;
        for (int f = lag; f < numEnvFrames; ++f)
            s += onset[(size_t) f] * onset[(size_t) (f - lag)];
        ac[(size_t) lag] = s / ac0;
    }

    // Prefer lags whose half and double also correlate (beat vs. bar/8th ambiguity),
    // with a gentle prior towards 80–150 BPM.
    int bestLag = -1;
    float bestScore = -1.f;
    for (int lag = minLag; lag <= maxLag; ++lag)
    {
        const float bpm = 6000.f / (float) lag;
        float score = ac[(size_t) lag];
        if (lag * 2 <= maxLag) score += 0.5f * ac[(size_t) (lag * 2)];
        if (lag / 2 >= minLag) score += 0.25f * ac[(size_t) (lag / 2)];
        const float prior = std::exp (-0.5f * juce::square ((bpm - 115.f) / 45.f));
        score *= 0.6f + 0.4f * prior;
        if (score > bestScore)
        {
            bestScore = score;
            bestLag = lag;
        }
    }

    if (bestLag <= 0)
        return;

    // refine with parabolic interpolation around the peak
    float lagF = (float) bestLag;
    if (bestLag > minLag && bestLag < maxLag)
    {
        const float y0 = ac[(size_t) (bestLag - 1)], y1 = ac[(size_t) bestLag], y2 = ac[(size_t) (bestLag + 1)];
        const float den = y0 - 2.f * y1 + y2;
        if (std::abs (den) > 1.0e-6f)
            lagF += 0.5f * (y0 - y2) / den;
    }

    float bpm = 6000.f / lagF;
    while (bpm > 180.f) bpm *= 0.5f;
    while (bpm < 65.f)  bpm *= 2.f;

    out.bpm = std::round (bpm * 2.f) * 0.5f; // nearest 0.5 BPM
    out.bpmConfidence = juce::jlimit (0.f, 1.f, ac[(size_t) bestLag] * 2.f);
}

void estimateKey (const float* mono, int numFrames, double sampleRate, Result& out)
{
    constexpr int order = 12;
    constexpr int fftSize = 1 << order;
    if (numFrames < fftSize)
        return;

    juce::dsp::FFT fft (order);
    juce::dsp::WindowingFunction<float> window ((size_t) fftSize,
                                                juce::dsp::WindowingFunction<float>::hann);
    std::vector<float> frame ((size_t) fftSize * 2, 0.f);
    std::array<float, 12> chroma {};

    const int hop = fftSize / 2;
    const float binHz = (float) (sampleRate / fftSize);
    const int minBin = juce::jmax (1, (int) (55.f / binHz));
    const int maxBin = juce::jmin (fftSize / 2 - 1, (int) (4000.f / binHz));

    for (int start = 0; start + fftSize <= numFrames; start += hop)
    {
        std::fill (frame.begin(), frame.end(), 0.f);
        std::copy (mono + start, mono + start + fftSize, frame.begin());
        window.multiplyWithWindowingTable (frame.data(), (size_t) fftSize);
        fft.performFrequencyOnlyForwardTransform (frame.data());

        for (int b = minBin; b <= maxBin; ++b)
        {
            const float mag = frame[(size_t) b];
            if (mag <= 1.0e-6f)
                continue;
            const float hz = (float) b * binHz;
            const float midi = 69.f + 12.f * std::log2 (hz / 440.f);
            const int pc = ((int) std::lround (midi) % 12 + 12) % 12;
            chroma[(size_t) pc] += std::sqrt (mag); // compress dynamics
        }
    }

    float total = 0.f;
    for (float c : chroma) total += c;
    if (total <= 1.0e-6f)
        return;

    float best = -2.f, second = -2.f;
    int bestKey = -1;
    bool bestMinor = false;
    for (int key = 0; key < 12; ++key)
    {
        const float cMaj = pearson (chroma, kMajorProfile, key);
        const float cMin = pearson (chroma, kMinorProfile, key);
        for (int m = 0; m < 2; ++m)
        {
            const float c = m == 0 ? cMaj : cMin;
            if (c > best)
            {
                second = best;
                best = c;
                bestKey = key;
                bestMinor = m == 1;
            }
            else if (c > second)
            {
                second = c;
            }
        }
    }

    out.keyPitchClass = bestKey;
    out.keyMinor = bestMinor;
    out.keyConfidence = juce::jlimit (0.f, 1.f, (best - second) * 4.f + 0.2f);
}
} // namespace

Result analyzeMono (const float* mono, int numFrames, double sampleRate)
{
    Result r;
    if (mono == nullptr || numFrames <= 0 || sampleRate <= 0.0)
        return r;

    const int maxFrames = (int) juce::jmin ((double) numFrames, kMaxAnalysisSeconds * sampleRate);
    estimateTempo (mono, maxFrames, sampleRate, r);
    estimateKey (mono, maxFrames, sampleRate, r);
    return r;
}

juce::String keyName (int pitchClass, bool minor)
{
    static const char* names[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    if (pitchClass < 0 || pitchClass > 11)
        return juce::String::fromUTF8 ("\xe2\x80\x94");
    return juce::String (names[pitchClass]) + (minor ? " min" : " maj");
}

int rootNoteForPitchClass (int pitchClass)
{
    if (pitchClass < 0 || pitchClass > 11)
        return 60;
    return pitchClass <= 6 ? 60 + pitchClass : 48 + pitchClass;
}
} // namespace SampleAnalysis
