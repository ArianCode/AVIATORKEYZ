#include "TextureEngine.h"
#include "FastMath.h"
#include <cmath>
#include <cstring>

void TextureEngine::prepare (const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;
    captureLength = juce::jmax (1, static_cast<int> (sampleRate * kMaxBufferSec));
    captureL.malloc (static_cast<size_t> (captureLength));
    captureR.malloc (static_cast<size_t> (captureLength));
    reset();
    prepared = true;
}

void TextureEngine::reset()
{
    const auto bytes = static_cast<size_t> (captureLength) * sizeof (float);
    std::memset (captureL.getData(), 0, bytes);
    std::memset (captureR.getData(), 0, bytes);
    writeIndex = 0;
    frozenWriteIndex = 0;
    frozen = false;
    spawnAccumulator = 0.f;
    scanPos = 0.f;
    driftPhase = 0.f;

    for (int i = 0; i < kMaxGrains; ++i)
        grains[i].active = false;
}

float TextureEngine::tempoGrainRateHz (float rate01, double bpm) const noexcept
{
    const double beatsPerSec = juce::jmax (20.0, bpm) / 60.0;
    const double div = juce::jmap (static_cast<double> (rate01), 0.25, 8.0);
    return static_cast<float> (beatsPerSec / div);
}

float TextureEngine::grainLengthSamples (float size01) const noexcept
{
    const float ms = juce::jmap (size01, 8.f, 120.f);
    return static_cast<float> (sampleRate) * ms * 0.001f;
}

float TextureEngine::readBuffer (float pos, int channel) const noexcept
{
    if (captureLength <= 1)
        return 0.f;

    while (pos < 0.f)
        pos += static_cast<float> (captureLength);
    while (pos >= static_cast<float> (captureLength))
        pos -= static_cast<float> (captureLength);

    const int i0 = static_cast<int> (pos) % captureLength;
    const float frac = pos - static_cast<float> (i0);
    const float* buf = channel == 0 ? captureL.getData() : captureR.getData();

    // Do not interpolate across the ring seam — i0 and index 0 are ~4 s apart in time.
    if (i0 + 1 >= captureLength)
        return buf[static_cast<size_t> (i0)];

    const int i1 = i0 + 1;
    return buf[static_cast<size_t> (i0)] + frac * (buf[static_cast<size_t> (i1)] - buf[static_cast<size_t> (i0)]);
}

void TextureEngine::writeCapture (const juce::AudioBuffer<float>& buffer) noexcept
{
    if (captureLength <= 0)
        return;

    const int n = buffer.getNumSamples();
    const float* inL = buffer.getReadPointer (0);
    const float* inR = buffer.getNumChannels() > 1 ? buffer.getReadPointer (1) : inL;

    float* capL = captureL.getData();
    float* capR = captureR.getData();

    if (frozen)
    {
        int w = frozenWriteIndex;
        for (int i = 0; i < n; ++i)
        {
            capL[static_cast<size_t> (w)] = inL[i];
            capR[static_cast<size_t> (w)] = inR[i];
            w = (w + 1) % captureLength;
        }
        frozenWriteIndex = w;
        return;
    }

    int w = writeIndex;
    for (int i = 0; i < n; ++i)
    {
        capL[static_cast<size_t> (w)] = inL[i];
        capR[static_cast<size_t> (w)] = inR[i];
        w = (w + 1) % captureLength;
    }
    writeIndex = w;
}

void TextureEngine::spawnGrain (float rate01,
                                float size01,
                                int pitchSemis,
                                float spread01,
                                float pan01,
                                bool reverse,
                                float scan01) noexcept
{
    for (int i = 0; i < kMaxGrains; ++i)
    {
        auto& g = grains[i];
        if (g.active)
            continue;

        const float len = juce::jmax (32.f, grainLengthSamples (size01));
        const float spreadSamples = spread01 * static_cast<float> (captureLength) * 0.25f;
        const float anchor = frozen ? static_cast<float> (frozenWriteIndex)
                                    : static_cast<float> (writeIndex);
        const float scanOffset = scan01 * static_cast<float> (captureLength) * 0.5f;
        const float offset = (rng.nextFloat() * 2.f - 1.f) * spreadSamples;
        scanPos = anchor + offset + scanOffset;
        while (scanPos < 0.f)
            scanPos += static_cast<float> (captureLength);
        while (scanPos >= static_cast<float> (captureLength))
            scanPos -= static_cast<float> (captureLength);

        g.active = true;
        g.readPos = scanPos;
        g.windowPhase = 0.f;
        g.windowInc = 1.f / len;
        const float pitchRatio = AviatorFastMath::semitoneRatio (static_cast<float> (pitchSemis));
        g.increment = (reverse ? -1.f : 1.f) * pitchRatio;
        const float pan = juce::jlimit (-1.f, 1.f, pan01 + (rng.nextFloat() * 2.f - 1.f) * spread01 * 0.5f);
        AviatorFastMath::constantPowerPan (pan, g.panL, g.panR);
        g.gain = juce::jmap (rate01, 0.4f, 1.f);
        juce::ignoreUnused (size01);
        return;
    }
}

