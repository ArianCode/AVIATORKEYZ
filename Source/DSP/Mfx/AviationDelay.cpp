#include "AviationDelay.h"
#include <algorithm>
#include <cmath>

namespace Mfx
{
namespace
{
using Mode  = AviationDelay::Mode;
using Style = AviationDelay::Style;

constexpr double kPiD    = juce::MathConstants<double>::pi;
constexpr double kTwoPiD = juce::MathConstants<double>::twoPi;

// Diffusion stage lengths (ms) — mutually prime-ish so the smear stays dense.
constexpr std::array<float, AviationDelay::kCloudStages> kCleanStageMs    { 2.3f, 3.7f, 5.3f, 7.9f, 11.3f, 15.7f, 0.f, 0.f };
constexpr std::array<float, AviationDelay::kCloudStages> kCloudStageMs    { 7.3f, 11.9f, 17.3f, 23.9f, 31.1f, 41.7f, 53.3f, 67.1f };
constexpr std::array<float, AviationDelay::kCloudStages> kRightStageScale { 1.09f, 1.13f, 1.07f, 1.11f, 1.05f, 1.12f, 1.08f, 1.10f };

// BBD compander reference level (compressed and expanded around this).
constexpr float kCompRef = 0.3f;

inline float pct01 (float v) noexcept { return juce::jlimit (0.f, 1.f, v * 0.01f); }

inline float coefForTau (double seconds, double sampleRate) noexcept
{
    return (float) (1.0 - std::exp (-1.0 / juce::jmax (1.0e-6, seconds * sampleRate)));
}

inline float onePoleCoef (float hz, double sampleRate) noexcept
{
    const double fc = juce::jlimit (5.0, sampleRate * 0.45, (double) hz);
    return (float) (1.0 - std::exp (-kTwoPiD * fc / sampleRate));
}

/** Transparent below 80 % of the ceiling, then bends smoothly into it. */
inline float softLimit (float x, float ceiling = 1.f) noexcept
{
    const float knee = 0.8f * ceiling;
    const float a = std::abs (x);
    if (a <= knee)
        return x;
    const float span = ceiling - knee;
    const float y = knee + span * std::tanh ((a - knee) / span);
    return x < 0.f ? -y : y;
}

inline float triangle (double phase) noexcept
{
    const double p = phase - std::floor (phase);
    return (float) (4.0 * std::abs (p - 0.5) - 1.0);
}

inline bool usesSecondLine (Style s) noexcept
{
    return s != Style::single && s != Style::quad;
}
} // namespace

// =============================================================================
//  Per-block settings
// =============================================================================
struct AviationDelay::BlockSettings
{
    Mode  mode { Mode::clean };
    Style style { Style::stereo };

    std::array<double, 2> timeTarget {};                     // samples, line A / line B
    std::array<double, kMaxTaps> quadScale { 1.0, 1.0, 1.0, 1.0 };
    float timeCoef { 0.001f };

    float fbTarget { 0.f }, mixTarget { 0.f }, widthTarget { 1.f };
    float duck { 0.f }, duckRelease { 0.f };
    float age { 0.f }, modDepth { 0.f }, modRate { 0.5f };

    // diffusion
    float diffusion { 0.f }, diffG { 0.f }, diffEngageTarget { 0.f };
    float diffModSamples { 0.f }, diffSizeFactor { 2.f };
    int   diffStages { kCleanStages };
    std::array<float, 2> diffLatency {};
    std::array<float, kCloudStages> stageMod {};

    // mode models
    float gapCoef { 1.f }, satDrive { 1.f }, noiseLevel { 0.f };
    float envAtt { 0.f }, envRel { 0.f };
    double clockRatio { 2.0 };                               // hold rate in cycles/sample; >= 1 = no hold
    float quantSteps { 0.f };
    std::array<double, 2> pitchRatio { 1.0, 1.0 };
    float pitchCompensation { 0.f };
    float spliceDepth { 0.f };
    double splicePeriod { 4.0 };
    int switchFadeSamples { 480 };
};

// =============================================================================
//  Building blocks
// =============================================================================
void AviationDelay::Line::allocate (int n)
{
    size = juce::jmax (8, n);
    data.assign ((size_t) size, 0.f);
    write = 0;
}

void AviationDelay::Line::clear() noexcept
{
    std::fill (data.begin(), data.end(), 0.f);
    write = 0;
}

float AviationDelay::Line::read (float delay) const noexcept
{
    delay = juce::jlimit (2.f, (float) (size - 4), delay);
    const int di = (int) delay;
    const float t = 1.f - (delay - (float) di);   // position measured from the older neighbour

    int i1 = write - di - 1;
    if (i1 < 0) i1 += size;
    int i0 = i1 - 1;
    if (i0 < 0) i0 += size;
    int i2 = i1 + 1;
    if (i2 >= size) i2 -= size;
    int i3 = i2 + 1;
    if (i3 >= size) i3 -= size;

    const float* d = data.data();
    const float y0 = d[i0], y1 = d[i1], y2 = d[i2], y3 = d[i3];
    const float c1 = 0.5f * (y2 - y0);
    const float c2 = y0 - 2.5f * y1 + 2.f * y2 - 0.5f * y3;
    const float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);
    return ((c3 * t + c2) * t + c1) * t + y1;
}

