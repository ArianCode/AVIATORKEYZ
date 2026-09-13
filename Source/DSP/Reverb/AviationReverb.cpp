#include "AviationReverb.h"
#include "../FastMath.h"
#include <juce_audio_basics/juce_audio_basics.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace AviationReverb
{
namespace
{
constexpr float kTwoPi = juce::MathConstants<float>::twoPi;
constexpr float kInvSqrt8 = 0.35355339f;
constexpr int kLines = 8;

// -----------------------------------------------------------------------------
//  Per-algorithm tuning shared by the engine front end
// -----------------------------------------------------------------------------
struct Tuning
{
    float rtMin, rtMax;          // RT60 seconds at size 0 / 1 (geometric sweep)
    float scaleMin, scaleMax;    // delay-length scale at size 0 / 1
    float fcMax, fcMin;          // in-loop damping cutoff at damping 0 / 1
    float preDelayMs;            // default pre-delay
    float outputGain;            // wet calibration vs. juce::Reverb
    float rtCorrection;          // measured-vs-target RT60 compensation
};

constexpr std::array<Tuning, (size_t) Algorithm::count> kTuning { {
    { 0.30f,  8.0f, 0.60f, 1.15f, 14000.f, 1400.f,  0.f, 2.525f, 1.04f },   // plate
    { 0.40f, 12.0f, 0.65f, 1.30f, 12000.f, 1100.f, 12.f, 2.247f, 1.22f },   // hall
    { 0.20f,  3.0f, 0.50f, 1.20f, 11000.f, 1200.f,  2.f, 2.018f, 1.22f },   // room
    { 1.00f, 30.0f, 0.70f, 1.40f, 10000.f,  900.f, 20.f, 2.393f, 1.22f },   // cloud
    { 0.40f, 10.0f, 0.60f, 1.25f,  9000.f, 1000.f,  6.f, 1.892f, 1.08f },   // hardware
} };

constexpr float kMaxPreDelayMs = 250.f;
constexpr float kVintageSteps = 2048.f;      // 12-bit converter
constexpr float kFadeSeconds = 0.06f;

inline int nextPow2 (int v) noexcept
{
    int p = 1;
    while (p < v)
        p <<= 1;
    return p;
}

inline float onePoleAlpha (float hz, double sampleRate) noexcept
{
    hz = juce::jlimit (5.f, (float) sampleRate * 0.45f, hz);
    return 1.f - std::exp (-kTwoPi * hz / (float) sampleRate);
}

inline float msToSamples (float ms, double sampleRate) noexcept
{
    return ms * 0.001f * (float) sampleRate;
}

/** Magnitude truncation — cannot sustain a zero-input limit cycle. */
inline float truncateTo (float x, float steps) noexcept
{
    return std::trunc (x * steps) / steps;
}

/** Per-line decay gain for a loop of `seconds` reaching -60 dB after rt60. */
inline float decayGain (float seconds, float rt60) noexcept
{
    return juce::jlimit (0.f, 0.9995f, std::pow (10.f, -3.f * seconds / juce::jmax (0.05f, rt60)));
}

// -----------------------------------------------------------------------------
//  Primitives
// -----------------------------------------------------------------------------
class DelayLine
{
public:
    void prepare (int maxDelaySamples)
    {
        const int size = nextPow2 (juce::jmax (16, maxDelaySamples + 8));
        buffer.assign ((size_t) size, 0.f);
        mask = size - 1;
        writePos = 0;
    }

    void reset() noexcept
    {
        std::fill (buffer.begin(), buffer.end(), 0.f);
        writePos = 0;
    }

    float maxDelay() const noexcept { return (float) (mask - 4); }

    // Read before write: read (d) is the sample written d writes ago.
    float read (int d) const noexcept
    {
        return buffer[(size_t) ((writePos - d) & mask)];
    }

    float readLinear (float d) const noexcept
    {
        d = juce::jlimit (1.f, maxDelay(), d);
        const int i = (int) d;
        const float f = d - (float) i;
        const float a = read (i);
        return a + f * (read (i + 1) - a);
    }

    float readCubic (float d) const noexcept
    {
        d = juce::jlimit (2.f, maxDelay(), d);
        const int i = (int) d;
        return AviatorFastMath::hermite4 (read (i - 1), read (i), read (i + 1), read (i + 2), d - (float) i);
    }

    void write (float x) noexcept
    {
        buffer[(size_t) writePos] = x;
        writePos = (writePos + 1) & mask;
    }

private:
    std::vector<float> buffer;
    int mask { 0 };
    int writePos { 0 };
};

class Allpass
{
public:
    void prepare (int maxDelaySamples) { line.prepare (maxDelaySamples); }
    void reset() noexcept { line.reset(); }

    float process (float x, float delay, float g) noexcept
    {
        const float z = line.readCubic (delay);
        const float w = x + g * z;
        line.write (w);
        return z - g * w;
    }

    float processInt (float x, int delay, float g) noexcept
    {
        const float z = line.read (delay);
        const float w = x + g * z;
        line.write (w);
        return z - g * w;
    }

    const DelayLine& internal() const noexcept { return line; }

private:
    DelayLine line;
};

struct OnePole
{
    float y { 0.f };
    float lowpass (float x, float a) noexcept { y += a * (x - y); return y; }
    float highpass (float x, float a) noexcept { y += a * (x - y); return x - y; }
    void reset() noexcept { y = 0.f; }
};

/** C1-smooth random modulator: Catmull-Rom through random points. */
class RandomLfo
{
public:
    void seed (uint32_t s) noexcept
    {
        state = s != 0 ? s : 0x9e3779b9u;
        for (auto& p : points)
            p = nextRandom();
        phase = 0.f;
    }

    float next (float increment) noexcept
    {
        phase += increment;
        if (phase >= 1.f)
        {
            phase -= 1.f;
            points[0] = points[1];
            points[1] = points[2];
            points[2] = points[3];
            points[3] = nextRandom();
        }
        return AviatorFastMath::hermite4 (points[0], points[1], points[2], points[3], phase);
    }

private:
    float nextRandom() noexcept
    {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        return (float) (state & 0xffffffu) / 8388607.5f - 1.f;
    }

    uint32_t state { 1 };
    std::array<float, 4> points {};
    float phase { 0.f };
};

inline void hadamard8 (float* x) noexcept
{
    for (int h = 1; h < kLines; h <<= 1)
        for (int i = 0; i < kLines; i += h * 2)
            for (int j = i; j < i + h; ++j)
            {
                const float a = x[j];
                const float b = x[j + h];
                x[j] = a + b;
                x[j + h] = a - b;
            }
    for (int i = 0; i < kLines; ++i)
        x[i] *= kInvSqrt8;
}

// -----------------------------------------------------------------------------
//  Cores
// -----------------------------------------------------------------------------
struct CoreParams
{
    float rt60 { 1.5f };
    float dampAlpha { 0.5f };
    float scale { 1.f };        // target delay scale (cores glide toward it)
    float modDepth { 1.f };     // 1 = modern, lower for vintage
};

class Core
{
public:
    virtual ~Core() = default;
    virtual void prepare (double sampleRate) = 0;
    virtual void reset() noexcept = 0;
    virtual void process (const float* inL, const float* inR, float* outL, float* outR,
                          int n, const CoreParams& p) noexcept = 0;

protected:
    void prepareScale (double sampleRate) noexcept
    {
        scaleCoef = 1.f - std::exp (-1.f / (0.25f * (float) sampleRate));
        scaleInitialised = false;
    }

    float nextScale (float target) noexcept
    {
        if (! scaleInitialised)
        {
            scale = target;
            scaleInitialised = true;
        }
        scale += scaleCoef * (target - scale);
        return scale;
    }

    float scale { 1.f };
    float scaleCoef { 0.001f };
    bool scaleInitialised { false };
};

// ---- Plate: Dattorro (1997) figure-eight tank -------------------------------
class PlateCore final : public Core
{
public:
    void prepare (double sampleRate) override
    {
        sr = sampleRate;
        ratio = (float) (sampleRate / kRefRate);
        const float maxScale = kTuning[(size_t) Algorithm::plate].scaleMax * 1.02f;
        const int excursion = (int) std::ceil (msToSamples (kExcursionMs, sampleRate)) + 4;
        auto len = [&] (float refSamples) { return (int) std::ceil (refSamples * ratio * maxScale) + excursion; };

        for (size_t i = 0; i < inputDiffusers.size(); ++i)
            inputDiffusers[i].prepare ((int) std::ceil (kInputDiffLengths[i] * ratio) + 4);
        apL1.prepare (len (672.f));
        apL2.prepare (len (1800.f));
        apR1.prepare (len (908.f));
        apR2.prepare (len (2656.f));
        dL1.prepare (len (4453.f));
        dL2.prepare (len (3720.f));
        dR1.prepare (len (4217.f));
        dR2.prepare (len (3163.f));
        bandwidthAlpha = onePoleAlpha (13500.f, sampleRate);
        prepareScale (sampleRate);
        reset();
    }

    void reset() noexcept override
    {
        for (auto& d : inputDiffusers) d.reset();
        apL1.reset(); apL2.reset(); apR1.reset(); apR2.reset();
        dL1.reset(); dL2.reset(); dR1.reset(); dR2.reset();
        bandwidth.reset(); dampL.reset(); dampR.reset();
        phaseL = 0.f;
        phaseR = 0.25f;
        scaleInitialised = false;
    }

    void process (const float* inL, const float* inR, float* outL, float* outR,
                  int n, const CoreParams& p) noexcept override
    {
        // Four decay multipliers per full circuit of the tank.
        const float loopSeconds = kTankRefSamples / (float) kRefRate * scale;
        const float decay = juce::jmin (0.97f, decayGain (loopSeconds * 0.25f, p.rt60));
        const float decayDiffusion2 = juce::jlimit (0.25f, 0.5f, decay + 0.15f);
        const float excursion = msToSamples (kExcursionMs, sr) * p.modDepth;
        const float incL = 0.50f / (float) sr;
        const float incR = 0.63f / (float) sr;

        const int d0 = (int) (kInputDiffLengths[0] * ratio);
        const int d1 = (int) (kInputDiffLengths[1] * ratio);
        const int d2 = (int) (kInputDiffLengths[2] * ratio);
        const int d3 = (int) (kInputDiffLengths[3] * ratio);

        for (int i = 0; i < n; ++i)
        {
            const float k = ratio * nextScale (p.scale);

            float x = bandwidth.lowpass (0.5f * (inL[i] + inR[i]), bandwidthAlpha);
            x = inputDiffusers[0].processInt (x, d0, 0.75f);
            x = inputDiffusers[1].processInt (x, d1, 0.75f);
            x = inputDiffusers[2].processInt (x, d2, 0.625f);
            x = inputDiffusers[3].processInt (x, d3, 0.625f);

            phaseL += incL; if (phaseL >= 1.f) phaseL -= 1.f;
            phaseR += incR; if (phaseR >= 1.f) phaseR -= 1.f;
            const float modL = std::sin (kTwoPi * phaseL) * excursion;
            const float modR = std::sin (kTwoPi * phaseR) * excursion;

            const float intoLeft  = dR2.readLinear (3163.f * k) * decay;
            const float intoRight = dL2.readLinear (3720.f * k) * decay;

            // left half
            const float a = apL1.process (x + intoLeft, 672.f * k + modL, -0.70f);
            const float b = dampL.lowpass (dL1.readLinear (4453.f * k), p.dampAlpha) * decay;
            dL1.write (a);
            dL2.write (apL2.process (b, 1800.f * k, decayDiffusion2));

            // right half
            const float c = apR1.process (x + intoRight, 908.f * k + modR, -0.70f);
            const float d = dampR.lowpass (dR1.readLinear (4217.f * k), p.dampAlpha) * decay;
            dR1.write (c);
            dR2.write (apR2.process (d, 2656.f * k, decayDiffusion2));

            const float yL = dR1.readLinear (266.f * k) + dR1.readLinear (2974.f * k)
                           - apR2.internal().readLinear (1913.f * k) + dR2.readLinear (1996.f * k)
                           - dL1.readLinear (1990.f * k) - apL2.internal().readLinear (187.f * k)
                           - dL2.readLinear (1066.f * k);
            const float yR = dL1.readLinear (353.f * k) + dL1.readLinear (3627.f * k)
                           - apL2.internal().readLinear (1228.f * k) + dL2.readLinear (2673.f * k)
                           - dR1.readLinear (2111.f * k) - apR2.internal().readLinear (335.f * k)
                           - dR2.readLinear (121.f * k);
            outL[i] = yL * 0.6f;
            outR[i] = yR * 0.6f;
        }
    }

private:
    static constexpr double kRefRate = 29761.0;
    static constexpr float kExcursionMs = 0.45f;
    static constexpr float kTankRefSamples = 672.f + 4453.f + 1800.f + 3720.f + 908.f + 4217.f + 2656.f + 3163.f;
    static constexpr std::array<float, 4> kInputDiffLengths { 142.f, 107.f, 379.f, 277.f };

    double sr { 44100.0 };
    float ratio { 1.f };
    std::array<Allpass, 4> inputDiffusers;
    Allpass apL1, apL2, apR1, apR2;
    DelayLine dL1, dL2, dR1, dR2;
    OnePole bandwidth, dampL, dampR;
    float bandwidthAlpha { 0.8f };
    float phaseL { 0.f }, phaseR { 0.25f };
};

// ---- FDN: Hall / Room / Cloud ----------------------------------------------
struct FdnConfig
{
    Algorithm algorithm;
    std::array<float, kLines> lengthsMs;
    int diffusers;                          // input all-passes per channel (0–4)
    std::array<float, 4> diffMsL, diffMsR;
    float diffG;
    bool loopAllpass;                       // Cloud: diffusion inside the loop
    std::array<float, kLines> loopApMs;
    float loopApG;
    float modDepthMs, modRateHz;
    float erLevel;                          // 0 = no early reflections
    std::array<float, kLines> erMsL, erMsR;
    float outputGain;
};

constexpr std::array<float, kLines> kErGains { 0.84f, -0.72f, 0.63f, -0.55f, 0.47f, -0.41f, 0.34f, -0.28f };

constexpr FdnConfig kHall {
    Algorithm::hall,
    { 29.7f, 37.1f, 41.1f, 43.7f, 53.9f, 59.3f, 67.1f, 79.3f },
    4, { 4.77f, 3.59f, 12.73f, 9.31f }, { 5.21f, 3.97f, 11.87f, 8.83f }, 0.70f,
    false, {}, 0.f,
    0.55f, 0.35f,
    0.25f,
    { 9.8f, 15.1f, 22.6f, 31.2f, 39.4f, 50.3f, 61.7f, 74.9f },
    { 11.2f, 17.4f, 20.5f, 34.7f, 43.1f, 47.9f, 66.2f, 79.8f },
    1.0f
};

constexpr FdnConfig kRoom {
    Algorithm::room,
    { 11.3f, 13.7f, 17.9f, 19.1f, 23.3f, 27.1f, 29.9f, 33.7f },
    2, { 3.13f, 2.27f, 0.f, 0.f }, { 3.41f, 2.53f, 0.f, 0.f }, 0.62f,
    false, {}, 0.f,
    0.15f, 0.6f,
    0.55f,
    { 4.3f, 7.1f, 11.9f, 15.2f, 21.7f, 26.3f, 33.1f, 41.6f },
    { 5.1f, 8.3f, 10.7f, 17.3f, 19.9f, 28.9f, 31.4f, 44.2f },
    1.0f
};

constexpr FdnConfig kCloud {
    Algorithm::cloud,
    { 43.1f, 53.7f, 61.9f, 71.3f, 83.9f, 97.1f, 109.3f, 127.7f },
    4, { 6.13f, 4.61f, 15.37f, 11.29f }, { 6.67f, 4.99f, 14.53f, 10.73f }, 0.75f,
    true, { 3.1f, 4.3f, 5.3f, 6.1f, 7.3f, 8.9f, 10.1f, 12.7f }, 0.55f,
    1.6f, 0.22f,
    0.f, {}, {},
    1.0f
};

class FdnCore final : public Core
{
public:
    explicit FdnCore (const FdnConfig& c) : cfg (c) {}

    void prepare (double sampleRate) override
    {
        sr = sampleRate;
        const auto& t = kTuning[(size_t) cfg.algorithm];
        const float maxScale = t.scaleMax * 1.02f;
        const int modMargin = (int) std::ceil (msToSamples (cfg.modDepthMs * 1.3f, sampleRate)) + 4;

        for (int j = 0; j < kLines; ++j)
        {
            lines[(size_t) j].prepare ((int) std::ceil (msToSamples (cfg.lengthsMs[(size_t) j] * maxScale, sampleRate)) + modMargin);
            if (cfg.loopAllpass)
                loopAps[(size_t) j].prepare ((int) std::ceil (msToSamples (cfg.loopApMs[(size_t) j] * maxScale, sampleRate)) + 4);
            lfos[(size_t) j].seed (0x51ed27u * (uint32_t) (j + 1) + (uint32_t) cfg.algorithm * 7919u);
        }
        for (int d = 0; d < cfg.diffusers; ++d)
        {
            diffL[(size_t) d].prepare ((int) std::ceil (msToSamples (cfg.diffMsL[(size_t) d], sampleRate)) + 4);
            diffR[(size_t) d].prepare ((int) std::ceil (msToSamples (cfg.diffMsR[(size_t) d], sampleRate)) + 4);
        }
        if (cfg.erLevel > 0.f)
        {
            const float longest = juce::jmax (cfg.erMsL.back(), cfg.erMsR.back());
            erL.prepare ((int) std::ceil (msToSamples (longest * maxScale, sampleRate)) + 4);
            erR.prepare ((int) std::ceil (msToSamples (longest * maxScale, sampleRate)) + 4);
        }
        prepareScale (sampleRate);
        reset();
    }

    void reset() noexcept override
    {
        for (auto& l : lines) l.reset();
        for (auto& a : loopAps) a.reset();
        for (auto& d : diffL) d.reset();
        for (auto& d : diffR) d.reset();
        for (auto& f : damp) f.reset();
        erL.reset();
        erR.reset();
        scaleInitialised = false;
    }

    void process (const float* inL, const float* inR, float* outL, float* outR,
                  int n, const CoreParams& p) noexcept override
    {
        std::array<float, kLines> gains {};
        for (int j = 0; j < kLines; ++j)
        {
            float ms = cfg.lengthsMs[(size_t) j];
            if (cfg.loopAllpass)
                ms += cfg.loopApMs[(size_t) j];
            gains[(size_t) j] = decayGain (ms * 0.001f * scale, p.rt60);
        }

        std::array<int, 4> dl {}, dr {};
        for (int d = 0; d < cfg.diffusers; ++d)
        {
            dl[(size_t) d] = juce::jmax (1, (int) msToSamples (cfg.diffMsL[(size_t) d], sr));
            dr[(size_t) d] = juce::jmax (1, (int) msToSamples (cfg.diffMsR[(size_t) d], sr));
        }

        const float msToS = 0.001f * (float) sr;
        const float modDepth = cfg.modDepthMs * msToS * p.modDepth;
        const float modInc = cfg.modRateHz / (float) sr;
        const float erGain = cfg.erLevel * 0.45f;

        std::array<float, kLines> o {}, f {};
        for (int i = 0; i < n; ++i)
        {
            const float s = nextScale (p.scale);
            float l = inL[i];
            float r = inR[i];

            float earlyL = 0.f, earlyR = 0.f;
            if (erGain > 0.f)
            {
                erL.write (l);
                erR.write (r);
                for (int t = 0; t < kLines; ++t)
                {
                    earlyL += kErGains[(size_t) t] * erL.readLinear (cfg.erMsL[(size_t) t] * msToS * s);
                    earlyR += kErGains[(size_t) t] * erR.readLinear (cfg.erMsR[(size_t) t] * msToS * s);
                }
            }

            for (int d = 0; d < cfg.diffusers; ++d)
            {
                l = diffL[(size_t) d].processInt (l, dl[(size_t) d], cfg.diffG);
                r = diffR[(size_t) d].processInt (r, dr[(size_t) d], cfg.diffG);
            }

            for (int j = 0; j < kLines; ++j)
            {
                const float rate = modInc * (0.8f + 0.07f * (float) j);
                const float len = cfg.lengthsMs[(size_t) j] * msToS * s + lfos[(size_t) j].next (rate) * modDepth;
                o[(size_t) j] = lines[(size_t) j].readCubic (len);
                if (cfg.loopAllpass)
                    o[(size_t) j] = loopAps[(size_t) j].process (o[(size_t) j], cfg.loopApMs[(size_t) j] * msToS * s, cfg.loopApG);
                f[(size_t) j] = damp[(size_t) j].lowpass (o[(size_t) j], p.dampAlpha) * gains[(size_t) j];
            }

            hadamard8 (f.data());

            const float injL = l * 0.5f;
            const float injR = r * 0.5f;
            for (int j = 0; j < kLines; ++j)
                lines[(size_t) j].write (f[(size_t) j] + ((j & 1) == 0 ? injL : injR));

            const float lateL = o[0] - o[1] + o[2] - o[3] + o[4] - o[5] + o[6] - o[7];
            const float lateR = o[0] + o[1] - o[2] - o[3] + o[4] + o[5] - o[6] - o[7];
            outL[i] = (lateL * kInvSqrt8) * cfg.outputGain + earlyL * erGain;
            outR[i] = (lateR * kInvSqrt8) * cfg.outputGain + earlyR * erGain;
        }
    }

private:
    const FdnConfig& cfg;
    double sr { 44100.0 };
    std::array<DelayLine, kLines> lines;
    std::array<Allpass, kLines> loopAps;
    std::array<OnePole, kLines> damp;
    std::array<RandomLfo, kLines> lfos;
    std::array<Allpass, 4> diffL, diffR;
    DelayLine erL, erR;
};

// ---- Hardware: ring of nested all-pass sections -----------------------------
class HardwareCore final : public Core
{
public:
    void prepare (double sampleRate) override
    {
        sr = sampleRate;
        const float maxScale = kTuning[(size_t) Algorithm::hardware].scaleMax * 1.02f;
        const int modMargin = (int) std::ceil (msToSamples (kModDepthMs * 1.3f, sampleRate)) + 4;
        for (size_t s = 0; s < kSections; ++s)
        {
            outer[s].prepare ((int) std::ceil (msToSamples (kOuterMs[s] * maxScale, sampleRate)) + modMargin);
            inner[s].prepare ((int) std::ceil (msToSamples (kInnerMs[s] * maxScale, sampleRate)) + 4);
            delays[s].prepare ((int) std::ceil (msToSamples (kDelayMs[s] * maxScale, sampleRate)) + 4);
            lfos[s].seed (0xa24baed5u + (uint32_t) s * 104729u);
        }
        for (size_t d = 0; d < 2; ++d)
        {
            diffL[d].prepare ((int) std::ceil (msToSamples (kDiffMsL[d], sampleRate)) + 4);
            diffR[d].prepare ((int) std::ceil (msToSamples (kDiffMsR[d], sampleRate)) + 4);
        }
        bandLimitAlpha = onePoleAlpha (7800.f, sampleRate);
        prepareScale (sampleRate);
        reset();
    }

    void reset() noexcept override
    {
        for (auto& a : outer) a.reset();
        for (auto& d : delays) d.reset();
        for (auto& a : inner) a.reset();
        for (auto& f : damp) f.reset();
        for (auto& f : bandLimit) f.reset();
        for (auto& d : diffL) d.reset();
        for (auto& d : diffR) d.reset();
        scaleInitialised = false;
    }

    void process (const float* inL, const float* inR, float* outL, float* outR,
                  int n, const CoreParams& p) noexcept override
    {
        std::array<float, kSections> gains {};
        for (size_t s = 0; s < kSections; ++s)
            gains[s] = decayGain ((kOuterMs[s] + kDelayMs[s]) * 0.001f * scale, p.rt60);

        const float msToS = 0.001f * (float) sr;
        const float modDepth = kModDepthMs * msToS * p.modDepth;
        const float modInc = 1.7f / (float) sr;
        const int dl0 = (int) msToSamples (kDiffMsL[0], sr), dl1 = (int) msToSamples (kDiffMsL[1], sr);
        const int dr0 = (int) msToSamples (kDiffMsR[0], sr), dr1 = (int) msToSamples (kDiffMsR[1], sr);

        for (int i = 0; i < n; ++i)
        {
            const float s = nextScale (p.scale);
            float l = diffL[1].processInt (diffL[0].processInt (inL[i], dl0, 0.7f), dl1, 0.7f);
            float r = diffR[1].processInt (diffR[0].processInt (inR[i], dr0, 0.7f), dr1, 0.7f);
            const std::array<float, kSections> inject { l * 0.7f, -(l + r) * 0.35f, r * 0.7f };

            std::array<float, kSections> len {};
            for (size_t k = 0; k < kSections; ++k)
                len[k] = kDelayMs[k] * msToS * s;

            // ring: section k is fed by the delayed output of section k-1
            std::array<float, kSections> ringIn {};
            for (size_t k = 0; k < kSections; ++k)
            {
                const size_t prev = (k + kSections - 1) % kSections;
                float y = delays[prev].readLinear (len[prev]);
                y = damp[prev].lowpass (y, p.dampAlpha);
                y = bandLimit[prev].lowpass (y, bandLimitAlpha);
                ringIn[k] = y * gains[prev] + inject[k];
            }

            for (size_t k = 0; k < kSections; ++k)
            {
                // nested all-pass: an inner all-pass sits inside the outer delay path
                const float outerLen = kOuterMs[k] * msToS * s + lfos[k].next (modInc * (1.f + 0.13f * (float) k)) * modDepth;
                const float z = inner[k].process (outerDelayRead (k, outerLen), kInnerMs[k] * msToS * s, 0.45f);
                const float w = ringIn[k] + 0.6f * z;
                outer[k].write (w);
                delays[k].write (z - 0.6f * w);
            }

            outL[i] = 0.7f * (delays[0].readLinear (0.31f * len[0]) - delays[1].readLinear (0.57f * len[1])
                            + delays[2].readLinear (0.13f * len[2]) - delays[2].readLinear (0.79f * len[2]));
            outR[i] = 0.7f * (delays[1].readLinear (0.23f * len[1]) - delays[2].readLinear (0.47f * len[2])
                            + delays[0].readLinear (0.67f * len[0]) - delays[1].readLinear (0.89f * len[1]));
        }
    }

private:
    static constexpr size_t kSections = 3;
    static constexpr float kModDepthMs = 0.3f;
    static constexpr std::array<float, kSections> kOuterMs { 23.1f, 31.7f, 27.3f };
    static constexpr std::array<float, kSections> kInnerMs { 7.3f, 9.1f, 5.9f };
    static constexpr std::array<float, kSections> kDelayMs { 97.3f, 83.9f, 113.1f };
    static constexpr std::array<float, 2> kDiffMsL { 4.7f, 3.6f };
    static constexpr std::array<float, 2> kDiffMsR { 5.3f, 3.1f };

    float outerDelayRead (size_t k, float delay) const noexcept { return outer[k].readCubic (delay); }

    double sr { 44100.0 };
    std::array<DelayLine, kSections> outer, delays;
    std::array<Allpass, kSections> inner;
    std::array<OnePole, kSections> damp, bandLimit;
    std::array<RandomLfo, kSections> lfos;
    std::array<Allpass, 2> diffL, diffR;
    float bandLimitAlpha { 0.5f };
};
} // namespace

// =============================================================================
//  Public helpers
// =============================================================================
juce::StringArray algorithmNames()
{
    return { "Plate", "Hall", "Room", "Cloud", "Hardware" };
}

juce::StringArray colorNames()
{
    return { "Modern", "Vintage" };
}

float targetRt60Seconds (Algorithm algorithm, float size) noexcept
{
    const auto& t = kTuning[(size_t) algorithmFromIndex ((int) algorithm)];
    return t.rtMin * std::pow (t.rtMax / t.rtMin, juce::jlimit (0.f, 1.f, size));
}

// =============================================================================
//  Engine
// =============================================================================
struct Engine::Impl
{
    Impl() : hall (kHall), room (kRoom), cloud (kCloud)
    {
        cores = { &plate, &hall, &room, &cloud, &hardware };
    }

    PlateCore plate;
    FdnCore hall, room, cloud;
    HardwareCore hardware;
    std::array<Core*, (size_t) Algorithm::count> cores {};

    double sampleRate { 44100.0 };
    int blockSize { 512 };
    Settings settings;

    int active { 0 };
    int fading { -1 };
    int fadeLength { 1 };
    int fadeRemaining { 0 };
    float activeGainStart { 1.f };

    DelayLine preDelayL, preDelayR;
    float preDelaySamples { 0.f };
    float preDelayCoef { 0.001f };
    OnePole hpL, hpR;
    float hpAlpha { 0.004f };
    std::array<OnePole, 4> vinInL, vinInR, vinOutL, vinOutR;
    float vinInAlpha { 0.5f }, vinOutAlpha { 0.5f };
    float widthCurrent { 1.f };

    std::vector<float> preL, preR, fadeL, fadeR, silence;
};

Engine::Engine() : impl (std::make_unique<Impl>()) {}
Engine::~Engine() = default;

void Engine::prepare (double sampleRate, int maxBlockSize)
{
    auto& m = *impl;
    m.sampleRate = sampleRate > 0.0 ? sampleRate : 44100.0;
    m.blockSize = juce::jmax (1, maxBlockSize);

    for (auto* core : m.cores)
        core->prepare (m.sampleRate);

    const float maxPre = kMaxPreDelayMs + 30.f;
    m.preDelayL.prepare ((int) std::ceil (msToSamples (maxPre, m.sampleRate)) + 4);
    m.preDelayR.prepare ((int) std::ceil (msToSamples (maxPre, m.sampleRate)) + 4);
    m.preDelayCoef = 1.f - std::exp (-1.f / (0.05f * (float) m.sampleRate));
    m.hpAlpha = onePoleAlpha (25.f, m.sampleRate);
    m.vinInAlpha = onePoleAlpha (9000.f, m.sampleRate);
    m.vinOutAlpha = onePoleAlpha (10500.f, m.sampleRate);
    m.fadeLength = juce::jmax (1, (int) (kFadeSeconds * m.sampleRate));

    for (auto* v : { &m.preL, &m.preR, &m.fadeL, &m.fadeR, &m.silence })
        v->assign ((size_t) m.blockSize, 0.f);

    prepared = true;
    reset();
}

void Engine::reset()
{
    auto& m = *impl;
    for (auto* core : m.cores)
        core->reset();
    m.preDelayL.reset();
    m.preDelayR.reset();
    m.hpL.reset();
    m.hpR.reset();
    for (auto* bank : { &m.vinInL, &m.vinInR, &m.vinOutL, &m.vinOutR })
        for (auto& f : *bank)
            f.reset();
    m.active = (int) m.settings.algorithm;
    m.fading = -1;
    m.fadeRemaining = 0;
    m.activeGainStart = 1.f;
    m.preDelaySamples = -1.f;
    m.widthCurrent = m.settings.width;
}

void Engine::setSettings (const Settings& s) noexcept
{
    auto& m = *impl;
    m.settings = s;
    m.settings.algorithm = algorithmFromIndex ((int) s.algorithm);
    m.settings.color = colorFromIndex ((int) s.color);

    const int wanted = (int) m.settings.algorithm;
    if (! prepared || wanted == m.active)
        return;

    if (wanted == m.fading)
    {
        // switching back mid-fade: resume the ringing tank from its faded level
        const float fadedLevel = (float) m.fadeRemaining / (float) m.fadeLength;
        m.fading = m.active;
        m.active = wanted;
        m.activeGainStart = fadedLevel;
    }
    else
    {
        if (m.fading >= 0)
            m.cores[(size_t) m.fading]->reset();
        m.fading = m.active;
        m.active = wanted;
        m.activeGainStart = 0.f;
    }
    m.fadeRemaining = m.fadeLength;
}

void Engine::process (const float* inL, const float* inR,
                      float* outL, float* outR, int numSamples) noexcept
{
    if (numSamples <= 0)
        return;

    auto& m = *impl;
    if (! prepared)
    {
        std::fill (outL, outL + numSamples, 0.f);
        std::fill (outR, outR + numSamples, 0.f);
        return;
    }

    juce::ScopedNoDenormals noDenormals;

    const auto& s = m.settings;
    const auto& tuning = kTuning[(size_t) m.active];
    const bool vintage = s.color == Color::vintage;
    const float size = juce::jlimit (0.f, 1.f, s.size);

    CoreParams p;
    p.rt60 = targetRt60Seconds ((Algorithm) m.active, size) * tuning.rtCorrection;
    p.scale = tuning.scaleMin + (tuning.scaleMax - tuning.scaleMin) * size;
    p.dampAlpha = onePoleAlpha (tuning.fcMax * std::pow (tuning.fcMin / tuning.fcMax, juce::jlimit (0.f, 1.f, s.damping)),
                                m.sampleRate);
    p.modDepth = vintage ? 0.6f : 1.f;

    CoreParams fadeParams = p;
    if (m.fading >= 0)
    {
        const auto& ft = kTuning[(size_t) m.fading];
        fadeParams.rt60 = targetRt60Seconds ((Algorithm) m.fading, size) * ft.rtCorrection;
        fadeParams.scale = ft.scaleMin + (ft.scaleMax - ft.scaleMin) * size;
    }

    const float preMs = s.preDelayMs < 0.f ? tuning.preDelayMs : juce::jmin (s.preDelayMs, kMaxPreDelayMs);
    const float preTarget = juce::jmax (1.f, msToSamples (preMs, m.sampleRate));
    if (m.preDelaySamples < 0.f)
        m.preDelaySamples = preTarget;

    const float widthTarget = juce::jlimit (0.f, 1.f, s.width);

    for (int offset = 0; offset < numSamples;)
    {
        const int n = juce::jmin (m.blockSize, numSamples - offset);
        const float* srcL = inL + offset;
        const float* srcR = inR + offset;

        // ---- front end: sub cut, pre-delay, vintage converter ----------------
        for (int i = 0; i < n; ++i)
        {
            m.preDelayL.write (m.hpL.highpass (srcL[i], m.hpAlpha));
            m.preDelayR.write (m.hpR.highpass (srcR[i], m.hpAlpha));
            m.preDelaySamples += m.preDelayCoef * (preTarget - m.preDelaySamples);
            // written first, so a d-sample delay reads position d + 1
            float l = m.preDelayL.readCubic (m.preDelaySamples + 1.f);
            float r = m.preDelayR.readCubic (m.preDelaySamples + 1.f);

            if (vintage)
            {
                for (size_t k = 0; k < 2; ++k)
                {
                    l = m.vinInL[k].lowpass (l, m.vinInAlpha);
                    r = m.vinInR[k].lowpass (r, m.vinInAlpha);
                }
                l = truncateTo (l, kVintageSteps);
                r = truncateTo (r, kVintageSteps);
            }
            m.preL[(size_t) i] = l;
            m.preR[(size_t) i] = r;
        }

        float* dstL = outL + offset;
        float* dstR = outR + offset;

        // ---- tanks -------------------------------------------------------------
        m.cores[(size_t) m.active]->process (m.preL.data(), m.preR.data(), dstL, dstR, n, p);

        if (m.fading >= 0)
        {
            m.cores[(size_t) m.fading]->process (m.silence.data(), m.silence.data(),
                                                 m.fadeL.data(), m.fadeR.data(), n, fadeParams);
            const float fadeStep = 1.f / (float) m.fadeLength;
            for (int i = 0; i < n; ++i)
            {
                const float out = juce::jmax (0.f, (float) m.fadeRemaining * fadeStep);
                const float in = m.activeGainStart + (1.f - m.activeGainStart) * (1.f - out);
                dstL[i] = dstL[i] * in + m.fadeL[(size_t) i] * out;
                dstR[i] = dstR[i] * in + m.fadeR[(size_t) i] * out;
                if (m.fadeRemaining > 0)
                    --m.fadeRemaining;
            }
            if (m.fadeRemaining <= 0)
            {
                m.cores[(size_t) m.fading]->reset();
                m.fading = -1;
                m.activeGainStart = 1.f;
            }
        }

        // ---- output: calibration, vintage converter, width ------------------
        const float gain = tuning.outputGain;
        const float widthStep = (widthTarget - m.widthCurrent) / (float) n;
        for (int i = 0; i < n; ++i)
        {
            float l = dstL[i] * gain;
            float r = dstR[i] * gain;
            if (vintage)
            {
                for (size_t k = 0; k < 2; ++k)
                {
                    l = m.vinOutL[k].lowpass (l, m.vinOutAlpha);
                    r = m.vinOutR[k].lowpass (r, m.vinOutAlpha);
                }
                l = truncateTo (l, kVintageSteps);
                r = truncateTo (r, kVintageSteps);
            }
            m.widthCurrent += widthStep;
            const float mid = 0.5f * (l + r);
            const float side = 0.5f * (l - r) * m.widthCurrent;
            dstL[i] = mid + side;
            dstR[i] = mid - side;
        }
        m.widthCurrent = widthTarget;

        offset += n;
    }
}
} // namespace AviationReverb
