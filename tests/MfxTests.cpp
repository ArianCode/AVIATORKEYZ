// =============================================================================
//  MFX tests — descriptor sanity, randomiser contract, every effect renders
//  finite bounded audio, rack bypass is transparent, ptex -> slot B migration.
// =============================================================================

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

#include "DSP/Mfx/MfxDescriptors.h"
#include "DSP/Mfx/MfxEffects.h"
#include "DSP/Mfx/MfxRack.h"
#include "DSP/Mfx/AviationDelay.h"
#include <juce_audio_formats/juce_audio_formats.h>
#include <algorithm>
#include <functional>
#include "State/ParameterLayout.h"
#include "State/ApvtsStateHelpers.h"
#include "State/StateSchema.h"
#include "PluginProcessor.h"

namespace
{
constexpr double kSr = 48000.0;
constexpr int kBlock = 256;

void fillTone (juce::AudioBuffer<float>& b, int startSample, float hz = 220.f, float amp = 0.5f)
{
    for (int ch = 0; ch < b.getNumChannels(); ++ch)
        for (int i = 0; i < b.getNumSamples(); ++i)
            b.setSample (ch, i, amp * std::sin (juce::MathConstants<float>::twoPi * hz * (float) (startSample + i) / (float) kSr));
}

bool finiteAndBounded (const juce::AudioBuffer<float>& b, float bound)
{
    for (int ch = 0; ch < b.getNumChannels(); ++ch)
        for (int i = 0; i < b.getNumSamples(); ++i)
        {
            const float v = b.getSample (ch, i);
            if (! std::isfinite (v) || std::abs (v) > bound)
                return false;
        }
    return true;
}

using AD = Mfx::AviationDelay;

/** Aviation Delay values: descriptor defaults, a neutral test voicing, then overrides. */
Mfx::Values delayValues (std::initializer_list<std::pair<int, float>> overrides)
{
    const auto& d = Mfx::descriptor (Mfx::Effect::aviationDelay);
    Mfx::Values v {};
    for (int i = 0; i < Mfx::kParamsPerSlot; ++i)
        v[(size_t) i] = d.params[(size_t) i].def;
    v[AD::pSync] = 0.f;      v[AD::pTime] = 100.f;   v[AD::pFeedback] = 0.f; v[AD::pMix] = 100.f;
    v[AD::pDiffusion] = 0.f; v[AD::pModDepth] = 0.f; v[AD::pLoCut] = 20.f;   v[AD::pHiCut] = 20000.f;
    v[AD::pAge] = 0.f;       v[AD::pDuck] = 0.f;     v[AD::pWidth] = 100.f;
    for (const auto& [index, value] : overrides)
        v[(size_t) index] = value;
    return v;
}

/** Runs `fx` block by block over `input`; `perBlock` may edit the values between blocks. */
juce::AudioBuffer<float> renderDelay (Mfx::EffectProcessor& fx, const juce::AudioBuffer<float>& input, Mfx::Values v,
                                      const std::function<void (int, Mfx::Values&)>& perBlock = {})
{
    Mfx::Clock clock;
    clock.bpm = 120.0;
    clock.sampleRate = kSr;
    juce::AudioBuffer<float> out (input);
    juce::AudioBuffer<float> block (2, kBlock);
    for (int start = 0, b = 0; start < out.getNumSamples(); start += kBlock, ++b)
    {
        const int len = juce::jmin (kBlock, out.getNumSamples() - start);
        if (perBlock)
            perBlock (b, v);
        block.setSize (2, len, false, false, true);
        for (int ch = 0; ch < 2; ++ch)
            block.copyFrom (ch, 0, out, ch, start, len);
        fx.process (block, v, clock);
        for (int ch = 0; ch < 2; ++ch)
            out.copyFrom (ch, start, block, ch, 0, len);
    }
    return out;
}

juce::AudioBuffer<float> impulse (int length, float amp = 0.5f)
{
    juce::AudioBuffer<float> b (2, length);
    b.clear();
    b.setSample (0, 0, amp);
    b.setSample (1, 0, amp);
    return b;
}

juce::AudioBuffer<float> toneThenSilence (double toneSec, double totalSec, float hz, float amp)
{
    juce::AudioBuffer<float> b (2, (int) (totalSec * kSr));
    b.clear();
    const int toneLen = juce::jmin (b.getNumSamples(), (int) (toneSec * kSr));
    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < toneLen; ++i)
            b.setSample (ch, i, amp * std::sin (juce::MathConstants<float>::twoPi * hz * (float) i / (float) kSr));
    return b;
}