float AviationDelay::Line::readLinear (float delay) const noexcept
{
    delay = juce::jlimit (1.f, (float) (size - 2), delay);
    const int di = (int) delay;
    const float frac = delay - (float) di;
    int ia = write - di;
    if (ia < 0) ia += size;
    int ib = ia - 1;
    if (ib < 0) ib += size;
    return data[(size_t) ia] + (data[(size_t) ib] - data[(size_t) ia]) * frac;
}

void AviationDelay::Svf::set (float hz, float q, double sr) noexcept
{
    const double fc = juce::jlimit (10.0, sr * 0.49, (double) hz);
    g = (float) std::tan (kPiD * fc / sr);
    k = 1.f / juce::jmax (0.1f, q);
    a1 = 1.f / (1.f + g * (g + k));
    a2 = g * a1;
    a3 = g * a2;
}

float AviationDelay::Svf::lowpass (float x) noexcept
{
    const float v3 = x - ic2;
    const float v1 = a1 * ic1 + a2 * v3;
    const float v2 = ic2 + a2 * ic1 + a3 * v3;
    ic1 = 2.f * v1 - ic1;
    ic2 = 2.f * v2 - ic2;
    return v2;
}

float AviationDelay::Svf::highpass (float x) noexcept
{
    const float v3 = x - ic2;
    const float v1 = a1 * ic1 + a2 * v3;
    const float v2 = ic2 + a2 * ic1 + a3 * v3;
    ic1 = 2.f * v1 - ic1;
    ic2 = 2.f * v2 - ic2;
    return x - k * v1 - v2;
}

float AviationDelay::Shifter::process (float x, double ratio, int window) noexcept
{
    const double w = (double) window;
    if (std::abs (ratio - 1.0) < 1.0e-6)
    {
        // No shift: glide onto a single head (phase 0 = one head at half a
        // window), otherwise the two heads would sit as a static comb.
        if (phase > 0.0 && phase < 0.5)       phase = juce::jmax (0.0, phase - 0.15 / w);
        else if (phase >= 0.5 && phase < 1.0) { phase += 0.15 / w; if (phase >= 1.0) phase = 0.0; }
    }
    else
    {
        phase += (1.0 - ratio) / w;
        phase -= std::floor (phase);
    }

    const double pb = phase >= 0.5 ? phase - 0.5 : phase + 0.5;
    const float s = (float) std::sin (kPiD * phase);
    const float gA = s * s;                                   // Hann pair: gA + gB = 1
    const float y = gA * buf.read ((float) (3.0 + phase * w))
                  + (1.f - gA) * buf.read ((float) (3.0 + pb * w));
    buf.push (x);
    return y;
}

void AviationDelay::Voice::clearState() noexcept
{
    for (auto& d : diffusers)
        d.clear();
    shifter.buf.clear();
    shifter.phase = 0.0;
    loCut.clear();
    hiCut.clear();
    convAA.clear();
    convRecon.clear();
    reversePhase.fill (0.0);
    gapLp = noiseLp = satEnv = 0.f;
    compEnv = expEnv = 0.f;
    expGain = 1.f;
    holdValue = 0.f;
    holdPhase = 0.0;
    dcIn = dcOut = 0.f;
    drift = driftTarget = 0.f;
    driftCountdown = 0;
}

// =============================================================================
//  Lifecycle
// =============================================================================
double AviationDelay::syncBeats (int choice) noexcept
{
    switch (choice)
    {
        case 1:  return 0.125;          // 1/32
        case 2:  return 1.0 / 6.0;      // 1/16T
        case 3:  return 0.25;           // 1/16
        case 4:  return 0.375;          // 1/16D
        case 5:  return 1.0 / 3.0;      // 1/8T
        case 6:  return 0.5;            // 1/8
        case 7:  return 0.75;           // 1/8D
        case 8:  return 2.0 / 3.0;      // 1/4T
        case 9:  return 1.0;            // 1/4
        case 10: return 1.5;            // 1/4D
        case 11: return 2.0;            // 1/2
        case 12: return 4.0;            // 1/1
        default: return 0.0;            // free
    }
}

float AviationDelay::snappedRatio (float ratio) noexcept
{
    static constexpr std::array<float, 7> ratios { 0.25f, 1.f / 3.f, 0.375f, 0.5f, 2.f / 3.f, 0.75f, 1.f };
    float best = 1.f;
    float bestDist = 10.f;
    for (float r : ratios)
    {
        const float d = std::abs (r - ratio);
        if (d < bestDist) { bestDist = d; best = r; }
    }
    return best;
}

