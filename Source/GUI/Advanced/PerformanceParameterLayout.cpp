#include "PerformanceParameterLayout.h"
#include "../../State/StateSchema.h"
#include "../../DSP/Mfx/MfxDescriptors.h"

using namespace juce;
using namespace AviatorKeyz;

namespace
{
using APF  = AudioParameterFloat;
using APB  = AudioParameterBool;
using APFC = AudioParameterChoice;
using APFI = AudioParameterInt;
using NR   = NormalisableRange<float>;
} // namespace

void PerformanceParameterLayout::appendParameters (std::vector<std::unique_ptr<RangedAudioParameter>>& params)
{
    params.push_back (std::make_unique<APF> (ParameterID { ParamID::SRC_START, 1 }, "Src Start",
                                             NR (0.f, 1.f, 0.001f), 0.f));
    params.push_back (std::make_unique<APF> (ParameterID { ParamID::SRC_END, 1 }, "Src End",
                                             NR (0.f, 1.f, 0.001f), 1.f));
    params.push_back (std::make_unique<APF> (ParameterID { ParamID::SRC_TUNE, 1 }, "Src Tune",
                                             NR (-24.f, 24.f, 0.01f), 0.f));
    params.push_back (std::make_unique<APF> (ParameterID { ParamID::SRC_SPEED, 1 }, "Src Speed",
                                             NR (0.25f, 4.f, 0.001f, 0.4f), 1.f));
    params.push_back (std::make_unique<APB> (ParameterID { ParamID::SRC_REVERSE, 1 }, "Src Reverse", false));
    params.push_back (std::make_unique<APFC> (ParameterID { ParamID::SRC_LOOP_MODE, 1 }, "Loop Mode",
                                              StringArray { "One Shot", "Loop", "Gate" }, 2));
    params.push_back (std::make_unique<APB> (ParameterID { ParamID::SRC_BPM_SYNC, 1 }, "Src BPM Sync", true));
    params.push_back (std::make_unique<APF> (ParameterID { ParamID::SRC_ORIGINAL_BPM, 1 }, "Original BPM",
                                             NR (40.f, 240.f, 0.1f), 120.f));
    params.push_back (std::make_unique<APFI> (ParameterID { ParamID::SRC_ROOT_NOTE, 1 }, "Root Note", 0, 127, 60));
    params.push_back (std::make_unique<APFC> (ParameterID { ParamID::SRC_PLAYBACK_MODE, 1 }, "Playback Mode",
        StringArray { "One Shot", "Phrase", "Chromatic", "Time Stretch", "Slice" }, 2));
    params.push_back (std::make_unique<APB> (ParameterID { ParamID::SRC_KEYTRACK, 1 }, "Keytrack", false));

    params.push_back (std::make_unique<APB> (ParameterID { ParamID::CHOP_ON, 1 }, "Chop On", false));
    params.push_back (std::make_unique<APF> (ParameterID { ParamID::CHOP_AMOUNT, 1 }, "Chop Amount",
                                             NR (0.f, 1.f, 0.001f), 0.f));
    params.push_back (std::make_unique<APFC> (ParameterID { ParamID::CHOP_RATE, 1 }, "Chop Rate",
                                              StringArray { "1/4", "1/8", "1/16", "1/32" }, 2));
    params.push_back (std::make_unique<APF> (ParameterID { ParamID::CHOP_GATE, 1 }, "Chop Gate",
                                             NR (0.f, 1.f, 0.001f), 0.5f));
    params.push_back (std::make_unique<APF> (ParameterID { ParamID::CHOP_SWING, 1 }, "Chop Swing",
                                             NR (0.f, 1.f, 0.001f), 0.f));
    params.push_back (std::make_unique<APF> (ParameterID { ParamID::CHOP_RANDOM, 1 }, "Chop Random",
                                             NR (0.f, 1.f, 0.001f), 0.f));
    params.push_back (std::make_unique<APF> (ParameterID { ParamID::CHOP_REVERSE_CHANCE, 1 }, "Reverse Chance",
                                             NR (0.f, 1.f, 0.001f), 0.f));
    params.push_back (std::make_unique<APF> (ParameterID { ParamID::CHOP_SMOOTH, 1 }, "Chop Smooth",
                                             NR (0.001f, 0.1f, 0.001f), 0.01f));

    for (int step = 0; step < ParamID::CHOP_STEP_COUNT; ++step)
    {
        const auto onId = ParamID::chopStepParamId (step, "on").toStdString();
        const auto volId = ParamID::chopStepParamId (step, "vol").toStdString();
        const auto offId = ParamID::chopStepParamId (step, "offset").toStdString();
        const auto revId = ParamID::chopStepParamId (step, "rev").toStdString();
        const auto pitId = ParamID::chopStepParamId (step, "pitch").toStdString();

        params.push_back (std::make_unique<APB> (ParameterID { onId, 1 },
                                                 "Step " + String (step + 1) + " On", true));
        params.push_back (std::make_unique<APF> (ParameterID { volId, 1 },
                                                 "Step " + String (step + 1) + " Vol",
                                                 NR (0.f, 1.f, 0.001f), 1.f));
        params.push_back (std::make_unique<APF> (ParameterID { offId, 1 },
                                                 "Step " + String (step + 1) + " Offset",
                                                 NR (-1.f, 1.f, 0.001f), 0.f));
        params.push_back (std::make_unique<APB> (ParameterID { revId, 1 },
                                                 "Step " + String (step + 1) + " Rev", false));
        params.push_back (std::make_unique<APFI> (ParameterID { pitId, 1 },
                                                 "Step " + String (step + 1) + " Pitch", -12, 12, 0));
    }

    params.push_back (std::make_unique<APB> (ParameterID { ParamID::PTEX_ON, 1 }, "Texture On", false));
    params.push_back (std::make_unique<APB> (ParameterID { ParamID::PTEX_FREEZE, 1 }, "Tex Freeze", false));
    params.push_back (std::make_unique<APF> (ParameterID { ParamID::PTEX_GRAIN_SIZE, 1 }, "Grain Size",
                                             NR (0.f, 1.f, 0.001f), 0.5f));
    params.push_back (std::make_unique<APF> (ParameterID { ParamID::PTEX_DENSITY, 1 }, "Density",
                                             NR (0.f, 1.f, 0.001f), 0.5f));
    params.push_back (std::make_unique<APF> (ParameterID { ParamID::PTEX_POSITION, 1 }, "Position",
                                             NR (0.f, 1.f, 0.001f), 0.f));
    params.push_back (std::make_unique<APF> (ParameterID { ParamID::PTEX_PITCH_SPREAD, 1 }, "Pitch Spread",
                                             NR (0.f, 1.f, 0.001f), 0.f));
    params.push_back (std::make_unique<APF> (ParameterID { ParamID::PTEX_SMEAR, 1 }, "Tex Smear",
                                             NR (0.f, 1.f, 0.001f), 0.f));
    params.push_back (std::make_unique<APF> (ParameterID { ParamID::PTEX_WIDTH, 1 }, "Tex Width",
                                             NR (0.f, 1.f, 0.001f), 0.5f));
    params.push_back (std::make_unique<APF> (ParameterID { ParamID::PTEX_MIX, 1 }, "Texture Mix",
                                             NR (0.f, 1.f, 0.001f), 0.f));

    params.push_back (std::make_unique<APFC> (ParameterID { ParamID::PERF_MODE, 1 }, "Perf Mode",
        StringArray { "Normal", "Chop", "Gate", "Stutter", "Half Time", "Reverse", "Scatter", "Freeze" }, 0));
    params.push_back (std::make_unique<APB> (ParameterID { ParamID::PERF_FX_STUTTER, 1 }, "FX Stutter", false));
    params.push_back (std::make_unique<APB> (ParameterID { ParamID::PERF_FX_REVERSE, 1 }, "FX Reverse", false));
    params.push_back (std::make_unique<APB> (ParameterID { ParamID::PERF_FX_HALF_TIME, 1 }, "FX Half Time", false));
    params.push_back (std::make_unique<APB> (ParameterID { ParamID::PERF_FX_FREEZE, 1 }, "FX Freeze", false));
    params.push_back (std::make_unique<APB> (ParameterID { ParamID::PERF_FX_TAPE_STOP, 1 }, "FX Tape Stop", false));
    params.push_back (std::make_unique<APB> (ParameterID { ParamID::PERF_FX_SCATTER, 1 }, "FX Scatter", false));
    params.push_back (std::make_unique<APB> (ParameterID { ParamID::PERF_FX_PITCH_DROP, 1 }, "FX Pitch Drop", false));
    params.push_back (std::make_unique<APB> (ParameterID { ParamID::PERF_FX_FILTER_SWEEP, 1 }, "FX Filter Sweep", false));

    // --- Flight Deck: arpeggiator + flip lever -----------------------------
    const auto percentText = [] (float v, int) { return String (roundToInt (v * 100.f)) + "%"; };

    params.push_back (std::make_unique<APB> (ParameterID { ParamID::ARP_ON, 1 }, "Arp On", false));
    params.push_back (std::make_unique<APFC> (ParameterID { ParamID::ARP_MODE, 1 }, "Arp Mode",
        StringArray { "Up", "Down", "Up-Down", "Random", "As Played" }, 0));
    params.push_back (std::make_unique<APFC> (ParameterID { ParamID::ARP_RATE, 1 }, "Arp Rate",
        StringArray { "1/4", "1/8", "1/16", "1/32" }, 2));
    params.push_back (std::make_unique<APFC> (ParameterID { ParamID::ARP_FEEL, 1 }, "Arp Feel",
        StringArray { "Straight", "Triplet", "Dotted" }, 0));
    params.push_back (std::make_unique<APFI> (ParameterID { ParamID::ARP_OCTAVES, 1 }, "Arp Octaves", 1, 4, 1));
    params.push_back (std::make_unique<APF> (ParameterID { ParamID::ARP_GATE, 1 }, "Arp Gate",
        NR (0.05f, 1.f, 0.001f), 0.7f, AudioParameterFloatAttributes().withStringFromValueFunction (percentText)));
    params.push_back (std::make_unique<APF> (ParameterID { ParamID::ARP_SWING, 1 }, "Arp Swing",
        NR (0.f, 1.f, 0.001f), 0.f, AudioParameterFloatAttributes().withStringFromValueFunction (percentText)));
    params.push_back (std::make_unique<APF> (ParameterID { ParamID::ARP_HUMANIZE, 1 }, "Arp Humanize",
        NR (0.f, 1.f, 0.001f), 0.f, AudioParameterFloatAttributes().withStringFromValueFunction (percentText)));
    params.push_back (std::make_unique<APF> (ParameterID { ParamID::ARP_OCT_SPREAD, 1 }, "Arp Octave Spread",
        NR (0.f, 1.f, 0.001f), 0.f, AudioParameterFloatAttributes().withStringFromValueFunction (percentText)));
    params.push_back (std::make_unique<APB> (ParameterID { ParamID::ARP_HOLD, 1 }, "Arp Hold", false));
    params.push_back (std::make_unique<APFC> (ParameterID { ParamID::ARP_TARGET, 1 }, "Arp Target",
        StringArray { "Slices", "Notes" }, 1));

    params.push_back (std::make_unique<APFC> (ParameterID { ParamID::FLIP_WINDOW, 1 }, "Flip Window",
        StringArray { "Phrase", "Slice", "Beat" }, 0));
    params.push_back (std::make_unique<APFC> (ParameterID { ParamID::FLIP_SNAP, 1 }, "Flip Snap",
        StringArray { "Off", "1/4", "1/8", "1/16" }, 2));
    params.push_back (std::make_unique<APFC> (ParameterID { ParamID::FLIP_MODE, 1 }, "Flip Mode",
        StringArray { "Latch", "Momentary" }, 0));
    params.push_back (std::make_unique<APB> (ParameterID { ParamID::FLIP_TRIGGER_ON, 1 }, "Flip Trigger On", false));
    params.push_back (std::make_unique<APFI> (ParameterID { ParamID::FLIP_TRIGGER_NOTE, 1 }, "Flip Trigger Note", 0, 127, 24));
    params.push_back (std::make_unique<APB> (ParameterID { ParamID::ENV_ENABLED, 1 }, "Envelope On", true));

    // --- MFX rack: slot A / slot B ------------------------------------------
    const auto effectNames = Mfx::effectNames();
    const auto modSources = Mfx::modSourceNames();
    for (int slot = 0; slot < Mfx::kNumSlots; ++slot)
    {
        const String tag = slot == 0 ? "MFX A " : "MFX B ";
        // Slot B starts as the Grain Cloud (what the ATMOSPHERE zone used to be).
        const int defaultEffect = slot == 1 ? (int) Mfx::Effect::grainCloud : (int) Mfx::Effect::sweepFilter;

        params.push_back (std::make_unique<APB> (ParameterID { Mfx::onId (slot), 1 }, tag + "On", false));
        params.push_back (std::make_unique<APFC> (ParameterID { Mfx::effectId (slot), 1 }, tag + "Effect",
                                                  effectNames, defaultEffect));
        params.push_back (std::make_unique<APF> (ParameterID { Mfx::sendId (slot), 1 }, tag + "Rev Send",
                                                 NR (0.f, 1.f, 0.001f), 0.f,
                                                 AudioParameterFloatAttributes().withStringFromValueFunction (percentText)));
        params.push_back (std::make_unique<APF> (ParameterID { Mfx::levelId (slot), 1 }, tag + "Level",
                                                 NR (-24.f, 6.f, 0.1f), 0.f,
                                                 AudioParameterFloatAttributes().withLabel ("dB")));

        const auto defaults = Mfx::defaultsNormalised (static_cast<Mfx::Effect> (defaultEffect));
        for (int p = 0; p < Mfx::kParamsPerSlot; ++p)
        {
            params.push_back (std::make_unique<APF> (ParameterID { Mfx::paramId (slot, p), 1 },
                                                     tag + "Param " + String (p + 1),
                                                     NR (0.f, 1.f, 0.0001f), defaults[(size_t) p],
                                                     AudioParameterFloatAttributes().withStringFromValueFunction (percentText)));
        }
        for (int a = 0; a < Mfx::kNumAssigns; ++a)
        {
            params.push_back (std::make_unique<APFC> (ParameterID { Mfx::assignSourceId (slot, a), 1 },
                                                      tag + "Assign " + String (a + 1) + " Source", modSources, 0));
            params.push_back (std::make_unique<APF> (ParameterID { Mfx::assignAmountId (slot, a), 1 },
                                                     tag + "Assign " + String (a + 1) + " Amount",
                                                     NR (-1.f, 1.f, 0.001f), 0.f,
                                                     AudioParameterFloatAttributes().withStringFromValueFunction (percentText)));
        }
    }
}