int peakIndex (const juce::AudioBuffer<float>& b, int ch, int from, int to)
{
    int best = from;
    for (int i = from; i < to; ++i)
        if (std::abs (b.getSample (ch, i)) > std::abs (b.getSample (ch, best)))
            best = i;
    return best;
}

double rmsRange (const juce::AudioBuffer<float>& b, int ch, double fromSec, double toSec)
{
    const int from = juce::jlimit (0, b.getNumSamples(), (int) (fromSec * kSr));
    const int to   = juce::jlimit (from, b.getNumSamples(), (int) (toSec * kSr));
    double sum = 0.0;
    for (int i = from; i < to; ++i)
        sum += (double) b.getSample (ch, i) * b.getSample (ch, i);
    return std::sqrt (sum / juce::jmax (1, to - from));
}

/** RMS of everything above ~8 kHz (two cascaded one-pole high-passes). */
double hfRms (const juce::AudioBuffer<float>& b, int ch, double fromSec, double toSec, double fc = 8000.0)
{
    const int from = juce::jlimit (0, b.getNumSamples(), (int) (fromSec * kSr));
    const int to   = juce::jlimit (from, b.getNumSamples(), (int) (toSec * kSr));
    const double a = std::exp (-2.0 * juce::MathConstants<double>::pi * fc / kSr);
    double y1 = 0, x1 = 0, y2 = 0, x2 = 0, sum = 0;
    for (int i = from; i < to; ++i)
    {
        const double x = b.getSample (ch, i);
        y1 = a * (y1 + x - x1); x1 = x;
        y2 = a * (y2 + y1 - x2); x2 = y1;
        sum += y2 * y2;
    }
    return std::sqrt (sum / juce::jmax (1, to - from));
}

/** Mean |step| exactly at block boundaries / mean |step| inside blocks.
    Well above 1 means something updates once per block and jumps — audible as a
    buzz at the block rate (187 Hz at 48 kHz / 256). */
double blockStepRatio (const juce::AudioBuffer<float>& b, int ch, double fromSec, double toSec, int blockSize)
{
    const int from = juce::jlimit (1, b.getNumSamples(), (int) (fromSec * kSr));
    const int to   = juce::jlimit (from, b.getNumSamples(), (int) (toSec * kSr));
    double edge = 0.0, interior = 0.0;
    int edgeN = 0, interiorN = 0;
    for (int i = from; i < to; ++i)
    {
        const double step = std::abs ((double) b.getSample (ch, i) - b.getSample (ch, i - 1));
        if (i % blockSize == 0) { edge += step; ++edgeN; }
        else                    { interior += step; ++interiorN; }
    }
    if (edgeN == 0 || interiorN == 0 || interior <= 0.0)
        return 1.0;
    return (edge / edgeN) / (interior / interiorN);
}

juce::String dbStr (double linear)
{
    return juce::String (20.0 * std::log10 (juce::jmax (1.0e-12, linear)), 1);
}

/** Energy-weighted RMS time spread in samples (how smeared an impulse response is). */
double energySpread (const juce::AudioBuffer<float>& b, int ch)
{
    double total = 0.0, centroid = 0.0;
    for (int i = 0; i < b.getNumSamples(); ++i)
    {
        const double e = (double) b.getSample (ch, i) * b.getSample (ch, i);
        total += e;
        centroid += e * i;
    }
    if (total <= 0.0)
        return 0.0;
    centroid /= total;
    double variance = 0.0;
    for (int i = 0; i < b.getNumSamples(); ++i)
    {
        const double e = (double) b.getSample (ch, i) * b.getSample (ch, i);
        variance += e * (i - centroid) * (i - centroid);
    }
    return std::sqrt (variance / total);
}
} // namespace

class MfxTests : public juce::UnitTest
{
public:
    MfxTests() : juce::UnitTest ("MFX", "AviatorKeyz") {}