void AviationDelay::prepare (const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate > 0.0 ? spec.sampleRate : 44100.0;
    const double msToSamples = sampleRate * 0.001;

    // REVERSE reads up to twice the delay time behind the write head.
    const int lineLen = (int) std::ceil (sampleRate * (2.0 * kMaxTimeSec + 0.1)) + 64;
    pitchWindow = juce::jmax (256, (int) (0.05 * sampleRate));

    for (int vi = 0; vi < 2; ++vi)
    {
        auto& voice = voices[(size_t) vi];
        voice.line.allocate (lineLen);
        for (int st = 0; st < kCloudStages; ++st)
        {
            const float scale = vi == 1 ? kRightStageScale[(size_t) st] : 1.f;
            cleanStageLen[(size_t) vi][(size_t) st] = kCleanStageMs[(size_t) st] * scale * (float) msToSamples;
            cloudStageLen[(size_t) vi][(size_t) st] = kCloudStageMs[(size_t) st] * scale * (float) msToSamples;
            const float longest = juce::jmax (cleanStageLen[(size_t) vi][(size_t) st], cloudStageLen[(size_t) vi][(size_t) st]);
            voice.diffusers[(size_t) st].allocate ((int) std::ceil (longest + 2.5 * msToSamples) + 8);
        }
        voice.shifter.buf.allocate (pitchWindow + 16);
    }
    reset();
}

void AviationDelay::reset()
{
    for (auto& voice : voices)
    {
        voice.line.clear();
        voice.clearState();
        voice.timeSm = 0.0;
    }
    for (int st = 0; st < kCloudStages; ++st)
        stageModPhase[(size_t) st] = 0.137 * st;

    firstBlock = true;
    snapOnNextBlock = false;
    switchGain = 1.f;
    switchDirection = 0;
    fbCur = mixCur = 0.f;
    widthCur = 1.f;
    duckEnv = 0.f;
    duckGain = 1.f;
    diffEngage = 0.f;
    diffusersDirty = false;
    lfoPhase = lfoPhase2 = flutterPhase = 0.0;
    spliceClock = 0.0;
    degradeHold = 0;
    degradeEnv = degradeDepth = 0.f;
    rngState = 0x9e3779b9u;
}

void AviationDelay::resetVoices() noexcept
{
    for (auto& voice : voices)
        voice.clearState();
    diffusersDirty = false;
    degradeHold = 0;
    degradeEnv = 0.f;
}

float AviationDelay::nextNoise() noexcept
{
    rngState = rngState * 1664525u + 1013904223u;
    return (float) (rngState >> 8) * (2.f / 16777216.f) - 1.f;
}

