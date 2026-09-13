// =============================================================================
//  AviationReverbTests — the algorithmic reverb engine and ReverbTail wrapper
//
//  1. Every algorithm/color renders finite, audible output at common rates.
//  2. Measured RT60 (Schroeder backward integration) tracks the size mapping.
//  3. Wet energy stays near the juce::Reverb it replaced (mix/send amounts
//     in presets and MFX keep their meaning).
//  4. Stereo decorrelation, click-free algorithm switching, clean decay
//     (no limit cycles; Vintage truncation reaches exact digital silence).
//  5. ReverbTail: no dry boost at low amounts, and switching off returns the
//     untouched dry signal.
// =============================================================================

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

#include "DSP/Reverb/AviationReverb.h"
#include "DSP/ReverbTail.h"

#include <array>
#include <cmath>
#include <vector>

namespace
{
using namespace AviationReverb;

constexpr std::array<Algorithm, 5> kAlgorithms {
    Algorithm::plate, Algorithm::hall, Algorithm::room, Algorithm::cloud, Algorithm::hardware
};

struct Stereo
{
    std::vector<float> l, r;
    size_t size() const noexcept { return l.size(); }
};

Stereo makeSilence (double sr, double seconds)
{
    Stereo s;
    s.l.assign ((size_t) (sr * seconds), 0.f);
    s.r.assign ((size_t) (sr * seconds), 0.f);
    return s;
}

Stereo makeNoise (double sr, double burstSeconds, double totalSeconds, int seed)
{
    auto s = makeSilence (sr, totalSeconds);
    juce::Random rng (seed);
    const auto burst = juce::jmin (s.size(), (size_t) (sr * burstSeconds));
    for (size_t i = 0; i < burst; ++i)
    {
        s.l[i] = (rng.nextFloat() * 2.f - 1.f) * 0.5f;
        s.r[i] = (rng.nextFloat() * 2.f - 1.f) * 0.5f;
    }
    return s;
}

Stereo makeImpulse (double sr, double seconds)
{
    auto s = makeSilence (sr, seconds);
    s.l[0] = s.r[0] = 1.f;
    return s;
}

Stereo render (Engine& engine, const Stereo& in, int block = 512)
{
    Stereo out;
    out.l.assign (in.size(), 0.f);
    out.r.assign (in.size(), 0.f);
    for (size_t offset = 0; offset < in.size(); offset += (size_t) block)
    {
        const int n = (int) juce::jmin ((size_t) block, in.size() - offset);
        engine.process (in.l.data() + offset, in.r.data() + offset,
                        out.l.data() + offset, out.r.data() + offset, n);
    }
    return out;
}

Engine& prepared (Engine& engine, Algorithm a, Color c, float size, double sr, float damping = 0.4f)
{
    Settings s;
    s.algorithm = a;
    s.color = c;
    s.size = size;
    s.damping = damping;
    engine.setSettings (s);
    engine.prepare (sr, 512);
    return engine;
}

double energy (const Stereo& s, size_t from = 0, size_t to = SIZE_MAX)
{
    double e = 0.0;
    for (size_t i = from; i < juce::jmin (to, s.size()); ++i)
        e += (double) s.l[i] * s.l[i] + (double) s.r[i] * s.r[i];
    return e;
}

bool allFinite (const Stereo& s)
{
    for (size_t i = 0; i < s.size(); ++i)
        if (! std::isfinite (s.l[i]) || ! std::isfinite (s.r[i]))
            return false;
    return true;
}

/** RT60 from the Schroeder energy decay curve, T20 fit (-5 dB to -25 dB) x 3. */
double estimateRt60 (const Stereo& ir, double sr)
{
    std::vector<double> edc (ir.size() + 1, 0.0);
    for (size_t i = ir.size(); i-- > 0;)
        edc[i] = edc[i + 1] + (double) ir.l[i] * ir.l[i] + (double) ir.r[i] * ir.r[i];
    if (edc[0] <= 0.0)
        return 0.0;

    // start after the peak so pre-delay does not count as decay
    size_t start = 0;
    double peak = 0.0;
    for (size_t i = 0; i < ir.size(); ++i)
    {
        const double v = (double) ir.l[i] * ir.l[i] + (double) ir.r[i] * ir.r[i];
        if (v > peak) { peak = v; start = i; }
    }

    double t5 = -1.0, t25 = -1.0;
    for (size_t i = start; i < ir.size(); ++i)
    {
        const double db = 10.0 * std::log10 (juce::jmax (1e-30, edc[i] / edc[start]));
        if (t5 < 0.0 && db <= -5.0) t5 = (double) i / sr;
        if (db <= -25.0) { t25 = (double) i / sr; break; }
    }
    return (t5 < 0.0 || t25 < 0.0) ? 0.0 : (t25 - t5) * 3.0;
}

double maxStep (const Stereo& s, size_t from, size_t to)
{
    double m = 0.0;
    for (size_t i = juce::jmax ((size_t) 1, from); i < juce::jmin (to, s.size()); ++i)
        m = juce::jmax (m, (double) std::abs (s.l[i] - s.l[i - 1]), (double) std::abs (s.r[i] - s.r[i - 1]));
    return m;
}

const char* nameOf (Algorithm a)
{
    static const auto names = algorithmNames();
    return names[(int) a].toRawUTF8();
}
} // namespace

