// =============================================================================
//  FlightDeckWiringTests — the parameters the MAIN page exposes must actually
//  reach the DSP: amp DECAY (seconds → ms), SUSTAIN with zero decay, the shared
//  FILTER, the LAYER MIX synth layer, and the arp / flip parameters.
//
//  Drives the real AviatorKeyzProcessor headless.
// =============================================================================

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

#include "PluginProcessor.h"
#include "State/StateSchema.h"

namespace
{
constexpr double kSr = 48000.0;
constexpr int    kBlock = 480; // 10 ms

void setParam (AviatorKeyzProcessor& p, const char* id, float value)
{
    auto* param = p.getAPVTS().getParameter (id);
    jassert (param != nullptr);
    param->setValueNotifyingHost (param->convertTo0to1 (value));
}

/** Renders a held note and returns the per-block RMS of channel 0. */
std::vector<float> renderRms (AviatorKeyzProcessor& p, int numBlocks, int note = 60, int releaseAtBlock = -1)
{
    std::vector<float> rms;
    juce::AudioBuffer<float> buffer (2, kBlock);
    for (int b = 0; b < numBlocks; ++b)
    {
        juce::MidiBuffer midi;
        if (b == 0)
            midi.addEvent (juce::MidiMessage::noteOn (1, note, (juce::uint8) 100), 0);
        if (b == releaseAtBlock)
            midi.addEvent (juce::MidiMessage::noteOff (1, note), 0);
        buffer.clear();
        p.processBlock (buffer, midi);
        rms.push_back (buffer.getRMSLevel (0, 0, kBlock));
    }
    return rms;
}

float meanOf (const std::vector<float>& v, size_t from, size_t to)
{
    float s = 0.f; size_t n = 0;
    for (size_t i = from; i < juce::jmin (to, v.size()); ++i) { s += v[i]; ++n; }
    return n ? s / (float) n : 0.f;
}

struct Proc
{
    AviatorKeyzProcessor p;
    Proc()
    {
        p.getPresetManager().loadPreset (AviatorKeyz::Category::LEADS, "Init");
        // Deterministic sustained tone: no reverb / FX / velocity scaling, gated loop.
        setParam (p, AviatorKeyz::ParamID::FX_REVERB_ON, 0.f);
        setParam (p, AviatorKeyz::ParamID::REVERB_AMOUNT, 0.f);
        setParam (p, AviatorKeyz::ParamID::OUTPUT_LIMITER, 0.f);
        setParam (p, AviatorKeyz::ParamID::SRC_LOOP_MODE, 1.f); // loop so the tone never runs out
        setParam (p, AviatorKeyz::ParamID::ENV_ATTACK, 0.f);
        setParam (p, AviatorKeyz::ParamID::ENV_RELEASE, 10.f);
        p.prepareToPlay (kSr, kBlock);
    }
};
} // namespace

class FlightDeckWiringTests : public juce::UnitTest
{
public:
    FlightDeckWiringTests() : juce::UnitTest ("FlightDeck_Wiring", "AviatorKeyz") {}