// =============================================================================
//  Block configuration — everything that only needs to change per block.
// =============================================================================
void AviationDelay::configureBlock (const Values& v, const Clock& clock, BlockSettings& s) noexcept
{
    const double sr = sampleRate;
    s.mode = activeMode;
    s.style = activeStyle;
    const bool cloud = s.mode == Mode::cloud;

    // ---- time ---------------------------------------------------------------
    double timeSec = juce::jmax (0.001, (double) v[pTime] * 0.001);
    const double beats = syncBeats (juce::roundToInt (v[pSync]));
    if (beats > 0.0)
        timeSec = beats * 60.0 / juce::jmax (20.0, clock.bpm);
    timeSec = juce::jlimit (0.005, kMaxTimeSec, timeSec);

    const float ratio = juce::jlimit (0.25f, 1.f, v[pRatio] * 0.01f);
    double ratioB = 1.0;
    if (s.style == Style::dual)       ratioB = ratio;
    else if (s.style == Style::ratio) ratioB = snappedRatio (ratio);
    s.timeTarget = { timeSec * sr, timeSec * ratioB * sr };

    // QUAD: four taps on line A at T·r³, T·r², T·r, T — r = 50 % lands on
    // straight binary subdivisions of the delay time.
    const double r = ratio;
    s.quadScale = { r * r * r, r * r, r, 1.0 };

    // Tape drags when the time knob moves (varispeed); digital glides quickly.
    const double glideSec = s.mode == Mode::tape ? 0.22
                          : (s.mode == Mode::bbd || s.mode == Mode::analog) ? 0.09 : 0.045;
    s.timeCoef = coefForTau (glideSec, sr);

    // ---- levels -------------------------------------------------------------
    const float fb = pct01 (v[pFeedback]);
    s.fbTarget = cloud ? 1.f - std::pow (1.f - fb, 1.6f) : fb;   // CLOUD is voiced for long tails
    s.mixTarget = pct01 (v[pMix]);
    s.widthTarget = pct01 (v[pWidth]);
    s.duck = pct01 (v[pDuck]);
    s.duckRelease = coefForTau (0.08 + 0.3 * s.duck, sr);        // bloom time once the dry signal stops
    s.age = pct01 (v[pAge]);
    s.modDepth = pct01 (v[pModDepth]);
    s.modRate = juce::jlimit (0.01f, 20.f, v[pModRate]);
    s.envAtt = coefForTau (0.002, sr);
    s.envRel = coefForTau (0.040, sr);
    s.switchFadeSamples = juce::jmax (16, (int) (0.012 * sr));

    // ---- diffusion ------------------------------------------------------------
    const float diff = pct01 (v[pDiffusion]);
    s.diffusion = cloud ? 0.35f + 0.65f * diff : diff;
    s.diffStages = cloud ? kCloudStages : kCleanStages;
    s.diffG = cloud ? 0.45f + 0.32f * s.diffusion : 0.72f * s.diffusion;
    s.diffEngageTarget = cloud ? 1.f : juce::jlimit (0.f, 1.f, diff / 0.05f);
    s.diffSizeFactor = cloud ? 1.25f : 2.f;
    s.diffModSamples = (float) (sr * 0.001) * (cloud ? 1.6f : 0.3f) * s.diffusion * (0.35f + 0.65f * s.modDepth);
    for (int vi = 0; vi < 2; ++vi)
    {
        const auto& lens = cloud ? cloudStageLen[(size_t) vi] : cleanStageLen[(size_t) vi];
        float sum = 0.f;
        for (int st = 0; st < s.diffStages; ++st)
            sum += lens[(size_t) st];
        s.diffLatency[(size_t) vi] = sum;
    }

    // ---- filters ----------------------------------------------------------------
    float hiCut = juce::jlimit (200.f, 20000.f, v[pHiCut]);
    const float loCut = juce::jlimit (10.f, 4000.f, v[pLoCut]);
    if (s.mode == Mode::analog) hiCut = juce::jmin (hiCut, 9000.f * (1.f - 0.5f * s.age));
    if (cloud)                  hiCut = juce::jmin (hiCut, 11000.f * (1.f - 0.45f * s.age));
    for (auto& voice : voices)
    {
        voice.loCut.set (loCut, 0.707f, sr);
        voice.hiCut.set (hiCut, 0.707f, sr);
    }

    // ---- mode models ----------------------------------------------------------
    s.clockRatio = 2.0;
    s.quantSteps = 0.f;
    s.noiseLevel = 0.f;
    s.satDrive = 1.f;
    s.gapCoef = 1.f;
    s.spliceDepth = 0.f;
    s.pitchCompensation = 0.f;

    switch (s.mode)
    {
        case Mode::tape:
        {
            // head gap loss: slower tape (longer delay) loses more top end
            const float fcTape = juce::jlimit (1200.f, 18000.f,
                16000.f * std::sqrt (0.12f / (float) timeSec) * (1.f - 0.55f * s.age));
            s.gapCoef = onePoleCoef (fcTape, sr);
            s.satDrive = 1.25f + 2.5f * s.age;
            s.spliceDepth = s.age > 0.15f ? 0.4f * (s.age - 0.15f) / 0.85f : 0.f;
            s.splicePeriod = juce::jlimit (1.5, 9.0, timeSec * 12.0);
            break;
        }
        case Mode::analog:
            s.satDrive = 1.1f + 3.f * s.age;
            s.noiseLevel = 0.0003f + 0.0012f * s.age;
            break;

        case Mode::bbd:
        {
            // clock rate follows delay time (MN3005-style chain: 8192 stages
            // bright, 4096 worn); the anti-alias / reconstruction filters track it.
            const double stages = 8192.0 * (1.0 - 0.5 * s.age);
            const double clockHz = stages / (2.0 * timeSec);
            s.clockRatio = clockHz / sr;
            const float aa = juce::jlimit (400.f, 18000.f, juce::jmin (hiCut, (float) (0.42 * clockHz)));
            for (auto& voice : voices)
            {
                voice.convAA.set (aa, 0.6f, sr);
                voice.convRecon.set (aa, 0.6f, sr);
            }
            s.satDrive = 1.f + 1.5f * s.age;
            s.noiseLevel = 0.0025f + 0.008f * s.age;
            break;
        }
        case Mode::lofi:
        {
            // vintage converter: 32 kHz / 15 bit clean .. 5.5 kHz / 6 bit worn
            const double rateHz = std::exp (juce::jmap ((double) s.age, std::log (32000.0), std::log (5500.0)));
            s.clockRatio = rateHz / sr;
            const float aa = juce::jlimit (300.f, 18000.f, juce::jmin (hiCut, (float) (0.45 * rateHz)));
            for (auto& voice : voices)
            {
                voice.convAA.set (aa, 0.707f, sr);
                voice.convRecon.set (aa, 0.707f, sr);
            }
            s.quantSteps = std::pow (2.f, (15.f - 9.f * s.age) - 1.f);
            break;
        }
        case Mode::pitch:
        {
            const double semis = std::round (juce::jlimit (-24.f, 24.f, v[pPitch]));
            const double cents = 18.0 * s.modDepth;                    // MOD DEPTH = L/R detune
            const double base = std::pow (2.0, semis / 12.0);
            s.pitchRatio = { base * std::pow (2.0, cents / 1200.0), base * std::pow (2.0, -cents / 1200.0) };
            s.pitchCompensation = 0.5f * (float) pitchWindow + 3.f;
            break;
        }
        case Mode::clean:
        case Mode::reverse:
        case Mode::cloud:
        case Mode::count:
            break;
    }
}