    void runTest() override
    {
        beginTest ("Descriptors: ranges, defaults and presets are consistent");
        {
            for (int e = 0; e < (int) Mfx::Effect::count; ++e)
            {
                const auto& d = Mfx::descriptor (e);
                expect (d.numUsedParams() >= 3 && d.numUsedParams() <= Mfx::kParamsPerSlot, juce::String (d.name));
                for (int i = 0; i < Mfx::kParamsPerSlot; ++i)
                {
                    const auto& p = d.params[(size_t) i];
                    if (! p.used()) continue;
                    expect (p.max > p.min, juce::String (d.name) + "/" + p.label + " range");
                    expect (p.rollMin >= p.min - 1e-4f && p.rollMax <= p.max + 1e-4f && p.rollMax >= p.rollMin,
                            juce::String (d.name) + "/" + p.label + " roll range inside range");
                    expect (p.def >= p.min - 1e-4f && p.def <= p.max + 1e-4f, juce::String (d.name) + "/" + p.label + " default");
                    if (! p.logScale || p.min > 0.f)
                    {
                        const float back = p.denormalise (p.normalise (p.def));
                        expectWithinAbsoluteError (back, p.steps > 0 ? std::round (p.def) : p.def,
                                                   juce::jmax (1.0e-3f, (p.max - p.min) * 2.0e-3f));
                    }
                }
                for (int a = 0; a < Mfx::kNumAssigns; ++a)
                {
                    const int t = d.assignTargets[(size_t) a];
                    expect (t >= 0 && t < Mfx::kParamsPerSlot && d.params[(size_t) t].used(),
                            juce::String (d.name) + " assign target " + juce::String (a));
                }
                int presets = 0;
                for (const auto& pr : d.presets)
                {
                    if (pr.name == nullptr) continue;
                    ++presets;
                    for (int i = 0; i < Mfx::kParamsPerSlot; ++i)
                    {
                        const auto& p = d.params[(size_t) i];
                        if (! p.used()) continue;
                        expect (pr.values[(size_t) i] >= p.min - 1e-4f && pr.values[(size_t) i] <= p.max + 1e-4f,
                                juce::String (d.name) + " preset " + pr.name + " / " + p.label + " in range");
                    }
                }
                expect (presets >= 2, juce::String (d.name) + " has presets");
            }
        }

        beginTest ("Param IDs: 28 per slot, unique, lower_snake_case");
        {
            juce::StringArray ids;
            for (int s = 0; s < Mfx::kNumSlots; ++s)
            {
                ids.add (Mfx::onId (s)); ids.add (Mfx::effectId (s)); ids.add (Mfx::sendId (s)); ids.add (Mfx::levelId (s));
                for (int p = 0; p < Mfx::kParamsPerSlot; ++p) ids.add (Mfx::paramId (s, p));
                for (int a = 0; a < Mfx::kNumAssigns; ++a) { ids.add (Mfx::assignSourceId (s, a)); ids.add (Mfx::assignAmountId (s, a)); }
            }
            expectEquals (ids.size(), Mfx::kNumSlots * Mfx::kIdsPerSlot);
            for (int i = 0; i < ids.size(); ++i)
                for (int j = i + 1; j < ids.size(); ++j)
                    expect (ids[i] != ids[j], "duplicate " + ids[i]);
            expectEquals (Mfx::paramId (0, 0), juce::String ("mfx1_p01"));
            expectEquals (Mfx::paramId (1, 15), juce::String ("mfx2_p16"));
            expectEquals (Mfx::assignAmountId (1, 3), juce::String ("mfx2_asg4_amt"));
            const auto registered = AviatorKeyz::getRegisteredParameterIds();
            for (const auto& id : ids)
                expect (registered.contains (id), "registered: " + id);
        }

        beginTest ("Reroll: stays inside roll ranges, honours locks and amount");
        {
            juce::Random rng (42);
            const auto e = Mfx::Effect::aviationDelay;
            const auto& d = Mfx::descriptor (e);
            auto cur = Mfx::defaultsNormalised (e);
            for (int trial = 0; trial < 50; ++trial)
            {
                const auto out = Mfx::reroll (e, cur, 1u << Mfx::AviationDelay::pFeedback /* lock feedback */, 1.f, rng);
                expectWithinAbsoluteError (out[Mfx::AviationDelay::pFeedback], cur[Mfx::AviationDelay::pFeedback], 1e-6f);
                for (int i = 0; i < Mfx::kParamsPerSlot; ++i)
                {
                    const auto& p = d.params[(size_t) i];
                    if (! p.used() || i == Mfx::AviationDelay::pFeedback) continue;
                    const float real = p.denormalise (out[(size_t) i]);
                    expect (real >= p.rollMin - 1e-3f && real <= p.rollMax + 1e-3f,
                            juce::String (p.label) + " rolled " + juce::String (real));
                }
            }
            const auto half = Mfx::reroll (e, cur, 0, 0.f, rng);
            for (int i = 0; i < Mfx::kParamsPerSlot; ++i)
                expectWithinAbsoluteError (half[(size_t) i], cur[(size_t) i], 1e-6f);
            // unused slots never change (the delay uses all 16, the sweep filter does not)
            const auto sweep = Mfx::defaultsNormalised (Mfx::Effect::sweepFilter);
            const auto out = Mfx::reroll (Mfx::Effect::sweepFilter, sweep, 0, 1.f, rng);
            expectWithinAbsoluteError (out[15], sweep[15], 1e-6f);
        }

        beginTest ("Every effect renders finite, bounded audio at defaults and presets");
        {
            juce::dsp::ProcessSpec spec { kSr, (juce::uint32) kBlock, 2 };
            Mfx::Clock clock; clock.bpm = 120.0; clock.sampleRate = kSr;
            for (int e = 0; e < (int) Mfx::Effect::count; ++e)
            {
                std::unique_ptr<Mfx::EffectProcessor> fx (MfxRack::makeEffect (static_cast<Mfx::Effect> (e)));
                fx->prepare (spec);
                const auto& d = Mfx::descriptor (e);
                for (int preset = -1; preset < Mfx::kMaxPresets; ++preset)
                {
                    if (preset >= 0 && d.presets[(size_t) preset].name == nullptr) continue;
                    const auto norm = preset < 0 ? Mfx::defaultsNormalised (static_cast<Mfx::Effect> (e))
                                                 : Mfx::presetNormalised (static_cast<Mfx::Effect> (e), preset);
                    Mfx::Values values {};
                    for (int i = 0; i < Mfx::kParamsPerSlot; ++i)
                        values[(size_t) i] = d.params[(size_t) i].used() ? d.params[(size_t) i].denormalise (norm[(size_t) i]) : 0.f;
                    fx->reset();
                    juce::AudioBuffer<float> buf (2, kBlock);
                    float energy = 0.f;
                    for (int b = 0; b < 200; ++b) // ~1 s
                    {
                        fillTone (buf, b * kBlock);
                        fx->process (buf, values, clock);
                        clock.beatPos += kBlock * clock.beatsPerSample();
                        expect (finiteAndBounded (buf, 4.f), juce::String (d.name) + " preset " + juce::String (preset) + " block " + juce::String (b));
                        energy += buf.getRMSLevel (0, 0, kBlock);
                    }
                    expect (energy > 1.0e-3f, juce::String (d.name) + " produces output");
                }
            }
        }

        beginTest ("Rack: slots off are transparent; slot on changes the signal");
        {
            AviatorKeyzProcessor proc;
            proc.prepareToPlay (kSr, kBlock);
            auto& apvts = proc.getAPVTS();
            auto set = [&] (const juce::String& id, float v) { auto* p = apvts.getParameter (id); p->setValueNotifyingHost (p->convertTo0to1 (v)); };

            MfxRack rack;
            rack.attachParameters (apvts);
            rack.prepare ({ kSr, (juce::uint32) kBlock, 2 });
            Mfx::Clock clock; clock.sampleRate = kSr;
            MfxRack::ModSources mods;
            MfxRack::TextureMacroOffsets tex;

            juce::AudioBuffer<float> a (2, kBlock), b (2, kBlock);
            fillTone (a, 0); fillTone (b, 0);
            rack.process (a, clock, mods, tex);
            for (int i = 0; i < kBlock; ++i)
                expectWithinAbsoluteError (a.getSample (0, i), b.getSample (0, i), 1e-6f);

            set (Mfx::onId (0), 1.f);
            set (Mfx::effectId (0), (float) Mfx::Effect::bitcrusher);
            set (Mfx::paramId (0, 0), Mfx::descriptor (Mfx::Effect::bitcrusher).params[0].normalise (3.f)); // 3 bits
            set (Mfx::paramId (0, 4), 1.f); // mix 100%
            fillTone (a, 0);
            for (int r = 0; r < 4; ++r) rack.process (a, clock, mods, tex);
            fillTone (a, 0);
            rack.process (a, clock, mods, tex);
            float diff = 0.f;
            for (int i = 0; i < kBlock; ++i)
                diff += std::abs (a.getSample (0, i) - b.getSample (0, i));
            expect (diff / kBlock > 0.01f, "3-bit crush must audibly change the tone: " + juce::String (diff / kBlock));
            expect (rack.getSlotLevel (0) > 0.05f);
        }

        beginTest ("Migration: pre-rack state with ptex_on lands in slot B as Grain Cloud");
        {
            juce::ValueTree state ("AviatorKeyzState");
            auto add = [&] (const juce::String& id, float v)
            {
                juce::ValueTree p ("PARAM");
                p.setProperty ("id", id, nullptr);
                p.setProperty ("value", v, nullptr);
                state.appendChild (p, nullptr);
            };
            add (AviatorKeyz::ParamID::PTEX_ON, 1.f);
            add (AviatorKeyz::ParamID::PTEX_MIX, 0.6f);
            add (AviatorKeyz::ParamID::PTEX_DENSITY, 0.8f);
            AviatorKeyz::migrateLegacyAdvancedParams (state);

            auto get = [&] (const juce::String& id, float& out)
            {
                for (int i = 0; i < state.getNumChildren(); ++i)
                    if (state.getChild (i).getProperty ("id").toString() == id) { out = state.getChild (i).getProperty ("value"); return true; }
                return false;
            };
            float v = -1.f;
            expect (get (Mfx::onId (1), v) && v > 0.5f, "slot B on");
            expect (get (Mfx::effectId (1), v) && (int) v == (int) Mfx::Effect::grainCloud, "slot B grain cloud");
            expect (get (Mfx::paramId (1, 10), v)); expectWithinAbsoluteError (v, 0.6f, 1e-5f);
            expect (get (Mfx::paramId (1, 1), v)); expectWithinAbsoluteError (v, 0.8f, 1e-5f);

            // a state that already has the rack is left alone
            juce::ValueTree fresh ("AviatorKeyzState");
            juce::ValueTree p ("PARAM"); p.setProperty ("id", Mfx::effectId (1), nullptr); p.setProperty ("value", 3.f, nullptr);
            fresh.appendChild (p, nullptr);
            juce::ValueTree q ("PARAM"); q.setProperty ("id", AviatorKeyz::ParamID::PTEX_ON, nullptr); q.setProperty ("value", 1.f, nullptr);
            fresh.appendChild (q, nullptr);
            AviatorKeyz::migrateLegacyAdvancedParams (fresh);
            bool foundOn = false;
            for (int i = 0; i < fresh.getNumChildren(); ++i)
                if (fresh.getChild (i).getProperty ("id").toString() == Mfx::onId (1)) foundOn = true;
            expect (! foundOn, "no migration when the rack params already exist");
        }

        // ---------------------------------------------------------------------
        //  Aviation Delay (multi-model delay, slot effect 2)
        // ---------------------------------------------------------------------
        const juce::dsp::ProcessSpec delaySpec { kSr, (juce::uint32) kBlock, 2 };
        const auto& delayDesc = Mfx::descriptor (Mfx::Effect::aviationDelay);
        auto modeName = [&] (AD::Mode m) { return Mfx::valueText (delayDesc.params[AD::pMode], (float) (int) m); };

        beginTest ("Aviation Delay: the echo lands on the delay time (clean, tape, analog, pitch at 0 st)");
        {
            const int T = 4800; // 100 ms
            for (auto mode : { AD::Mode::clean, AD::Mode::tape, AD::Mode::analog, AD::Mode::pitch })
            {
                AD fx;
                fx.prepare (delaySpec);
                const auto out = renderDelay (fx, impulse (T * 2), delayValues ({ { AD::pMode, (float) (int) mode },
                                                                                    { AD::pStyle, (float) (int) AD::Style::single },
                                                                                    { AD::pPitch, 0.f } }));
                const int peak = peakIndex (out, 0, 1, out.getNumSamples());
                expect (std::abs (peak - T) <= 2, modeName (mode) + " echo at sample " + juce::String (peak));
                expect (std::abs (out.getSample (0, peak)) > 0.2f, modeName (mode) + " echo level");
            }
        }

        beginTest ("Aviation Delay: PING-PONG starts left and answers right");
        {
            const int T = 4800;
            AD fx;
            fx.prepare (delaySpec);
            const auto out = renderDelay (fx, impulse (T * 3), delayValues ({ { AD::pStyle, (float) (int) AD::Style::pingPong },
                                                                                { AD::pFeedback, 50.f } }));
            auto level = [&] (int ch, int at) { return std::abs (out.getSample (ch, peakIndex (out, ch, at - 3, at + 4))); };
            expect (level (0, T) > 0.3f && level (1, T) < 0.02f, "first repeat on the left only");
            expect (level (1, 2 * T) > 0.15f && level (0, 2 * T) < 0.02f, "second repeat on the right only");
        }

        beginTest ("Aviation Delay: DIFFUSION smears a repeat into a wash");
        {
            auto spread = [&] (float diffusion)
            {
                AD fx;
                fx.prepare (delaySpec);
                const auto out = renderDelay (fx, impulse ((int) kSr), delayValues ({ { AD::pStyle, (float) (int) AD::Style::single },
                                                                                        { AD::pTime, 200.f },
                                                                                        { AD::pDiffusion, diffusion } }));
                return energySpread (out, 0);
            };
            const double tight = spread (0.f), washed = spread (100.f);
            expect (tight < 150.0, "diffusion 0 keeps the repeat tight: " + juce::String (tight, 1));
            expect (washed > 1000.0, "diffusion 100 spreads the repeat: " + juce::String (washed, 1));
        }

        beginTest ("Aviation Delay: PITCH +12 st repeats an octave up");
        {
            AD fx;
            fx.prepare (delaySpec);
            const auto out = renderDelay (fx, toneThenSilence (1.5, 1.5, 220.f, 0.5f),
                                          delayValues ({ { AD::pMode, (float) (int) AD::Mode::pitch },
                                                         { AD::pStyle, (float) (int) AD::Style::single },
                                                         { AD::pPitch, 12.f } }));
            const int from = (int) (0.5 * kSr), to = (int) (1.5 * kSr);
            int crossings = 0;
            for (int i = from + 1; i < to; ++i)
                if (out.getSample (0, i - 1) <= 0.f && out.getSample (0, i) > 0.f)
                    ++crossings;
            const double hz = crossings / ((to - from) / kSr);
            expect (hz > 400.0 && hz < 480.0, "wet frequency " + juce::String (hz, 1) + " Hz");
        }

        beginTest ("Aviation Delay: DUCK holds the wet down while playing and blooms after");
        {
            const auto in = toneThenSilence (1.5, 2.5, 330.f, 0.5f);
            auto wetRms = [&] (float duck, double fromSec, double toSec)
            {
                AD fx;
                fx.prepare (delaySpec);
                const auto out = renderDelay (fx, in, delayValues ({ { AD::pTime, 150.f }, { AD::pFeedback, 60.f },
                                                                       { AD::pMix, 50.f }, { AD::pDuck, duck } }));
                double sum = 0.0;
                const int from = (int) (fromSec * kSr), to = (int) (toSec * kSr);
                for (int i = from; i < to; ++i)
                {
                    const double w = out.getSample (0, i) - in.getSample (0, i);   // dry is unity at 50 % mix
                    sum += w * w;
                }
                return std::sqrt (sum / (to - from));
            };
            const double openPlaying = wetRms (0.f, 0.8, 1.5), duckedPlaying = wetRms (100.f, 0.8, 1.5);
            const double openTail = wetRms (0.f, 1.9, 2.4), duckedTail = wetRms (100.f, 1.9, 2.4);
            expect (openPlaying > 0.05, "wet is audible without ducking");
            expect (duckedPlaying < 0.2 * openPlaying,
                    "ducked while playing: " + juce::String (duckedPlaying, 4) + " vs " + juce::String (openPlaying, 4));
            expect (duckedTail > 0.35 * openTail,
                    "tail blooms after the input stops: " + juce::String (duckedTail, 4) + " vs " + juce::String (openTail, 4));
        }

        beginTest ("Aviation Delay: every mode x style stays finite and bounded at 100 % feedback");
        {
            const auto in = toneThenSilence (2.0, 3.0, 220.f, 0.8f);
            for (int mode = 0; mode < (int) AD::Mode::count; ++mode)
                for (int style = 0; style < (int) AD::Style::count; ++style)
                {
                    AD fx;
                    fx.prepare (delaySpec);
                    const auto out = renderDelay (fx, in, delayValues ({ { AD::pMode, (float) mode }, { AD::pStyle, (float) style },
                                                                           { AD::pTime, 90.f }, { AD::pFeedback, 100.f },
                                                                           { AD::pDiffusion, 100.f }, { AD::pAge, 100.f },
                                                                           { AD::pModDepth, 100.f }, { AD::pRatio, 50.f } }));
                    expect (finiteAndBounded (out, 4.f), "mode " + juce::String (mode) + " style " + juce::String (style));
                }
        }

        beginTest ("Aviation Delay: switching mode and style mid-stream is click-free");
        {
            AD fx;
            fx.prepare (delaySpec);
            const auto in = toneThenSilence (4.0, 4.0, 220.f, 0.4f);
            int mode = 0, style = 1;
            const auto out = renderDelay (fx, in, delayValues ({ { AD::pTime, 180.f }, { AD::pFeedback, 55.f },
                                                                   { AD::pMix, 50.f }, { AD::pModDepth, 20.f } }),
                                          [&] (int block, Mfx::Values& values)
                                          {
                                              if (block > 0 && block % 40 == 0)
                                              {
                                                  mode = (mode + 3) % (int) AD::Mode::count;
                                                  style = (style + 1) % (int) AD::Style::count;
                                                  values[AD::pMode] = (float) mode;
                                                  values[AD::pStyle] = (float) style;
                                              }
                                          });
            float maxStep = 0.f;
            int maxAt = 0;
            for (int ch = 0; ch < 2; ++ch)
                for (int i = 1; i < out.getNumSamples(); ++i)
                {
                    const float step = std::abs (out.getSample (ch, i) - out.getSample (ch, i - 1));
                    if (step > maxStep) { maxStep = step; maxAt = i; }
                }
            expect (finiteAndBounded (out, 4.f), "bounded through switches");
            expect (maxStep < 0.25f, "largest sample step " + juce::String (maxStep) + " at block " + juce::String (maxAt / kBlock)
                                     + " (switch every 40 blocks)");
        }

        beginTest ("Saturator adds harmonics without acting as a volume knob");
        {
            const auto& d = Mfx::descriptor (Mfx::Effect::saturator);
            juce::AudioBuffer<float> in (2, (int) kSr);
            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < in.getNumSamples(); ++i)
                    in.setSample (ch, i, 0.3f * std::sin (juce::MathConstants<float>::twoPi * 220.f * (float) i / (float) kSr));

            auto levelAtDrive = [&] (float drivePct)
            {
                Mfx::Values v {};
                for (int i = 0; i < Mfx::kParamsPerSlot; ++i)
                    v[(size_t) i] = d.params[(size_t) i].def;
                v[0] = drivePct;   // Drive
                v[3] = 0.f;        // Out 0 dB
                v[4] = 100.f;      // Mix 100 %
                std::unique_ptr<Mfx::EffectProcessor> fx (MfxRack::makeEffect (Mfx::Effect::saturator));
                fx->prepare (delaySpec);
                return rmsRange (renderDelay (*fx, in, v), 0, 0.2, 0.9);
            };

            const double quiet = levelAtDrive (10.f), loud = levelAtDrive (85.f);
            const double shiftDb = 20.0 * std::log10 (loud / juce::jmax (1.0e-9, quiet));
            expect (std::abs (shiftDb) < 4.0,
                    "drive 10 % -> 85 % shifted level by " + juce::String (shiftDb, 1) + " dB");
        }

