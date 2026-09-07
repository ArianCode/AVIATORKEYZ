#include "MfxEffects.h"
#include <cmath>

namespace Mfx
{
namespace
{
constexpr float kPi = juce::MathConstants<float>::pi;
constexpr float kTwoPi = juce::MathConstants<float>::twoPi;

inline float pct (float v) noexcept { return juce::jlimit (0.f, 1.f, v * 0.01f); }

inline float onePoleCoef (float cutoffHz, double sampleRate) noexcept
{
    const float x = std::exp (-kTwoPi * juce::jlimit (5.f, (float) sampleRate * 0.45f, cutoffHz) / (float) sampleRate);
    return 1.f - x;
}

inline void copyDry (juce::AudioBuffer<float>& dry, const juce::AudioBuffer<float>& src) noexcept
{
    const int n = juce::jmin (dry.getNumSamples(), src.getNumSamples());
    for (int ch = 0; ch < 2; ++ch)
        dry.copyFrom (ch, 0, src, ch, 0, n);
}

/** Beats for a Tape Echo SYNC choice (0 = free). */
inline double syncBeats (int choice) noexcept
{
    switch (choice)
    {
        case 1: return 0.25;          // 1/16
        case 2: return 0.5;           // 1/8
        case 3: return 1.0 / 3.0;     // 1/8T
        case 4: return 1.0;           // 1/4
        case 5: return 1.5;           // 1/4D
        case 6: return 2.0;           // 1/2
        default: return 0.0;
    }
}

inline double divisionBeats (int choice) noexcept
{
    switch (choice)
    {
        case 0: return 1.0;
        case 1: return 0.5;
        case 2: return 0.25;
        default: return 0.125;
    }
}
} // namespace

void blendDry (juce::AudioBuffer<float>& wet, const juce::AudioBuffer<float>& dry, float mix) noexcept
{
    mix = juce::jlimit (0.f, 1.f, mix);
    const int n = juce::jmin (wet.getNumSamples(), dry.getNumSamples());
    for (int ch = 0; ch < 2; ++ch)
    {
        wet.applyGain (ch, 0, n, mix);
        wet.addFrom (ch, 0, dry, ch, 0, n, 1.f - mix);
    }
}

// =============================================================================
//  Grain Cloud
// =============================================================================
void GrainCloud::prepare (const juce::dsp::ProcessSpec& spec)
{
    engine.prepare (spec);
    dry.setSize (2, (int) spec.maximumBlockSize, false, true, true);
}

void GrainCloud::reset() { engine.reset(); }

void GrainCloud::process (juce::AudioBuffer<float>& buffer, const Values& v, const Clock& clock) noexcept
{
    const int n = buffer.getNumSamples();
    if (n <= 0 || n > dry.getNumSamples())
        return;
    // TextureEngine mixes dry/wet internally by wetMix.
    const float sizeNorm = juce::jlimit (0.f, 1.f, (v[0] - 5.f) / 495.f);
    engine.process (buffer,
                    true,
                    pct (v[10]),                 // mix
                    v[9] > 0.5f,                 // freeze
                    0.55f,                       // grain rate (tempo-relative, fixed)
                    sizeNorm,                    // size
                    juce::roundToInt (v[4]),     // pitch semis
                    pct (v[1]),                  // density
                    pct (v[3]),                  // spread
                    0.f,                         // pan
                    pct (v[5] * 0.5f + 50.f),    // motion from scan (-100..100 -> 0..1)
                    pct (v[6]),                  // drift
                    pct (v[8]) * 0.5f,           // air from smear
                    false,
                    pct (v[7]),                  // width
                    pct (v[2]),                  // scan position
                    clock.bpm);
}

// =============================================================================
//  Sweep Filter
// =============================================================================
void SweepFilter::prepare (const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;
    filter.prepare (spec);
    dry.setSize (2, (int) spec.maximumBlockSize, false, true, true);
    lfoPhase = 0.f;
}

void SweepFilter::reset() { filter.reset(); lfoPhase = 0.f; }

void SweepFilter::process (juce::AudioBuffer<float>& buffer, const Values& v, const Clock& clock) noexcept
{
    const int n = buffer.getNumSamples();
    if (n <= 0 || n > dry.getNumSamples())
        return;
    copyDry (dry, buffer);

    // LFO (block-rate) in octaves, plus envelope amount
    const float lfo = std::sin (lfoPhase * kTwoPi);
    lfoPhase += (float) (v[4] * n / clock.sampleRate);
    lfoPhase -= std::floor (lfoPhase);
    const float octaves = lfo * pct (v[5]) * 2.5f;
    const float cutoff = juce::jlimit (20.f, 18000.f, v[0] * std::pow (2.f, octaves));

    filter.setParameters (cutoff, pct (v[1]),
                          static_cast<FilterProcessor::Type> (juce::jlimit (0, 3, juce::roundToInt (v[2]))),
                          pct (v[3]),
                          v[6] * 0.01f,           // env amount -1..1
                          envLevel);
    filter.process (buffer);
    blendDry (buffer, dry, pct (v[7]));
}

// =============================================================================
//  Tape Echo
// =============================================================================
void TapeEcho::prepare (const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;
    lineLen = juce::jmax (16, (int) (kMaxDelaySec * sampleRate) + 64);
    line.setSize (2, lineLen, false, true, true);
    dry.setSize (2, (int) spec.maximumBlockSize, false, true, true);
    reset();
}

void TapeEcho::reset()
{
    line.clear();
    writePos = 0;
    wowPhase = 0.f;
    delaySmoothed = 0.f;
    fbLpL = fbLpR = fbHpL = fbHpR = fbHpInL = fbHpInR = 0.f;
}

void TapeEcho::process (juce::AudioBuffer<float>& buffer, const Values& v, const Clock& clock) noexcept
{
    const int n = buffer.getNumSamples();
    if (n <= 0 || n > dry.getNumSamples())
        return;
    copyDry (dry, buffer);

    double timeSec = v[0] * 0.001;
    const double beats = syncBeats (juce::roundToInt (v[1]));
    if (beats > 0.0)
        timeSec = beats * 60.0 / juce::jmax (20.0, clock.bpm);
    const float targetSamples = (float) juce::jlimit (1.0, kMaxDelaySec * sampleRate - 8.0, timeSec * sampleRate);
    if (delaySmoothed <= 0.f)
        delaySmoothed = targetSamples;

    const float fb = pct (v[2]) * 0.95f;
    const float wowDepth = pct (v[3]) * 0.006f * (float) sampleRate;   // up to 6 ms
    const float ageCoef = onePoleCoef (juce::jmap (pct (v[4]), 16000.f, 1800.f), sampleRate);
    const float hpCoef = onePoleCoef (v[5], sampleRate);
    const float spread = pct (v[6]);
    const float mix = pct (v[7]);
    const float wowInc = 0.45f / (float) sampleRate;
    const float smoothCoef = 0.0008f;

    float* L = buffer.getWritePointer (0);
    float* R = buffer.getWritePointer (1);
    float* lineL = line.getWritePointer (0);
    float* lineR = line.getWritePointer (1);

    for (int i = 0; i < n; ++i)
    {
        delaySmoothed += (targetSamples - delaySmoothed) * smoothCoef;
        const float wow = std::sin (wowPhase * kTwoPi) * wowDepth;
        wowPhase += wowInc;
        if (wowPhase >= 1.f) wowPhase -= 1.f;

        auto readAt = [&] (const float* src, float delaySamples) noexcept
        {
            float pos = (float) writePos - delaySamples;
            while (pos < 0.f) pos += (float) lineLen;
            const int i0 = (int) pos;
            const float frac = pos - (float) i0;
            const int i1 = (i0 + 1) % lineLen;
            return src[i0] + (src[i1] - src[i0]) * frac;
        };

        const float dL = juce::jlimit (1.f, (float) lineLen - 4.f, delaySmoothed + wow);
        const float dR = juce::jlimit (1.f, (float) lineLen - 4.f, delaySmoothed * (1.f + spread * 0.18f) - wow);
        const float wetL = readAt (lineL, dL);
        const float wetR = readAt (lineR, dR);

        // feedback path: age (LP) + lo cut (HP) + soft clip
        fbLpL += ageCoef * (wetL - fbLpL);
        fbLpR += ageCoef * (wetR - fbLpR);
        const float hpOutL = fbLpL - fbHpInL + (1.f - hpCoef) * fbHpL; fbHpInL = fbLpL; fbHpL = hpOutL;
        const float hpOutR = fbLpR - fbHpInR + (1.f - hpCoef) * fbHpR; fbHpInR = fbLpR; fbHpR = hpOutR;

        lineL[writePos] = std::tanh (L[i] + hpOutL * fb);
        lineR[writePos] = std::tanh (R[i] + hpOutR * fb);
        writePos = (writePos + 1) % lineLen;

        L[i] = wetL;
        R[i] = wetR;
    }

    blendDry (buffer, dry, mix);
}

// =============================================================================
//  Saturator
// =============================================================================
void Saturator::prepare (const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;
    dry.setSize (2, (int) spec.maximumBlockSize, false, true, true);
    reset();
}

void Saturator::reset() { toneStateL = toneStateR = dcL = dcR = dcInL = dcInR = 0.f; }

void Saturator::process (juce::AudioBuffer<float>& buffer, const Values& v, const Clock&) noexcept
{
    const int n = buffer.getNumSamples();
    if (n <= 0 || n > dry.getNumSamples())
        return;
    copyDry (dry, buffer);

    const float drive = 1.f + pct (v[0]) * 11.f;
    const float bias = v[1] * 0.01f * 0.6f;
    const float tone = v[2] * 0.01f;                      // -1 dark .. +1 bright
    const float out = juce::Decibels::decibelsToGain (v[3]);
    const float norm = 1.f / std::tanh (drive);
    const float lpCoef = onePoleCoef (juce::jmap (tone, -1.f, 1.f, 1200.f, 12000.f), sampleRate);
    const float dcCoef = 0.995f;

    float* ch[2] = { buffer.getWritePointer (0), buffer.getWritePointer (1) };
    float* toneState[2] = { &toneStateL, &toneStateR };
    float* dcSt[2] = { &dcL, &dcR };
    float* dcIn[2] = { &dcInL, &dcInR };

    for (int c = 0; c < 2; ++c)
    {
        float* s = ch[c];
        for (int i = 0; i < n; ++i)
        {
            float x = std::tanh ((s[i] + bias) * drive) * norm;
            // dc block after bias
            const float y = x - *dcIn[c] + dcCoef * *dcSt[c];
            *dcIn[c] = x; *dcSt[c] = y; x = y;
            // tone: blend towards a low-passed copy when dark, add presence when bright
            *toneState[c] += lpCoef * (x - *toneState[c]);
            x = tone < 0.f ? juce::jmap (-tone, x, *toneState[c]) : x + (x - *toneState[c]) * tone * 0.6f;
            s[i] = x * out;
        }
    }
    blendDry (buffer, dry, pct (v[4]));
}

// =============================================================================
//  Stutter
// =============================================================================
void Stutter::prepare (const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;
    captureLen = juce::jmax (256, (int) (kMaxCaptureSec * sampleRate));
    capture.setSize (2, captureLen, false, true, true);
    dry.setSize (2, (int) spec.maximumBlockSize, false, true, true);
    rng.setSeed (0x5754);
    reset();
}

void Stutter::reset()
{
    capture.clear();
    writePos = 0;
    loopStart = 0;
    loopLen = 1;
    readPos = 0.0;
    lastDivisionIndex = -1;
    repeatGain = 1.f;
}

void Stutter::process (juce::AudioBuffer<float>& buffer, const Values& v, const Clock& clock) noexcept
{
    const int n = buffer.getNumSamples();
    if (n <= 0 || n > dry.getNumSamples())
        return;
    copyDry (dry, buffer);

    const double divBeats = divisionBeats (juce::roundToInt (v[0]));
    const double beatsPerSample = clock.beatsPerSample();
    const int divSamples = juce::jmax (32, (int) (divBeats / beatsPerSample));
    const float gate = pct (v[1]);
    const float jitter = pct (v[2]);
    const double rate = std::pow (2.0, v[3] / 12.0);
    const float decay = pct (v[4]);
    const float mix = pct (v[5]);

    float* L = buffer.getWritePointer (0);
    float* R = buffer.getWritePointer (1);
    float* capL = capture.getWritePointer (0);
    float* capR = capture.getWritePointer (1);

    double beat = clock.beatPos;
    for (int i = 0; i < n; ++i)
    {
        // write dry into the capture ring
        capL[writePos] = dry.getSample (0, i);
        capR[writePos] = dry.getSample (1, i);

        // new division boundary -> latch the previous division as the loop
        const long long divIndex = (long long) std::floor (beat / divBeats + 1.0e-9);
        if (divIndex != lastDivisionIndex)
        {
            lastDivisionIndex = divIndex;
            loopLen = juce::jmin (divSamples, captureLen - 1);
            int start = writePos - loopLen;
            if (jitter > 0.001f)
                start += (int) ((rng.nextFloat() * 2.f - 1.f) * jitter * (float) loopLen * 0.5f);
            while (start < 0) start += captureLen;
            loopStart = start % captureLen;
            readPos = 0.0;
            repeatGain = 1.f;
        }

        // read the loop
        const double p = readPos;
        const int i0 = (loopStart + (int) p) % captureLen;
        const int i1 = (i0 + 1) % captureLen;
        const float frac = (float) (p - std::floor (p));
        float wetL = capL[i0] + (capL[i1] - capL[i0]) * frac;
        float wetR = capR[i0] + (capR[i1] - capR[i0]) * frac;

        // gate within each repeat + decay per repeat
        const float phaseInLoop = (float) (p / (double) loopLen);
        const float g = phaseInLoop < gate ? 1.f : 0.f;
        const float fade = juce::jmin (1.f, juce::jmin (phaseInLoop, gate - phaseInLoop) * 40.f);
        wetL *= g * repeatGain * juce::jmax (0.f, fade);
        wetR *= g * repeatGain * juce::jmax (0.f, fade);

        readPos += rate;
        if (readPos >= (double) loopLen)
        {
            readPos -= (double) loopLen;
            repeatGain *= 1.f - decay * 0.5f;
        }

        L[i] = wetL;
        R[i] = wetR;

        writePos = (writePos + 1) % captureLen;
        beat += beatsPerSample;
    }

    blendDry (buffer, dry, mix);
}

// =============================================================================
//  Freeze — two overlapping Hann grains looping the material just behind the
//  freeze point. HOLD = wet, SMEAR = grain length, DECAY = per-cycle fade
//  (100 % sustains forever), PITCH = read rate, WIDTH = L/R read offset.
// =============================================================================
void Freeze::prepare (const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;
    ringLen = juce::jmax (1024, (int) (kRingSec * sampleRate));
    ring.setSize (2, ringLen, false, true, true);
    dry.setSize (2, (int) spec.maximumBlockSize, false, true, true);
    warmTarget = (int) (sampleRate * 0.5);
    rng.setSeed (0x465a);
    reset();
}

void Freeze::reset()
{
    ring.clear();
    writePos = 0;
    frozen = false;
    freezePoint = 0.0;
    cycleGain = 1.f;
    warmSamples = 0;
    for (auto& g : grains)
        g = {};
}

float Freeze::readRing (int ch, double pos) const noexcept
{
    while (pos < 0.0) pos += (double) ringLen;
    while (pos >= (double) ringLen) pos -= (double) ringLen;
    const int i0 = (int) pos;
    const int i1 = (i0 + 1) % ringLen;
    const float frac = (float) (pos - (double) i0);
    const float* d = ring.getReadPointer (ch);
    return d[i0] + (d[i1] - d[i0]) * frac;
}

void Freeze::spawn (Grain& g, int grainLen, double anchor, float startPhase) noexcept
{
    g.len = grainLen;
    const double jitter = (rng.nextDouble() * 2.0 - 1.0) * 0.08 * grainLen;
    g.readPos = anchor - (double) grainLen + jitter + startPhase * grainLen;
    g.phase = startPhase;
    g.gain = cycleGain;
}

void Freeze::process (juce::AudioBuffer<float>& buffer, const Values& v, const Clock&) noexcept
{
    const int n = buffer.getNumSamples();
    if (n <= 0 || n > dry.getNumSamples())
        return;
    copyDry (dry, buffer);

    const bool want = v[5] > 0.5f;
    const float hold = want ? pct (v[0]) : 0.f;
    const int grainLen = juce::jlimit (64, ringLen / 4, (int) ((30.f + pct (v[1]) * 370.f) * 0.001f * (float) sampleRate));
    const float decay = pct (v[2]);
    const double rate = std::pow (2.0, v[3] / 12.0);
    const double widthOffset = pct (v[4]) * 0.012 * sampleRate;

    // A freeze needs material: right after a reset keep capturing for half a
    // second before locking the buffer.
    const bool engaged = want && warmSamples >= warmTarget;
    warmSamples = juce::jmin (warmTarget, warmSamples + n);
    if (engaged && ! frozen)
    {
        frozen = true;
        freezePoint = (double) writePos;
        cycleGain = 1.f;
    }
    else if (! engaged && frozen)
    {
        frozen = false;
    }

    float* L = buffer.getWritePointer (0);
    float* R = buffer.getWritePointer (1);
    float* rL = ring.getWritePointer (0);
    float* rR = ring.getWritePointer (1);

    for (int i = 0; i < n; ++i)
    {
        if (! frozen)
        {
            rL[writePos] = dry.getSample (0, i);
            rR[writePos] = dry.getSample (1, i);
            writePos = (writePos + 1) % ringLen;
        }
        const double anchor = frozen ? freezePoint : (double) writePos;

        float wetL = 0.f, wetR = 0.f;
        for (int gi = 0; gi < 2; ++gi)
        {
            auto& g = grains[gi];
            if (g.phase >= 1.f)
            {
                if (frozen && gi == 0)
                    cycleGain *= 0.55f + 0.45f * decay;
                spawn (g, grainLen, anchor, gi == 1 && g.len <= 1 ? 0.5f : 0.f);
            }
            const float win = 0.5f - 0.5f * std::cos (g.phase * kTwoPi);
            wetL += readRing (0, g.readPos) * win * g.gain;
            wetR += readRing (1, g.readPos + widthOffset) * win * g.gain;
            g.readPos += rate;
            g.phase += 1.f / (float) g.len;
        }

        L[i] = dry.getSample (0, i) * (1.f - hold) + wetL * hold;
        R[i] = dry.getSample (1, i) * (1.f - hold) + wetR * hold;
    }
}

// =============================================================================
//  Bitcrusher
// =============================================================================
void Bitcrusher::prepare (const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;
    dry.setSize (2, (int) spec.maximumBlockSize, false, true, true);
    rng.setSeed (0x4243);
    reset();
}

void Bitcrusher::reset() { phase = 0.f; holdL = holdR = lpL = lpR = 0.f; jitterScale = 1.f; }

void Bitcrusher::process (juce::AudioBuffer<float>& buffer, const Values& v, const Clock&) noexcept
{
    const int n = buffer.getNumSamples();
    if (n <= 0 || n > dry.getNumSamples())
        return;
    copyDry (dry, buffer);

    const float bits = juce::jlimit (1.f, 16.f, v[0]);
    const float levels = std::pow (2.f, bits - 1.f);
    const float rate = juce::jlimit (500.f, (float) sampleRate, v[1]);
    const float jitter = pct (v[2]);
    const float lpCoef = onePoleCoef (v[3], sampleRate);
    const float mix = pct (v[4]);

    float* L = buffer.getWritePointer (0);
    float* R = buffer.getWritePointer (1);
    for (int i = 0; i < n; ++i)
    {
        phase += rate * jitterScale / (float) sampleRate;
        if (phase >= 1.f)
        {
            phase -= 1.f;
            holdL = std::round (L[i] * levels) / levels;
            holdR = std::round (R[i] * levels) / levels;
            jitterScale = 1.f + (rng.nextFloat() * 2.f - 1.f) * jitter * 0.8f;
        }
        lpL += lpCoef * (holdL - lpL);
        lpR += lpCoef * (holdR - lpR);
        L[i] = lpL;
        R[i] = lpR;
    }
    blendDry (buffer, dry, mix);
}

// =============================================================================
//  Compressor
// =============================================================================
void Compressor::prepare (const juce::dsp::ProcessSpec& spec)
{
    comp.prepare (spec);
    dry.setSize (2, (int) spec.maximumBlockSize, false, true, true);
}

void Compressor::reset() { comp.reset(); }

void Compressor::process (juce::AudioBuffer<float>& buffer, const Values& v, const Clock&) noexcept
{
    const int n = buffer.getNumSamples();
    if (n <= 0 || n > dry.getNumSamples())
        return;
    copyDry (dry, buffer);
    comp.setThreshold (v[0]);
    comp.setRatio (juce::jmax (1.f, v[1]));
    comp.setAttack (v[2]);
    comp.setRelease (v[3]);
    juce::dsp::AudioBlock<float> block (buffer);
    juce::dsp::ProcessContextReplacing<float> ctx (block);
    comp.process (ctx);
    buffer.applyGain (juce::Decibels::decibelsToGain (v[4]));
    blendDry (buffer, dry, pct (v[5]));
}

// =============================================================================
//  Chorus
// =============================================================================
void Chorus::prepare (const juce::dsp::ProcessSpec& spec) { chorus.prepare (spec); }
void Chorus::reset() { chorus.reset(); }

void Chorus::process (juce::AudioBuffer<float>& buffer, const Values& v, const Clock&) noexcept
{
    chorus.setRate (v[0]);
    chorus.setDepth (pct (v[1]));
    chorus.setCentreDelay (juce::jlimit (1.f, 30.f, v[2]));
    chorus.setFeedback (juce::jlimit (-0.95f, 0.95f, v[3] * 0.01f));
    chorus.setMix (pct (v[4]));
    juce::dsp::AudioBlock<float> block (buffer);
    juce::dsp::ProcessContextReplacing<float> ctx (block);
    chorus.process (ctx);
}

// =============================================================================
//  Space
// =============================================================================
void Space::prepare (const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;
    reverb.setSampleRate (spec.sampleRate);
    preLen = juce::jmax (16, (int) (0.25 * sampleRate));
    pre.setSize (2, preLen, false, true, true);
    dry.setSize (2, (int) spec.maximumBlockSize, false, true, true);
    reset();
}

void Space::reset()
{
    reverb.reset();
    pre.clear();
    preWrite = 0;
    hpL = hpR = hpInL = hpInR = 0.f;
}

void Space::process (juce::AudioBuffer<float>& buffer, const Values& v, const Clock&) noexcept
{
    const int n = buffer.getNumSamples();
    if (n <= 0 || n > dry.getNumSamples())
        return;
    copyDry (dry, buffer);

    juce::Reverb::Parameters p;
    p.roomSize = pct (v[0]);
    p.damping = pct (v[1]);
    p.width = pct (v[2]);
    p.wetLevel = 1.f;
    p.dryLevel = 0.f;
    p.freezeMode = 0.f;
    reverb.setParameters (p);

    const int preSamples = juce::jlimit (1, preLen - 2, (int) (v[3] * 0.001 * sampleRate));
    const float hpCoef = onePoleCoef (v[4], sampleRate);

    float* L = buffer.getWritePointer (0);
    float* R = buffer.getWritePointer (1);
    float* pL = pre.getWritePointer (0);
    float* pR = pre.getWritePointer (1);
    for (int i = 0; i < n; ++i)
    {
        pL[preWrite] = L[i];
        pR[preWrite] = R[i];
        const int rp = (preWrite - preSamples + preLen) % preLen;
        float inL = pL[rp], inR = pR[rp];
        // lo cut into the reverb
        const float oL = inL - hpInL + (1.f - hpCoef) * hpL; hpInL = inL; hpL = oL;
        const float oR = inR - hpInR + (1.f - hpCoef) * hpR; hpInR = inR; hpR = oR;
        L[i] = oL;
        R[i] = oR;
        preWrite = (preWrite + 1) % preLen;
    }
    reverb.processStereo (L, R, n);
    blendDry (buffer, dry, pct (v[5]));
}
} // namespace Mfx