// =============================================================================
//  Record path: filters -> diffusion -> mode model. Runs on input + feedback.
// =============================================================================
float AviationDelay::record (Voice& voice, int vi, float x, float diffScale, const BlockSettings& s) noexcept
{
    x = voice.loCut.highpass (x);
    x = voice.hiCut.lowpass (x);

    // Diffusion: series all-passes. The chain's nominal delay is subtracted
    // from the playback head, and the chain is crossfaded in over the first
    // 5 % of the knob so DIFFUSION 0 is a bit-clean delay with exact timing.
    if (diffEngage > 0.f)
    {
        const auto& lens = s.mode == Mode::cloud ? cloudStageLen[(size_t) vi] : cleanStageLen[(size_t) vi];
        float y = x;
        for (int st = 0; st < s.diffStages; ++st)
        {
            auto& ap = voice.diffusers[(size_t) st];
            const float d = ap.readLinear (lens[(size_t) st] * diffScale + s.stageMod[(size_t) st]);
            const float w = y + s.diffG * d;
            ap.push (w);
            y = d - s.diffG * w;
        }
        x += (y - x) * diffEngage;
        diffusersDirty = true;
    }

    switch (s.mode)
    {
        case Mode::tape:
        {
            // record head: asymmetric tanh with unity small-signal gain
            const float bias = 0.12f * s.age;
            const float tb = std::tanh (bias);
            x = (std::tanh (s.satDrive * x + bias) - tb) / (s.satDrive * (1.f - tb * tb));
            voice.gapLp += s.gapCoef * (x - voice.gapLp);
            x = voice.gapLp;
            // asperity noise rides the signal level
            const float a = std::abs (x);
            voice.satEnv += (a > voice.satEnv ? s.envAtt : s.envRel) * (a - voice.satEnv);
            voice.noiseLp += 0.3f * (nextNoise() - voice.noiseLp);
            x += voice.noiseLp * s.age * (0.0005f + 0.02f * voice.satEnv);
            break;
        }
        case Mode::analog:
        {
            const float d = s.satDrive;
            x = x >= 0.f ? std::tanh (d * x) / d : std::tanh (1.35f * d * x) / (1.35f * d);
            x += nextNoise() * s.noiseLevel;
            break;
        }
        case Mode::bbd:
        {
            // compressor half of the compander (the expander sits on the playback head)
            const float a = std::abs (x);
            voice.compEnv += (a > voice.compEnv ? s.envAtt : s.envRel) * (a - voice.compEnv);
            x *= juce::jmin (8.f, std::sqrt (kCompRef / juce::jmax (1.0e-4f, voice.compEnv)));
            x = voice.convAA.lowpass (x);
            if (s.clockRatio < 1.0)
            {
                voice.holdPhase += s.clockRatio;
                if (voice.holdPhase >= 1.0)
                {
                    voice.holdPhase -= 1.0;
                    voice.holdValue = x;
                }
                x = voice.holdValue;
            }
            x = softLimit (x * s.satDrive) / s.satDrive;
            x += nextNoise() * s.noiseLevel;             // expanded later -> signal-dependent hiss
            x = voice.convRecon.lowpass (x);
            break;
        }
        case Mode::lofi:
        {
            x = voice.convAA.lowpass (x);
            // magnitude truncation (not rounding) so quantised repeats always decay
            if (s.clockRatio < 1.0)
            {
                voice.holdPhase += s.clockRatio;
                if (voice.holdPhase >= 1.0)
                {
                    voice.holdPhase -= 1.0;
                    voice.holdValue = std::trunc (x * s.quantSteps) / s.quantSteps;
                }
                x = voice.holdValue;
            }
            else
            {
                x = std::trunc (x * s.quantSteps) / s.quantSteps;
            }
            x = softLimit (voice.convRecon.lowpass (juce::jlimit (-1.f, 1.f, x)));
            break;
        }
        case Mode::pitch:
            x = softLimit (voice.shifter.process (x, s.pitchRatio[(size_t) vi], pitchWindow));
            break;

        case Mode::clean:
        case Mode::reverse:
        case Mode::cloud:
        case Mode::count:
            x = softLimit (x);
            break;
    }

    if (s.mode == Mode::tape || s.mode == Mode::analog)
    {
        const float y = x - voice.dcIn + 0.9995f * voice.dcOut;
        voice.dcIn = x;
        voice.dcOut = y;
        x = y;
    }
    return x;
}