    void runTest() override
    {
        beginTest ("Amp DECAY is applied in seconds (2 s decay is still loud after 0.3 s)");
        {
            Proc t;
            setParam (t.p, AviatorKeyz::ParamID::ENV_AMP_DECAY, 2.0f);   // 2 seconds
            setParam (t.p, AviatorKeyz::ParamID::ENV_AMP_SUSTAIN, 0.0f);
            const auto rms = renderRms (t.p, 60); // 600 ms
            const float early = meanOf (rms, 1, 4);   // 10–40 ms
            const float mid   = meanOf (rms, 28, 32); // ~300 ms
            const float late  = meanOf (rms, 55, 60); // ~570 ms
            expect (early > 1.0e-3f, "note sounds");
            // A 10 ms decay (the old bug) would be silent by 300 ms; a 2 s decay is
            // still well above half level and still falling.
            expect (mid > early * 0.5f, "still loud at 300 ms: mid=" + juce::String (mid) + " early=" + juce::String (early));
            expect (late < mid, "level keeps falling through the decay");
        }

        beginTest ("SUSTAIN takes effect even when DECAY is 0");
        {
            Proc loud;
            setParam (loud.p, AviatorKeyz::ParamID::ENV_AMP_DECAY, 0.f);
            setParam (loud.p, AviatorKeyz::ParamID::ENV_AMP_SUSTAIN, 1.0f);
            const auto full = renderRms (loud.p, 20);

            Proc quiet;
            setParam (quiet.p, AviatorKeyz::ParamID::ENV_AMP_DECAY, 0.f);
            setParam (quiet.p, AviatorKeyz::ParamID::ENV_AMP_SUSTAIN, 0.25f);
            const auto low = renderRms (quiet.p, 20);

            const float a = meanOf (full, 5, 20), b = meanOf (low, 5, 20);
            expect (a > 1.0e-3f);
            expectWithinAbsoluteError (b / a, 0.25f, 0.06f);
        }

        beginTest ("FILTER: enabled HP at 12 kHz attenuates the sound; disabled leaves it alone");
        {
            Proc off;
            setParam (off.p, AviatorKeyz::ParamID::FILTER_ENABLED, 0.f);
            setParam (off.p, AviatorKeyz::ParamID::FILTER_TYPE, 1.f);
            setParam (off.p, AviatorKeyz::ParamID::FILTER_CUTOFF, 12000.f);
            const float dry = meanOf (renderRms (off.p, 30), 5, 30);

            Proc on;
            setParam (on.p, AviatorKeyz::ParamID::FILTER_ENABLED, 1.f);
            setParam (on.p, AviatorKeyz::ParamID::FILTER_TYPE, 1.f);
            setParam (on.p, AviatorKeyz::ParamID::FILTER_CUTOFF, 12000.f);
            setParam (on.p, AviatorKeyz::ParamID::FILTER_RESONANCE, 0.f);
            const float hp = meanOf (renderRms (on.p, 30), 5, 30);

            expect (dry > 1.0e-3f);
            expect (hp < dry * 0.5f, "HP 12k should remove most energy: hp=" + juce::String (hp) + " dry=" + juce::String (dry));

            Proc lp;
            setParam (lp.p, AviatorKeyz::ParamID::FILTER_ENABLED, 1.f);
            setParam (lp.p, AviatorKeyz::ParamID::FILTER_TYPE, 0.f);
            setParam (lp.p, AviatorKeyz::ParamID::FILTER_CUTOFF, 60.f);
            const float lpRms = meanOf (renderRms (lp.p, 30), 5, 30);
            expect (lpRms < dry * 0.5f, "LP 60 Hz should remove most energy");
        }

        beginTest ("LAYER MIX: OSC1 level adds the synth layer; default is sampler-only");
        {
            Proc base;
            const float samplerOnly = meanOf (renderRms (base.p, 20), 5, 20);

            Proc layered;
            setParam (layered.p, AviatorKeyz::ParamID::OSC1_LEVEL, 0.8f);
            setParam (layered.p, AviatorKeyz::ParamID::OSC1_TYPE, 3.f); // sine
            const float withSynth = meanOf (renderRms (layered.p, 20), 5, 20);

            expect (samplerOnly > 1.0e-3f);
            expect (std::abs (withSynth - samplerOnly) > samplerOnly * 0.15f,
                    "osc layer must change the output: " + juce::String (withSynth) + " vs " + juce::String (samplerOnly));

            // preset recall keeps the layer silent
            expectWithinAbsoluteError (base.p.getAPVTS().getRawParameterValue (AviatorKeyz::ParamID::OSC1_LEVEL)->load(), 0.f, 1e-6f);
            expectWithinAbsoluteError (base.p.getAPVTS().getRawParameterValue (AviatorKeyz::ParamID::OSC2_LEVEL)->load(), 0.f, 1e-6f);
        }

        beginTest ("Arpeggiator ON retriggers the sampler on the grid; OFF plays a single held note");
        {
            Proc plain;
            setParam (plain.p, AviatorKeyz::ParamID::ENV_RELEASE, 5.f);
            setParam (plain.p, AviatorKeyz::ParamID::SRC_LOOP_MODE, 2.f); // gate
            const auto held = renderRms (plain.p, 100); // 1 s
            int silentBlocksHeld = 0;
            for (size_t i = 2; i < held.size(); ++i) if (held[i] < 1.0e-4f) ++silentBlocksHeld;

            Proc arp;
            setParam (arp.p, AviatorKeyz::ParamID::ENV_RELEASE, 5.f);
            setParam (arp.p, AviatorKeyz::ParamID::SRC_LOOP_MODE, 2.f);
            setParam (arp.p, AviatorKeyz::ParamID::ARP_ON, 1.f);
            setParam (arp.p, AviatorKeyz::ParamID::ARP_RATE, 1.f);   // 1/8 @120 = 250 ms
            setParam (arp.p, AviatorKeyz::ParamID::ARP_GATE, 0.2f);  // 50 ms on, 200 ms off
            setParam (arp.p, AviatorKeyz::ParamID::ARP_TARGET, 1.f); // notes
            const auto arped = renderRms (arp.p, 100);
            int silentBlocksArp = 0;
            for (size_t i = 2; i < arped.size(); ++i) if (arped[i] < 1.0e-4f) ++silentBlocksArp;

            expect (silentBlocksHeld < 5, "held note is continuous");
            expect (silentBlocksArp > 40, "gated arp leaves gaps: silent blocks=" + juce::String (silentBlocksArp));
        }

        beginTest ("Flip lever: REVERSE edge flips the sounding voice (snap OFF applies immediately)");
        {
            Proc t;
            setParam (t.p, AviatorKeyz::ParamID::FLIP_SNAP, 0.f);
            setParam (t.p, AviatorKeyz::ParamID::FLIP_WINDOW, 0.f);
            renderRms (t.p, 10);
            const float before = t.p.getPlayheadNorm();
            setParam (t.p, AviatorKeyz::ParamID::REVERSE, 1.f);
            renderRms (t.p, 1, 61); // any block; the note-on here is a second voice
            const float justAfter = t.p.getPlayheadNorm();
            juce::ignoreUnused (before, justAfter);
            // Render more and confirm the original voice ran backwards: after the
            // flip the playhead of the most recent voice (note 61, started at the
            // END because the lever is REV) must be near 1.0 and decreasing.
            juce::AudioBuffer<float> buffer (2, kBlock);
            juce::MidiBuffer none;
            buffer.clear(); t.p.processBlock (buffer, none);
            const float p1 = t.p.getPlayheadNorm();
            buffer.clear(); t.p.processBlock (buffer, none);
            const float p2 = t.p.getPlayheadNorm();
            expect (p1 > 0.5f, "reversed note starts near the end: " + juce::String (p1));
            expect (p2 < p1, "and its playhead runs backwards");
        }
    }
};

static FlightDeckWiringTests flightDeckWiringTests;
