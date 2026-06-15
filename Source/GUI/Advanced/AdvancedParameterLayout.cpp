#include "AdvancedParameterLayout.h"
#include "../../DSP/ModMatrix.h"
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

void addOsc (std::vector<std::unique_ptr<RangedAudioParameter>>& params,
             const char* prefix,
             int typeDefault)
{
    const StringArray types { "Saw", "Square", "Triangle", "Sine", "Noise", "Wavetable", "FM", "Chord" };
    params.push_back (std::make_unique<APFC> (ParameterID { String (prefix) + "_type", 1 }, "Osc Type", types, typeDefault));
    params.push_back (std::make_unique<APFI> (ParameterID { String (prefix) + "_tune", 1 }, "Osc Tune", -24, 24, 0));
    params.push_back (std::make_unique<APF> (ParameterID { String (prefix) + "_fine", 1 }, "Osc Fine", NR (-100.f, 100.f, 0.1f), 0.f));
    params.push_back (std::make_unique<APF> (ParameterID { String (prefix) + "_shape", 1 }, "Osc Shape", NR (0.f, 1.f, 0.001f), 0.3f));
    params.push_back (std::make_unique<APF> (ParameterID { String (prefix) + "_level", 1 }, "Osc Level", NR (0.f, 1.f, 0.001f), 0.7f));
    params.push_back (std::make_unique<APF> (ParameterID { String (prefix) + "_pan", 1 }, "Osc Pan", NR (-1.f, 1.f, 0.001f), 0.f));
}

void addModRow (std::vector<std::unique_ptr<RangedAudioParameter>>& params,
                const char* onId, const char* srcId, const char* dstId, const char* amtId,
                bool onDef, int srcDef, int dstDef, float amtDef,
                const StringArray& sources, const StringArray& dests)
{
    params.push_back (std::make_unique<APB> (ParameterID { onId, 1 }, "Mod On", onDef));
    params.push_back (std::make_unique<APFC> (ParameterID { srcId, 1 }, "Mod Source", sources, srcDef));
    params.push_back (std::make_unique<APFC> (ParameterID { dstId, 1 }, "Mod Dest", dests, dstDef));
    params.push_back (std::make_unique<APF> (ParameterID { amtId, 1 }, "Mod Amount", NR (0.f, 1.f, 0.001f), amtDef));
}
} // namespace