        beginTest ("Grain Cloud actually spawns grains (and adds no noise of its own)");
        {
            // 1 s of silence, 1 s of tone, then silence: a working cloud keeps
            // granulating the captured tone after the input stops.
            juce::AudioBuffer<float> in (2, (int) (4.0 * kSr));
            in.clear();
            for (int ch = 0; ch < 2; ++ch)
                for (int i = (int) (1.0 * kSr); i < (int) (2.0 * kSr); ++i)
                    in.setSample (ch, i, 0.5f * std::sin (juce::MathConstants<float>::twoPi * 220.f * (float) i / (float) kSr));

            const auto& d = Mfx::descriptor (Mfx::Effect::grainCloud);
            Mfx::Values values {};
            for (int i = 0; i < Mfx::kParamsPerSlot; ++i)
                values[(size_t) i] = d.params[(size_t) i].def;

            std::unique_ptr<Mfx::EffectProcessor> fx (MfxRack::makeEffect (Mfx::Effect::grainCloud));
            fx->prepare (delaySpec);
            const auto out = renderDelay (*fx, in, values);

            expect (rmsRange (out, 0, 0.0, 0.95) < 1.0e-6,
                    "silent in, silent out: " + dbStr (rmsRange (out, 0, 0.0, 0.95)) + " dB");
            const double tail = rmsRange (out, 0, 2.2, 3.5);
            expect (tail > 0.002, "grains keep sounding after the input stops: " + dbStr (tail) + " dB");
            // grains replay captured tone, so the tail must not be broadband noise
            expect (hfRms (out, 0, 2.2, 3.5) < 0.25 * tail,
                    "the tail is tone, not white noise: HF " + dbStr (hfRms (out, 0, 2.2, 3.5)) + " dB");
        }