void TextureEngine::process (juce::AudioBuffer<float>& buffer,
                             bool enabled,
                             float wetMix,
                             bool freeze,
                             float grainRate,
                             float grainSize,
                             int grainPitchSemis,
                             float density,
                             float spread,
                             float pan,
                             float motion,
                             float drift,
                             float air,
                             bool grainReverse,
                             float stereoWidth,
                             float grainScan,
                             double hostBpm)
{
    if (! prepared || buffer.getNumChannels() < 2 || buffer.getNumSamples() <= 0)
        return;

    const float wet = juce::jlimit (0.f, 1.f, wetMix);
    const bool active = enabled && wet >= 0.001f;
    const float scan01 = juce::jlimit (0.f, 1.f, grainScan);

    if (active)
        writeCapture (buffer);

    if (! active)
    {
        for (int i = 0; i < kMaxGrains; ++i)
            grains[i].active = false;
        spawnAccumulator = 0.f;
        return;
    }

    if (freeze != frozen)
    {
        frozen = freeze;
        if (frozen)
            frozenWriteIndex = writeIndex;
    }

    const float dry = 1.f - wet;
    const float rateHz = tempoGrainRateHz (grainRate, hostBpm);
    const float spawnRate = rateHz * juce::jlimit (0.f, 1.f, density) * wet;
    const int n = buffer.getNumSamples();
    const float blockSec = static_cast<float> (n) / static_cast<float> (sampleRate);
    const float width = juce::jlimit (0.f, 1.f, stereoWidth);
    const float airScale = air * wet * 0.15f;

    spawnAccumulator += spawnRate * blockSec;
    while (spawnAccumulator >= 1.f)
    {
        spawnGrain (grainRate, grainSize, grainPitchSemis, spread, pan, grainReverse, scan01);
        spawnAccumulator -= 1.f;
    }

    driftPhase += drift * blockSec * 0.5f;
    scanPos += (motion + scan01 * 0.75f) * static_cast<float> (captureLength) * 0.001f * static_cast<float> (n);
    while (scanPos >= static_cast<float> (captureLength))
        scanPos -= static_cast<float> (captureLength);
    while (scanPos < 0.f)
        scanPos += static_cast<float> (captureLength);

    auto* L = buffer.getWritePointer (0);
    auto* R = buffer.getWritePointer (1);

    for (int s = 0; s < n; ++s)
    {
        const float dryL = L[s];
        const float dryR = R[s];
        float wetL = 0.f;
        float wetR = 0.f;

        for (int gi = 0; gi < kMaxGrains; ++gi)
        {
            auto& g = grains[gi];
            if (! g.active)
                continue;

            const float win = AviatorFastMath::hannWindow (g.windowPhase);
            const float sampleL = readBuffer (g.readPos, 0);
            const float sampleR = readBuffer (g.readPos, 1);
            const float grain = win * g.gain;
            wetL += sampleL * grain * g.panL;
            wetR += sampleR * grain * g.panR;

            g.readPos += g.increment;
            g.windowPhase += g.windowInc;

            if (g.readPos < 0.f)
                g.readPos += static_cast<float> (captureLength);
            if (g.readPos >= static_cast<float> (captureLength))
                g.readPos -= static_cast<float> (captureLength);

            if (g.windowPhase >= 1.f)
                g.active = false;
        }

        const float dryMag = juce::jmax (std::abs (dryL), std::abs (dryR));
        const float airAmt = airScale * juce::jmin (1.f, dryMag * 8.f);
        if (airAmt > 0.f)
        {
            wetL += airAmt * (rng.nextFloat() * 2.f - 1.f);
            wetR += airAmt * (rng.nextFloat() * 2.f - 1.f);
        }

        const float m = 0.5f * (wetL + wetR);
        const float side = 0.5f * (wetL - wetR) * width;
        wetL = m + side;
        wetR = m - side;

        const float driftMod = 1.f + std::sin (driftPhase + static_cast<float> (s) * 0.001f) * drift * 0.05f;
        L[s] = dryL * dry + wetL * wet * driftMod;
        R[s] = dryR * dry + wetR * wet * driftMod;
    }
}
