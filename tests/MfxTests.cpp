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
            const auto e = Mfx::Effect::tapeEcho;
            const auto& d = Mfx::descriptor (e);
            auto cur = Mfx::defaultsNormalised (e);
            for (int trial = 0; trial < 50; ++trial)
            {
                const auto out = Mfx::reroll (e, cur, 1u << 2 /* lock feedback */, 1.f, rng);
                expectWithinAbsoluteError (out[2], cur[2], 1e-6f);
                for (int i = 0; i < Mfx::kParamsPerSlot; ++i)
                {
                    const auto& p = d.params[(size_t) i];
                    if (! p.used() || i == 2) continue;
                    const float real = p.denormalise (out[(size_t) i]);
                    expect (real >= p.rollMin - 1e-3f && real <= p.rollMax + 1e-3f,
                            juce::String (p.label) + " rolled " + juce::String (real));
                }
            }
            const auto half = Mfx::reroll (e, cur, 0, 0.f, rng);
            for (int i = 0; i < Mfx::kParamsPerSlot; ++i)
                expectWithinAbsoluteError (half[(size_t) i], cur[(size_t) i], 1e-6f);
            // unused slots never change
            const auto out = Mfx::reroll (e, cur, 0, 1.f, rng);
            expectWithinAbsoluteError (out[15], cur[15], 1e-6f);
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
    }
};

static MfxTests mfxTests;