        // AVIATORKEYZ_DELAY_DEMO_DIR=<dir> renders every factory preset over a short
        // plucked phrase so the modes can be auditioned without a host.
        const auto demoDir = juce::SystemStats::getEnvironmentVariable ("AVIATORKEYZ_DELAY_DEMO_DIR", {});
        if (demoDir.isNotEmpty())
        {
            beginTest ("Aviation Delay: render preset demos");
            const juce::File dir (demoDir);
            dir.createDirectory();

            // C4 E4 G4 B4 plucks, 8th notes at 120 BPM, then a long tail
            juce::AudioBuffer<float> phrase (2, (int) (6.0 * kSr));
            phrase.clear();
            const float notes[] { 261.63f, 329.63f, 392.0f, 493.88f };
            for (int k = 0; k < 4; ++k)
            {
                const int start = (int) (k * 0.25 * kSr);
                for (int i = 0; i < (int) (0.9 * kSr) && start + i < phrase.getNumSamples(); ++i)
                {
                    const float t = (float) i / (float) kSr;
                    const float env = std::exp (-t * 6.f) * juce::jmin (1.f, t * 400.f);
                    const float w = juce::MathConstants<float>::twoPi * notes[k] * t;
                    const float s = 0.3f * env * (std::sin (w) + 0.4f * std::sin (2.f * w) + 0.15f * std::sin (3.f * w));
                    phrase.addSample (0, start + i, s);
                    phrase.addSample (1, start + i, s);
                }
            }

            auto writeWav = [&] (const juce::String& name, const juce::AudioBuffer<float>& audio)
            {
                const auto file = dir.getChildFile (name + ".wav");
                file.deleteFile();
                juce::WavAudioFormat wav;
                std::unique_ptr<juce::OutputStream> stream (file.createOutputStream());
                std::unique_ptr<juce::AudioFormatWriter> writer;
                if (stream != nullptr)   // on success the writer takes ownership of the stream
                    writer = wav.createWriterFor (stream, juce::AudioFormatWriterOptions{}.withSampleRate (kSr)
                                                                                         .withNumChannels (2)
                                                                                         .withBitsPerSample (24));
                expect (writer != nullptr, "writer for " + file.getFileName());
                if (writer != nullptr)
                    expect (writer->writeFromAudioSampleBuffer (audio, 0, audio.getNumSamples()));
            };

            writeWav ("0_Dry", phrase);
            for (int p = 0; p < Mfx::kMaxPresets; ++p)
            {
                const auto& pr = delayDesc.presets[(size_t) p];
                if (pr.name == nullptr)
                    continue;
                const auto norm = Mfx::presetNormalised (Mfx::Effect::aviationDelay, p);
                Mfx::Values values {};
                for (int i = 0; i < Mfx::kParamsPerSlot; ++i)
                    values[(size_t) i] = delayDesc.params[(size_t) i].denormalise (norm[(size_t) i]);
                AD fx;
                fx.prepare (delaySpec);
                writeWav (juce::String (p + 1) + "_" + juce::String (pr.name).replaceCharacter (' ', '_'),
                          renderDelay (fx, phrase, values));
            }
            logMessage ("Aviation Delay demos written to " + dir.getFullPathName());
        }