void AdvancedParameterLayout::appendParameters (std::vector<std::unique_ptr<RangedAudioParameter>>& params)
{
    const StringArray lfoShapes { "Sine", "Square", "Triangle", "Ramp Up", "Ramp Down", "Random" };

    auto addLfo = [&] (const char* rateId, const char* depthId, const char* shapeId,
                       const char* syncId, const char* phaseId,
                       float defRate, float defDepth)
    {
        params.push_back (std::make_unique<APF> (ParameterID { rateId, 1 }, "LFO Rate", NR (0.01f, 20.f, 0.001f, 0.4f), defRate));
        params.push_back (std::make_unique<APF> (ParameterID { depthId, 1 }, "LFO Depth", NR (0.f, 1.f, 0.001f), defDepth));
        params.push_back (std::make_unique<APFC> (ParameterID { shapeId, 1 }, "LFO Shape", lfoShapes, 0));
        params.push_back (std::make_unique<APB> (ParameterID { syncId, 1 }, "LFO Sync", false));
        params.push_back (std::make_unique<APF> (ParameterID { phaseId, 1 }, "LFO Phase", NR (0.f, 1.f, 0.001f), 0.f));
    };

    addLfo (ParamID::LFO1_RATE, ParamID::LFO1_DEPTH, ParamID::LFO1_SHAPE, ParamID::LFO1_SYNC, ParamID::LFO1_PHASE, 0.5f, 0.5f);
    addLfo (ParamID::LFO2_RATE, ParamID::LFO2_DEPTH, ParamID::LFO2_SHAPE, ParamID::LFO2_SYNC, ParamID::LFO2_PHASE, 0.2f, 0.4f);
    addLfo (ParamID::LFO3_RATE, ParamID::LFO3_DEPTH, ParamID::LFO3_SHAPE, ParamID::LFO3_SYNC, ParamID::LFO3_PHASE, 1.0f, 0.3f);

    addOsc (params, "osc1", 0);
    addOsc (params, "osc2", 1);
    params.push_back (std::make_unique<APF> (ParameterID { ParamID::SOURCE_BLEND, 1 }, "Source Blend", NR (0.f, 1.f, 0.001f), 0.f));

    params.push_back (std::make_unique<APB> (ParameterID { ParamID::FILTER_ENABLED, 1 }, "Filter On", false));
    params.push_back (std::make_unique<APF> (ParameterID { ParamID::FILTER_CUTOFF, 1 }, "Filter Cutoff", NR (20.f, 20000.f, 0.1f, 0.3f), 8000.f));
    params.push_back (std::make_unique<APF> (ParameterID { ParamID::FILTER_RESONANCE, 1 }, "Filter Reso", NR (0.f, 1.f, 0.001f), 0.25f));
    params.push_back (std::make_unique<APFC> (ParameterID { ParamID::FILTER_TYPE, 1 }, "Filter Type", StringArray { "LP", "HP", "BP", "Notch" }, 0));
    params.push_back (std::make_unique<APF> (ParameterID { ParamID::FILTER_DRIVE, 1 }, "Filter Drive", NR (0.f, 1.f, 0.001f), 0.f));

    params.push_back (std::make_unique<APF> (ParameterID { ParamID::ENV_AMP_DECAY, 1 }, "Amp Decay", NR (0.f, 10.f, 0.001f, 0.4f), 0.f));
    params.push_back (std::make_unique<APF> (ParameterID { ParamID::ENV_AMP_SUSTAIN, 1 }, "Amp Sustain", NR (0.f, 1.f, 0.001f), 1.f));
    params.push_back (std::make_unique<APF> (ParameterID { ParamID::ENV_FLT_ATTACK, 1 }, "Flt Attack", NR (0.001f, 10.f, 0.001f, 0.4f), 0.01f));
    params.push_back (std::make_unique<APF> (ParameterID { ParamID::ENV_FLT_DECAY, 1 }, "Flt Decay", NR (0.001f, 10.f, 0.001f, 0.4f), 0.3f));
    params.push_back (std::make_unique<APF> (ParameterID { ParamID::ENV_FLT_SUSTAIN, 1 }, "Flt Sustain", NR (0.f, 1.f, 0.001f), 0.6f));
    params.push_back (std::make_unique<APF> (ParameterID { ParamID::ENV_FLT_RELEASE, 1 }, "Flt Release", NR (0.001f, 30.f, 0.001f, 0.4f), 0.3f));
    params.push_back (std::make_unique<APF> (ParameterID { ParamID::ENV_FLT_AMOUNT, 1 }, "Flt Env Amt", NR (-1.f, 1.f, 0.001f), 0.f));

    params.push_back (std::make_unique<APF> (ParameterID { ParamID::VELOCITY_SENSITIVITY, 1 }, "Velocity Sens", NR (0.f, 1.f, 0.001f), 0.f));
    params.push_back (std::make_unique<APFI> (ParameterID { ParamID::VOICE_POLYPHONY, 1 }, "Polyphony", 1, 16, 8));
    params.push_back (std::make_unique<APFC> (ParameterID { ParamID::VOICE_GLIDE_MODE, 1 }, "Glide Mode", StringArray { "Off", "Legato", "Always" }, 0));
    params.push_back (std::make_unique<APFC> (ParameterID { ParamID::VOICE_PLAY_MODE, 1 }, "Play Mode", StringArray { "Poly", "Mono", "Legato" }, 0));
    params.push_back (std::make_unique<APB> (ParameterID { ParamID::OUTPUT_LIMITER, 1 }, "Output Limiter", true));

    params.push_back (std::make_unique<APB> (ParameterID { ParamID::TEX_ENABLED, 1 }, "Texture On", false));
    params.push_back (std::make_unique<APF> (ParameterID { ParamID::TEX_AMOUNT, 1 }, "Texture Amount", NR (0.f, 1.f, 0.001f), 0.f));
    params.push_back (std::make_unique<APF> (ParameterID { ParamID::TEX_WIDTH, 1 }, "Texture Width", NR (0.f, 1.f, 0.001f), 0.8f));
    params.push_back (std::make_unique<APB> (ParameterID { ParamID::TEX_FREEZE, 1 }, "Tex Freeze", false));
    params.push_back (std::make_unique<APF> (ParameterID { ParamID::TEX_GRAIN_SCAN, 1 }, "Grain Scan", NR (0.f, 1.f, 0.001f), 0.3f));
    params.push_back (std::make_unique<APF> (ParameterID { ParamID::TEX_GRAIN_RATE, 1 }, "Grain Rate", NR (0.f, 1.f, 0.001f), 0.55f));
    params.push_back (std::make_unique<APF> (ParameterID { ParamID::TEX_GRAIN_SIZE, 1 }, "Grain Size", NR (0.f, 1.f, 0.001f), 0.4f));
    params.push_back (std::make_unique<APFI> (ParameterID { ParamID::TEX_GRAIN_PITCH, 1 }, "Grain Pitch", -24, 24, 0));
    params.push_back (std::make_unique<APF> (ParameterID { ParamID::TEX_GRAIN_DENSITY, 1 }, "Grain Density", NR (0.f, 1.f, 0.001f), 0.7f));
    params.push_back (std::make_unique<APF> (ParameterID { ParamID::TEX_GRAIN_SPREAD, 1 }, "Grain Spread", NR (0.f, 1.f, 0.001f), 0.55f));
    params.push_back (std::make_unique<APF> (ParameterID { ParamID::TEX_GRAIN_PAN, 1 }, "Grain Pan", NR (-1.f, 1.f, 0.001f), 0.f));
    params.push_back (std::make_unique<APF> (ParameterID { ParamID::TEX_MOTION, 1 }, "Tex Motion", NR (0.f, 1.f, 0.001f), 0.35f));
    params.push_back (std::make_unique<APF> (ParameterID { ParamID::TEX_DRIFT, 1 }, "Tex Drift", NR (0.f, 1.f, 0.001f), 0.35f));
    params.push_back (std::make_unique<APF> (ParameterID { ParamID::TEX_AIR, 1 }, "Tex Air", NR (0.f, 1.f, 0.001f), 0.f));
    params.push_back (std::make_unique<APB> (ParameterID { ParamID::TEX_REVERSE, 1 }, "Grain Reverse", false));

    params.push_back (std::make_unique<APB> (ParameterID { ParamID::PHRASE_ENABLED, 1 }, "Phrase On", false));
    params.push_back (std::make_unique<APB> (ParameterID { ParamID::PHRASE_TEMPO_SYNC, 1 }, "Phrase Tempo Sync", true));
    params.push_back (std::make_unique<APB> (ParameterID { ParamID::PHRASE_KEY_SYNC, 1 }, "Phrase Key Sync", true));
    params.push_back (std::make_unique<APFC> (ParameterID { ParamID::PHRASE_TRIGGER_MODE, 1 }, "Phrase Trigger", StringArray { "Hold", "Trigger", "Gate" }, 0));
    params.push_back (std::make_unique<APB> (ParameterID { ParamID::PHRASE_LOOP, 1 }, "Phrase Loop", false));
    params.push_back (std::make_unique<APF> (ParameterID { ParamID::PHRASE_START, 1 }, "Phrase Start", NR (0.f, 1.f, 0.001f), 0.f));
    params.push_back (std::make_unique<APF> (ParameterID { ParamID::PHRASE_LENGTH, 1 }, "Phrase Length", NR (0.f, 1.f, 0.001f), 1.f));
    params.push_back (std::make_unique<APFI> (ParameterID { ParamID::PHRASE_PITCH, 1 }, "Phrase Pitch", -24, 24, 0));

    params.push_back (std::make_unique<APB> (ParameterID { ParamID::FX_REVERB_ON, 1 }, "Reverb On", false));
    params.push_back (std::make_unique<APF> (ParameterID { ParamID::FX_REVERB_DAMP, 1 }, "Reverb Damp", NR (0.f, 1.f, 0.001f), 0.4f));
    params.push_back (std::make_unique<APB> (ParameterID { ParamID::FX_EDITS_ON, 1 }, "Preset FX Edits", true));

    params.push_back (std::make_unique<APB> (ParameterID { ParamID::FX_DELAY_ON, 1 }, "Delay On", false));
    params.push_back (std::make_unique<APF> (ParameterID { ParamID::FX_DELAY_TIME, 1 }, "Delay Time", NR (0.01f, 2.f, 0.001f), 0.4f));
    params.push_back (std::make_unique<APF> (ParameterID { ParamID::FX_DELAY_FEEDBACK, 1 }, "Delay Feedback", NR (0.f, 1.f, 0.001f), 0.35f));
    params.push_back (std::make_unique<APF> (ParameterID { ParamID::FX_DELAY_MIX, 1 }, "Delay Mix", NR (0.f, 1.f, 0.001f), 0.28f));
    params.push_back (std::make_unique<APB> (ParameterID { ParamID::FX_DELAY_SYNC, 1 }, "Delay Sync", true));
    params.push_back (std::make_unique<APB> (ParameterID { ParamID::FX_CHORUS_ON, 1 }, "Chorus On", false));
    params.push_back (std::make_unique<APF> (ParameterID { ParamID::FX_CHORUS_RATE, 1 }, "Chorus Rate", NR (0.f, 1.f, 0.001f), 0.3f));
    params.push_back (std::make_unique<APF> (ParameterID { ParamID::FX_CHORUS_DEPTH, 1 }, "Chorus Depth", NR (0.f, 1.f, 0.001f), 0.25f));
    params.push_back (std::make_unique<APF> (ParameterID { ParamID::FX_CHORUS_MIX, 1 }, "Chorus Mix", NR (0.f, 1.f, 0.001f), 0.3f));
    params.push_back (std::make_unique<APB> (ParameterID { ParamID::FX_LOFI_ON, 1 }, "Lo-Fi On", false));
    params.push_back (std::make_unique<APF> (ParameterID { ParamID::FX_LOFI_AMOUNT, 1 }, "Lo-Fi Amount", NR (0.f, 1.f, 0.001f), 0.1f));
    params.push_back (std::make_unique<APB> (ParameterID { ParamID::FX_DIST_ON, 1 }, "Dist On", false));
    params.push_back (std::make_unique<APF> (ParameterID { ParamID::FX_DIST_DRIVE, 1 }, "Dist Drive", NR (0.f, 1.f, 0.001f), 0.2f));

    params.push_back (std::make_unique<APF> (ParameterID { ParamID::PERF_MACRO_1, 1 }, "Macro 1", NR (0.f, 1.f, 0.001f), 0.5f));
    params.push_back (std::make_unique<APF> (ParameterID { ParamID::PERF_MACRO_2, 1 }, "Macro 2", NR (0.f, 1.f, 0.001f), 0.5f));
    params.push_back (std::make_unique<APF> (ParameterID { ParamID::PERF_MACRO_3, 1 }, "Macro 3", NR (0.f, 1.f, 0.001f), 0.5f));
    params.push_back (std::make_unique<APF> (ParameterID { ParamID::PERF_MACRO_4, 1 }, "Macro 4", NR (0.f, 1.f, 0.001f), 0.5f));

    const auto modSources = ModMatrix::sourceNames();
    const auto modDests   = ModMatrix::destNames();

    addModRow (params, ParamID::MOD0_ON, ParamID::MOD0_SOURCE, ParamID::MOD0_DEST, ParamID::MOD0_AMOUNT,
               false, 1, static_cast<int> (ModDest::smear), 0.35f, modSources, modDests);
    addModRow (params, ParamID::MOD1_ON, ParamID::MOD1_SOURCE, ParamID::MOD1_DEST, ParamID::MOD1_AMOUNT,
               false, 2, static_cast<int> (ModDest::filterCutoff), 0.28f, modSources, modDests);
    addModRow (params, ParamID::MOD2_ON, ParamID::MOD2_SOURCE, ParamID::MOD2_DEST, ParamID::MOD2_AMOUNT,
               false, 3, static_cast<int> (ModDest::reverbAmount), 0.22f, modSources, modDests);
    addModRow (params, ParamID::MOD3_ON, ParamID::MOD3_SOURCE, ParamID::MOD3_DEST, ParamID::MOD3_AMOUNT,
               false, 0, 0, 0.f, modSources, modDests);
    addModRow (params, ParamID::MOD4_ON, ParamID::MOD4_SOURCE, ParamID::MOD4_DEST, ParamID::MOD4_AMOUNT,
               false, 0, 0, 0.f, modSources, modDests);
    addModRow (params, ParamID::MOD5_ON, ParamID::MOD5_SOURCE, ParamID::MOD5_DEST, ParamID::MOD5_AMOUNT,
               false, 0, 0, 0.f, modSources, modDests);
    addModRow (params, ParamID::MOD6_ON, ParamID::MOD6_SOURCE, ParamID::MOD6_DEST, ParamID::MOD6_AMOUNT,
               false, 0, 0, 0.f, modSources, modDests);
    addModRow (params, ParamID::MOD7_ON, ParamID::MOD7_SOURCE, ParamID::MOD7_DEST, ParamID::MOD7_AMOUNT,
               false, 0, 0, 0.f, modSources, modDests);
}