// =============================================================================
//  Playback head: forward (modulated, Hermite) or reverse (two windowed grains
//  running backwards). For forward heads `offset` is added to the tap time; for
//  reverse heads it is the distance behind the write head the grains start at.
// =============================================================================
float AviationDelay::head (Voice& voice, int tap, double tapTimeSamples, double offset, const BlockSettings& s) noexcept
{
    if (s.mode != Mode::reverse)
        return voice.line.read ((float) (tapTimeSamples + offset));

    const double t = juce::jmax (64.0, tapTimeSamples);
    auto& ph = voice.reversePhase[(size_t) tap];
    ph += 1.0 / t;
    if (ph >= 1.0)
        ph -= 1.0;
    const double pb = ph >= 0.5 ? ph - 0.5 : ph + 0.5;
    const float w = (float) std::sin (kPiD * ph);
    const float g = w * w;
    return g * voice.line.read ((float) (offset + 2.0 * t * ph))
         + (1.f - g) * voice.line.read ((float) (offset + 2.0 * t * pb));
}

// =============================================================================
//  Process
// =============================================================================
void AviationDelay::process (juce::AudioBuffer<float>& buffer, const Values& v, const Clock& clock) noexcept
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    if (n <= 0 || buffer.getNumChannels() < 2 || voices[0].line.data.empty())
        return;

    // ---- mode / style changes: fade the wet out, swap, fade back in ----------
    const auto requestedMode  = static_cast<Mode>  (juce::jlimit (0, (int) Mode::count - 1,  juce::roundToInt (v[pMode])));
    const auto requestedStyle = static_cast<Style> (juce::jlimit (0, (int) Style::count - 1, juce::roundToInt (v[pStyle])));

    if (firstBlock)
    {
        activeMode = requestedMode;
        activeStyle = requestedStyle;
        switchDirection = 0;
        switchGain = 1.f;
    }
    else if (switchDirection < 0 && switchGain <= 0.f)
    {
        const bool hadSecondLine = usesSecondLine (activeStyle);
        activeMode = requestedMode;
        activeStyle = requestedStyle;
        resetVoices();
        if (! hadSecondLine && usesSecondLine (activeStyle))
            voices[1].line.clear();                       // no ghost echoes from an old session
        switchDirection = 1;
        snapOnNextBlock = true;
    }
    else if ((requestedMode != activeMode || requestedStyle != activeStyle) && switchDirection >= 0)
    {
        switchDirection = -1;
    }

    BlockSettings s;
    configureBlock (v, clock, s);

    if (firstBlock || snapOnNextBlock)
    {
        for (int vi = 0; vi < 2; ++vi)
            voices[(size_t) vi].timeSm = s.timeTarget[(size_t) vi];
        diffEngage = s.diffEngageTarget;
        if (firstBlock)
        {
            fbCur = s.fbTarget;
            mixCur = s.mixTarget;
            widthCur = s.widthTarget;
        }
        firstBlock = false;
        snapOnNextBlock = false;
    }

    // block-rate diffusion stage modulation (keeps the smear from ringing metallic)
    for (int st = 0; st < s.diffStages; ++st)
    {
        auto& p = stageModPhase[(size_t) st];
        p += n * s.modRate * (0.23 + 0.11 * st) / sampleRate;
        p -= std::floor (p);
        s.stageMod[(size_t) st] = s.diffModSamples * (float) std::sin (kTwoPiD * p);
    }

    const float invN = 1.f / (float) n;
    const float fbInc = (s.fbTarget - fbCur) * invN;
    const float mixInc = (s.mixTarget - mixCur) * invN;
    const float widthInc = (s.widthTarget - widthCur) * invN;
    const float switchStep = 1.f / (float) s.switchFadeSamples;

    const bool reverse = s.mode == Mode::reverse;
    const bool bbd = s.mode == Mode::bbd;
    const bool tape = s.mode == Mode::tape;
    const bool secondLine = usesSecondLine (s.style);
    const float depthSamples = s.modDepth * (float) (sampleRate * 0.001);
    const double lfoInc = s.modRate / sampleRate;
    const double lfoInc2 = s.modRate * 1.37 / sampleRate;
    const double flutterInc = (s.modRate * 7.0 + 4.5) / sampleRate;
    const int driftInterval = juce::jmax (1, (int) (0.11 * sampleRate));
    const float driftCoef = coefForTau (0.25, sampleRate);
    const float duckAttack = coefForTau (0.003, sampleRate);
    const float duckEnvRelease = coefForTau (0.04, sampleRate);
    const float duckSmooth = coefForTau (0.006, sampleRate);
    const float engageCoef = coefForTau (0.08, sampleRate);
    const float degradeCoef = coefForTau (0.004, sampleRate);
    const float expRel = coefForTau (0.045, sampleRate);    // slower than the compressor -> breathing

    auto& va = voices[0];
    auto& vb = voices[1];

    auto updateDrift = [&] (Voice& voice) noexcept
    {
        if (--voice.driftCountdown <= 0)
        {
            voice.driftCountdown = driftInterval;
            voice.driftTarget = nextNoise();
        }
        voice.drift += driftCoef * (voice.driftTarget - voice.drift);
    };

    auto expand = [&] (Voice& voice, float tap) noexcept
    {
        const float a = std::abs (tap);
        voice.expEnv += (a > voice.expEnv ? s.envAtt : expRel) * (a - voice.expEnv);
        voice.expGain = juce::jlimit (0.f, 3.3f, voice.expEnv / kCompRef);
        return softLimit (tap * voice.expGain);
    };

    float* L = buffer.getWritePointer (0);
    float* R = buffer.getWritePointer (1);

    for (int i = 0; i < n; ++i)
    {
        fbCur += fbInc;
        mixCur += mixInc;
        widthCur += widthInc;
        if (switchDirection < 0)
        {
            switchGain = juce::jmax (0.f, switchGain - switchStep);
        }
        else if (switchDirection > 0)
        {
            switchGain = juce::jmin (1.f, switchGain + switchStep);
            if (switchGain >= 1.f)
                switchDirection = 0;
        }

        const float inL = L[i];
        const float inR = R[i];
        const float mono = 0.5f * (inL + inR);

        // ---- ducking: the dry signal pushes the wet down, release lets it bloom
        const float level = juce::jmax (std::abs (inL), std::abs (inR));
        duckEnv += (level > duckEnv ? duckAttack : duckEnvRelease) * (level - duckEnv);
        float duckTarget = 1.f;
        if (s.duck > 0.f)
        {
            const float db = juce::Decibels::gainToDecibels (duckEnv, -100.f);
            duckTarget = 1.f - s.duck * juce::jlimit (0.f, 1.f, (db + 50.f) / 30.f);   // full duck above -20 dB
        }
        duckGain += (duckTarget < duckGain ? duckSmooth : s.duckRelease) * (duckTarget - duckGain);

        // ---- smoothed time, diffusion engage ------------------------------
        va.timeSm += (s.timeTarget[0] - va.timeSm) * s.timeCoef;
        vb.timeSm += (s.timeTarget[1] - vb.timeSm) * s.timeCoef;
        diffEngage += engageCoef * (s.diffEngageTarget - diffEngage);
        if (s.diffEngageTarget <= 0.f && diffEngage < 1.0e-4f)
        {
            diffEngage = 0.f;
            if (diffusersDirty)
            {
                for (auto& voice : voices)
                    for (auto& d : voice.diffusers)
                        d.clear();
                diffusersDirty = false;
            }
        }

        // ---- modulation (samples) -------------------------------------------
        lfoPhase += lfoInc;
        if (lfoPhase >= 1.0) lfoPhase -= 1.0;
        lfoPhase2 += lfoInc2;
        if (lfoPhase2 >= 1.0) lfoPhase2 -= 1.0;

        float modA = 0.f, modB = 0.f;
        if (depthSamples > 0.f)
        {
            updateDrift (va);
            updateDrift (vb);
            const float sA = (float) std::sin (kTwoPiD * lfoPhase);
            const float sB = (float) std::sin (kTwoPiD * (lfoPhase + 0.25));
            switch (s.mode)
            {
                case Mode::clean:   modA = 2.f * sA; modB = 2.f * sB; break;
                case Mode::tape:
                {
                    // one tape transport: wow + flutter shared, drift per side
                    flutterPhase += flutterInc;
                    if (flutterPhase >= 1.0) flutterPhase -= 1.0;
                    const float flutter = 0.22f * (float) std::sin (kTwoPiD * flutterPhase);
                    modA = 2.4f * sA + 0.9f * va.drift + flutter;
                    modB = 2.3f * sA + 0.9f * vb.drift + flutter;
                    break;
                }
                case Mode::analog:  modA = 0.6f * sA + 1.4f * va.drift; modB = 0.6f * sB + 1.4f * vb.drift; break;
                case Mode::bbd:     modA = 3.f * triangle (lfoPhase); modB = 3.f * triangle (lfoPhase + 0.25); break;
                case Mode::lofi:    modA = 1.2f * sA + 0.2f * va.drift; modB = 1.2f * sB + 0.2f * vb.drift; break;
                case Mode::pitch:   modA = 0.6f * sA; modB = 0.6f * sB; break;
                case Mode::reverse: modA = 0.4f * sA; modB = 0.4f * sB; break;
                case Mode::cloud:
                {
                    const double ph2 = kTwoPiD * lfoPhase2;
                    modA = 2.5f * sA + 1.2f * (float) std::sin (ph2) + 0.8f * va.drift;
                    modB = 2.5f * sB + 1.2f * (float) std::cos (ph2) + 0.8f * vb.drift;
                    break;
                }
                case Mode::count: break;
            }
            modA *= depthSamples;
            modB *= depthSamples;
        }

        // ---- tape transport: splice bumps + random dropouts on worn tape -------
        float degradeGain = 1.f;
        if (tape)
        {
            spliceClock += 1.0 / sampleRate;
            if (spliceClock >= s.splicePeriod)
            {
                spliceClock -= s.splicePeriod;
                if (s.spliceDepth > 0.f)
                {
                    degradeHold = (int) (0.007 * sampleRate);
                    degradeDepth = s.spliceDepth;
                }
            }
            if (s.age > 0.5f && degradeHold <= 0
                && (nextNoise() * 0.5f + 0.5f) < (s.age - 0.5f) * 0.8f / (float) sampleRate)
            {
                degradeHold = (int) ((0.02 + 0.04 * (nextNoise() * 0.5f + 0.5f)) * sampleRate);
                degradeDepth = 0.5f * s.age;
            }
            const float target = degradeHold > 0 ? degradeDepth : 0.f;
            if (degradeHold > 0)
                --degradeHold;
            degradeEnv += degradeCoef * (target - degradeEnv);
            degradeGain = 1.f - degradeEnv;
        }

        // ---- playback heads ----------------------------------------------------
        const float scaleA = juce::jlimit (0.02f, 1.f, (float) (va.timeSm / (s.diffSizeFactor * s.diffLatency[0])));
        const float scaleB = juce::jlimit (0.02f, 1.f, (float) (vb.timeSm / (s.diffSizeFactor * s.diffLatency[1])));
        const double offA = reverse ? 4.0 + 0.4 * depthSamples + modA
                                    : -(diffEngage * scaleA * s.diffLatency[0] + s.pitchCompensation) + modA;
        const double offB = reverse ? 4.0 + 0.4 * depthSamples + modB
                                    : -(diffEngage * scaleB * s.diffLatency[1] + s.pitchCompensation) + modB;

        float tapA = head (va, kMaxTaps - 1, va.timeSm, offA, s);
        float tapB = secondLine ? head (vb, kMaxTaps - 1, vb.timeSm, offB, s) : 0.f;
        if (bbd)
        {
            tapA = expand (va, tapA);
            if (secondLine)
                tapB = expand (vb, tapB);
        }
        tapA *= degradeGain;
        tapB *= degradeGain;

        // ---- stereo routing ------------------------------------------------------
        const float fb = fbCur;
        float wetL = 0.f, wetR = 0.f, writeA = 0.f, writeB = 0.f;
        switch (s.style)
        {
            case Style::single:   wetL = wetR = tapA; writeA = mono + fb * tapA; break;
            case Style::stereo:   wetL = tapA; wetR = tapB; writeA = inL + fb * tapA;  writeB = inR + fb * tapB;  break;
            case Style::pingPong: wetL = tapA; wetR = tapB; writeA = mono + fb * tapB; writeB = fb * tapA;         break;
            case Style::dual:     wetL = tapA; wetR = tapB; writeA = mono + fb * tapA; writeB = mono + fb * tapB; break;
            case Style::ratio:    wetL = tapA; wetR = tapB; writeA = mono + fb * tapB; writeB = mono + fb * tapA; break;
            case Style::quad:
            {
                float t0 = head (va, 0, va.timeSm * s.quadScale[0], offA, s);
                float t1 = head (va, 1, va.timeSm * s.quadScale[1], offA, s);
                float t2 = head (va, 2, va.timeSm * s.quadScale[2], offA, s);
                if (bbd)
                {
                    t0 = softLimit (t0 * va.expGain);
                    t1 = softLimit (t1 * va.expGain);
                    t2 = softLimit (t2 * va.expGain);
                }
                t0 *= degradeGain;
                t1 *= degradeGain;
                t2 *= degradeGain;
                wetL = 0.72f * (0.9f * t0 + 0.25f * t1 + 0.8f * t2 + 0.45f * tapA);
                wetR = 0.72f * (0.25f * t0 + 0.9f * t1 + 0.45f * t2 + 0.8f * tapA);
                writeA = mono + fb * tapA;
                break;
            }
            case Style::count: break;
        }

        // The switch fade also gates what is recorded: the model swap resets
        // filter/shifter state, and an un-gated reset would be written into
        // the line and play back as a click one delay time later.
        va.line.push (record (va, 0, writeA, scaleA, s) * switchGain);
        if (secondLine)
            vb.line.push (record (vb, 1, writeB, scaleB, s) * switchGain);

        // ---- width, duck, mix -------------------------------------------------------
        const float mid = 0.5f * (wetL + wetR);
        const float side = 0.5f * (wetL - wetR) * widthCur;
        const float wetGain = duckGain * switchGain;
        // dry stays at unity up to 50 % mix, wet reaches unity at 50 %
        const float dryMix = juce::jmin (1.f, 2.f * (1.f - mixCur));
        const float wetMix = juce::jmin (1.f, 2.f * mixCur) * wetGain;
        L[i] = inL * dryMix + (mid + side) * wetMix;
        R[i] = inR * dryMix + (mid - side) * wetMix;
    }
}
} // namespace Mfx