        // AVIATORKEYZ_DSP_DIAG=1 prints a noise / discontinuity table (not an assertion).
        if (juce::SystemStats::getEnvironmentVariable ("AVIATORKEYZ_DSP_DIAG", {}).isNotEmpty())
        {
            beginTest ("DSP diagnostics: noise floor and block-rate discontinuities");

            // 1 s of true silence, 1 s of tone, then 3 s of silence for the tail
            juce::AudioBuffer<float> in (2, (int) (5.0 * kSr));
            in.clear();
            for (int ch = 0; ch < 2; ++ch)
                for (int i = (int) (1.0 * kSr); i < (int) (2.0 * kSr); ++i)
                    in.setSample (ch, i, 0.5f * std::sin (juce::MathConstants<float>::twoPi * 220.f * (float) i / (float) kSr));

            auto row = [&] (const juce::String& label, const juce::AudioBuffer<float>& out)
            {
                logMessage (label.paddedRight (' ', 22)
                            + ("silence " + dbStr (rmsRange (out, 0, 0.0, 0.95))).paddedRight (' ', 16)
                            + ("tail " + dbStr (rmsRange (out, 0, 3.0, 5.0))).paddedRight (' ', 14)
                            + ("tailHF>8k " + dbStr (hfRms (out, 0, 3.0, 5.0))).paddedRight (' ', 19)
                            + "blockStep x" + juce::String (blockStepRatio (out, 0, 1.0, 3.0, kBlock), 2));
            };

            for (int diffusion : { 0, 30 })
            {
                logMessage ("--- Aviation Delay, diffusion " + juce::String (diffusion) + " %, age 40 %, feedback 50 %, mix 100 %");
                for (int mode = 0; mode < (int) AD::Mode::count; ++mode)
                {
                    AD fx;
                    fx.prepare (delaySpec);
                    const auto out = renderDelay (fx, in, delayValues ({ { AD::pMode, (float) mode },
                                                                           { AD::pStyle, (float) (int) AD::Style::stereo },
                                                                           { AD::pTime, 300.f }, { AD::pFeedback, 50.f },
                                                                           { AD::pMix, 100.f }, { AD::pDiffusion, (float) diffusion },
                                                                           { AD::pAge, 40.f }, { AD::pModDepth, 20.f } }));
                    row (Mfx::valueText (delayDesc.params[AD::pMode], (float) mode), out);
                }
            }

            logMessage ("--- other slot effects at their defaults (mix as per descriptor)");
            for (auto e : { Mfx::Effect::grainCloud, Mfx::Effect::saturator, Mfx::Effect::space })
            {
                const auto& d = Mfx::descriptor (e);
                Mfx::Values values {};
                for (int i = 0; i < Mfx::kParamsPerSlot; ++i)
                    values[(size_t) i] = d.params[(size_t) i].def;
                std::unique_ptr<Mfx::EffectProcessor> fx (MfxRack::makeEffect (e));
                fx->prepare (delaySpec);
                row (d.name, renderDelay (*fx, in, values));
            }
        }
    }
};

static MfxTests mfxTests;
