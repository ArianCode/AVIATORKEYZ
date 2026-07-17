#include "PerformanceParameterLayout.h"
#include "../../State/StateSchema.h"

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
}