class AviationReverbTests : public juce::UnitTest
{
public:
    AviationReverbTests() : juce::UnitTest ("AviationReverb", "AviationReverb") {}

    void runTest() override
    {
        beginTest ("Every algorithm and color renders finite, audible wet signal");
        for (double sr : { 44100.0, 48000.0, 96000.0 })
            for (auto a : kAlgorithms)
                for (auto c : { Color::modern, Color::vintage })
                {
                    Engine engine;
                    const auto out = render (prepared (engine, a, c, 0.5f, sr), makeNoise (sr, 0.2, 1.0, 7));
                    const juce::String tag = juce::String (nameOf (a)) + (c == Color::vintage ? " vintage @" : " modern @") + juce::String (sr);
                    expect (allFinite (out), "non-finite output: " + tag);
                    expect (energy (out) > 1.0, "silent output: " + tag);
                }

        beginTest ("Decay time tracks the algorithm RT60 target");
        {
            const double sr = 48000.0;
            for (auto a : kAlgorithms)
                for (float size : { 0.2f, 0.5f, 0.8f })
                {
                    const double target = targetRt60Seconds (a, size);
                    Engine engine;
                    const auto ir = render (prepared (engine, a, Color::modern, size, sr, 0.0f),
                                            makeImpulse (sr, target * 1.6 + 0.6));
                    const double measured = estimateRt60 (ir, sr);
                    logMessage (juce::String::formatted ("  %-8s size %.1f  target %6.2fs  measured %6.2fs",
                                                         nameOf (a), size, target, measured));
                    expect (measured > target * 0.6 && measured < target * 1.5,
                            juce::String (nameOf (a)) + " RT60 off target at size " + juce::String (size));
                }
        }

        beginTest ("Wet energy stays within 3 dB of the juce::Reverb it replaces");
        {
            const double sr = 48000.0;
            const auto noise = makeNoise (sr, 1.0, 5.0, 11);

            juce::Reverb reference;
            reference.setSampleRate (sr);
            juce::Reverb::Parameters rp;
            rp.roomSize = 0.5f;
            rp.damping = 0.4f;
            rp.width = 1.f;
            rp.wetLevel = 1.f;
            rp.dryLevel = 0.f;
            reference.setParameters (rp);
            auto refOut = noise;
            for (size_t offset = 0; offset < refOut.size(); offset += 512)
            {
                const int n = (int) juce::jmin ((size_t) 512, refOut.size() - offset);
                reference.processStereo (refOut.l.data() + offset, refOut.r.data() + offset, n);
            }
            const double refEnergy = energy (refOut);

            for (auto a : kAlgorithms)
            {
                Engine engine;
                const auto out = render (prepared (engine, a, Color::modern, 0.5f, sr), noise);
                const double db = 10.0 * std::log10 (energy (out) / refEnergy);
                logMessage (juce::String::formatted ("  %-8s wet energy vs juce::Reverb: %+5.2f dB", nameOf (a), db));
                expect (std::abs (db) < 3.0, juce::String (nameOf (a)) + " wet level off by " + juce::String (db, 2) + " dB");
            }
        }

        beginTest ("Stereo tail is decorrelated");
        {
            const double sr = 48000.0;
            for (auto a : kAlgorithms)
            {
                Engine engine;
                const auto ir = render (prepared (engine, a, Color::modern, 0.5f, sr), makeImpulse (sr, 1.2));
                double lr = 0.0, ll = 0.0, rr = 0.0;
                for (size_t i = (size_t) (0.15 * sr); i < ir.size(); ++i)
                {
                    lr += (double) ir.l[i] * ir.r[i];
                    ll += (double) ir.l[i] * ir.l[i];
                    rr += (double) ir.r[i] * ir.r[i];
                }
                const double corr = lr / std::sqrt (juce::jmax (1e-30, ll * rr));
                logMessage (juce::String::formatted ("  %-8s L/R tail correlation %+.3f", nameOf (a), corr));
                expect (std::abs (corr) < 0.5, juce::String (nameOf (a)) + " tail too correlated: " + juce::String (corr, 3));
            }
        }

        beginTest ("Switching algorithm mid-signal crossfades without a click");
        {
            const double sr = 48000.0;
            for (auto from : kAlgorithms)
            {
                const auto to = kAlgorithms[((size_t) from + 1) % kAlgorithms.size()];
                Engine engine;
                prepared (engine, from, Color::modern, 0.5f, sr);
                const auto in = makeNoise (sr, 2.0, 2.0, 3);
                Stereo out;
                out.l.assign (in.size(), 0.f);
                out.r.assign (in.size(), 0.f);
                const size_t switchAt = (size_t) (sr * 1.0) / 512 * 512;
                for (size_t offset = 0; offset < in.size(); offset += 512)
                {
                    if (offset == switchAt)
                    {
                        Settings s;
                        s.algorithm = to;
                        s.size = 0.5f;
                        engine.setSettings (s);
                    }
                    const int n = (int) juce::jmin ((size_t) 512, in.size() - offset);
                    engine.process (in.l.data() + offset, in.r.data() + offset,
                                    out.l.data() + offset, out.r.data() + offset, n);
                }
                const double before = maxStep (out, switchAt - (size_t) (0.25 * sr), switchAt);
                const double after = maxStep (out, switchAt, switchAt + (size_t) (0.1 * sr));
                expect (allFinite (out));
                expect (after < before * 1.6 + 1e-4,
                        juce::String (nameOf (from)) + " -> " + nameOf (to) + " switch step "
                            + juce::String (after, 4) + " vs " + juce::String (before, 4));
            }
        }

        beginTest ("Modern tails decay cleanly with no limit cycles");
        {
            const double sr = 48000.0;
            for (auto a : kAlgorithms)
            {
                const double rt = targetRt60Seconds (a, 0.1f);
                Engine engine;
                const auto out = render (prepared (engine, a, Color::modern, 0.1f, sr),
                                         makeNoise (sr, 0.1, rt * 3.0 + 1.0, 5));
                const size_t tailStart = out.size() - (size_t) (0.1 * sr);
                const double rms = std::sqrt (energy (out, tailStart) / (2.0 * (double) (out.size() - tailStart)));
                expect (rms < 1e-5, juce::String (nameOf (a)) + " tail rms " + juce::String (rms));
            }
        }

        beginTest ("Vintage truncation reaches exact digital silence");
        {
            const double sr = 48000.0;
            Engine engine;
            const auto out = render (prepared (engine, Algorithm::plate, Color::vintage, 0.3f, sr),
                                     makeNoise (sr, 0.1, 6.0, 9));
            bool silent = true;
            for (size_t i = out.size() - (size_t) (0.5 * sr); i < out.size(); ++i)
                silent = silent && out.l[i] == 0.f && out.r[i] == 0.f;
            expect (silent, "vintage tail never reached zero");
        }

        beginTest ("ReverbTail: low amount does not boost the dry signal");
        {
            const double sr = 48000.0;
            ReverbTail tail;
            tail.prepare ({ sr, 512, 2 });
            juce::AudioBuffer<float> block (2, 512);
            double inEnergy = 0.0, outEnergy = 0.0;
            int64_t n = 0;
            juce::Random rng (21);
            for (int b = 0; b < (int) (2.0 * sr / 512); ++b)
            {
                for (int i = 0; i < 512; ++i, ++n)
                {
                    const float x = std::sin ((float) n * 0.0576f) * 0.3f + (rng.nextFloat() - 0.5f) * 0.2f;
                    block.setSample (0, i, x);
                    block.setSample (1, i, x);
                    if (b > (int) (sr / 512)) inEnergy += 2.0 * (double) x * x;
                }
                tail.process (block, 0.1f, 0.5f, true, 0.4f);
                if (b > (int) (sr / 512))
                    for (int ch = 0; ch < 2; ++ch)
                        for (int i = 0; i < 512; ++i)
                            outEnergy += (double) block.getSample (ch, i) * block.getSample (ch, i);
            }
            const double db = 10.0 * std::log10 (outEnergy / inEnergy);
            logMessage (juce::String::formatted ("  ReverbTail amount 0.1 level change: %+5.2f dB", db));
            expect (db < 1.5, "reverb at amount 0.1 raises the level by " + juce::String (db, 2) + " dB");
        }

        beginTest ("ReverbTail: switching off returns the untouched dry signal");
        {
            const double sr = 48000.0;
            ReverbTail tail;
            tail.prepare ({ sr, 512, 2 });
            juce::AudioBuffer<float> block (2, 512), original (2, 512);
            juce::Random rng (4);
            bool identicalAfterRamp = true;
            for (int b = 0; b < 200; ++b)
            {
                for (int ch = 0; ch < 2; ++ch)
                    for (int i = 0; i < 512; ++i)
                        block.setSample (ch, i, rng.nextFloat() - 0.5f);
                original.makeCopyOf (block);
                const bool on = b < 60;
                tail.process (block, 0.4f, 0.5f, on, 0.4f, Algorithm::hall, Color::modern);
                if (b >= 70)
                    for (int ch = 0; ch < 2; ++ch)
                        for (int i = 0; i < 512; ++i)
                            identicalAfterRamp = identicalAfterRamp && block.getSample (ch, i) == original.getSample (ch, i);
            }
            expect (identicalAfterRamp, "dry path not restored after reverb off");
            expect (! tail.isActive());
        }
    }
};

static AviationReverbTests aviationReverbTests;
