#pragma once

#include <juce_core/juce_core.h>
#include "State/StateSchema.h"

/** Canonical list of every APVTS ParamID string — shared by schema and wiring tests. */
inline juce::StringArray allSchemaParamIDs()
{
    using namespace AviatorKeyz;
    return {
        ParamID::INPUT_GAIN,
        ParamID::OUTPUT_GAIN,
        ParamID::REVERSE,
        ParamID::GLIDE_TIME,
        ParamID::SMEAR,
        ParamID::TONE,
        ParamID::REVERB_AMOUNT,
        ParamID::REVERB_SIZE,
        ParamID::STEREO_WIDTH,
        ParamID::ENV_ATTACK,
        ParamID::ENV_RELEASE,
        ParamID::PAN,

        ParamID::LFO1_RATE,   ParamID::LFO1_DEPTH, ParamID::LFO1_SHAPE, ParamID::LFO1_SYNC, ParamID::LFO1_PHASE,
        ParamID::LFO2_RATE,   ParamID::LFO2_DEPTH, ParamID::LFO2_SHAPE, ParamID::LFO2_SYNC, ParamID::LFO2_PHASE,
        ParamID::LFO3_RATE,   ParamID::LFO3_DEPTH, ParamID::LFO3_SHAPE, ParamID::LFO3_SYNC, ParamID::LFO3_PHASE,

        ParamID::FX_DELAY_ON, ParamID::FX_DELAY_TIME, ParamID::FX_DELAY_FEEDBACK,
        ParamID::FX_DELAY_MIX, ParamID::FX_DELAY_SYNC,
        ParamID::FX_CHORUS_ON, ParamID::FX_CHORUS_RATE, ParamID::FX_CHORUS_DEPTH, ParamID::FX_CHORUS_MIX,
        ParamID::FX_LOFI_ON, ParamID::FX_LOFI_AMOUNT,
        ParamID::FX_DIST_ON, ParamID::FX_DIST_DRIVE,

        ParamID::MOD0_ON, ParamID::MOD0_SOURCE, ParamID::MOD0_DEST, ParamID::MOD0_AMOUNT,
        ParamID::MOD1_ON, ParamID::MOD1_SOURCE, ParamID::MOD1_DEST, ParamID::MOD1_AMOUNT,
        ParamID::MOD2_ON, ParamID::MOD2_SOURCE, ParamID::MOD2_DEST, ParamID::MOD2_AMOUNT,
        ParamID::MOD3_ON, ParamID::MOD3_SOURCE, ParamID::MOD3_DEST, ParamID::MOD3_AMOUNT,
        ParamID::MOD4_ON, ParamID::MOD4_SOURCE, ParamID::MOD4_DEST, ParamID::MOD4_AMOUNT,
        ParamID::MOD5_ON, ParamID::MOD5_SOURCE, ParamID::MOD5_DEST, ParamID::MOD5_AMOUNT,
        ParamID::MOD6_ON, ParamID::MOD6_SOURCE, ParamID::MOD6_DEST, ParamID::MOD6_AMOUNT,
        ParamID::MOD7_ON, ParamID::MOD7_SOURCE, ParamID::MOD7_DEST, ParamID::MOD7_AMOUNT,

        ParamID::OSC1_TYPE, ParamID::OSC1_TUNE, ParamID::OSC1_FINE,
        ParamID::OSC1_SHAPE, ParamID::OSC1_LEVEL, ParamID::OSC1_PAN,
        ParamID::OSC2_TYPE, ParamID::OSC2_TUNE, ParamID::OSC2_FINE,
        ParamID::OSC2_SHAPE, ParamID::OSC2_LEVEL, ParamID::OSC2_PAN,
        ParamID::SOURCE_BLEND,

        ParamID::FILTER_ENABLED,
        ParamID::FILTER_CUTOFF, ParamID::FILTER_RESONANCE,
        ParamID::FILTER_TYPE, ParamID::FILTER_DRIVE,
        ParamID::ENV_AMP_DECAY, ParamID::ENV_AMP_SUSTAIN,
        ParamID::ENV_FLT_ATTACK, ParamID::ENV_FLT_DECAY,
        ParamID::ENV_FLT_SUSTAIN, ParamID::ENV_FLT_RELEASE, ParamID::ENV_FLT_AMOUNT,
        ParamID::VELOCITY_SENSITIVITY,
        ParamID::VOICE_POLYPHONY, ParamID::VOICE_GLIDE_MODE, ParamID::VOICE_PLAY_MODE,
        ParamID::OUTPUT_LIMITER,

        ParamID::TEX_ENABLED, ParamID::TEX_AMOUNT, ParamID::TEX_WIDTH,
        ParamID::TEX_FREEZE, ParamID::TEX_GRAIN_SCAN, ParamID::TEX_GRAIN_RATE,
        ParamID::TEX_GRAIN_SIZE, ParamID::TEX_GRAIN_PITCH, ParamID::TEX_GRAIN_DENSITY,
        ParamID::TEX_GRAIN_SPREAD, ParamID::TEX_GRAIN_PAN,
        ParamID::TEX_MOTION, ParamID::TEX_DRIFT, ParamID::TEX_AIR, ParamID::TEX_REVERSE,

        ParamID::PHRASE_ENABLED, ParamID::PHRASE_TEMPO_SYNC,
        ParamID::PHRASE_KEY_SYNC, ParamID::PHRASE_TRIGGER_MODE,
        ParamID::PHRASE_LOOP, ParamID::PHRASE_START, ParamID::PHRASE_LENGTH,
        ParamID::PHRASE_PITCH,

        ParamID::FX_REVERB_DAMP, ParamID::FX_REVERB_ON, ParamID::FX_EDITS_ON,

        ParamID::PERF_MACRO_1, ParamID::PERF_MACRO_2,
        ParamID::PERF_MACRO_3, ParamID::PERF_MACRO_4,

        ParamID::SRC_START, ParamID::SRC_END, ParamID::SRC_TUNE, ParamID::SRC_SPEED,
        ParamID::SRC_REVERSE, ParamID::SRC_LOOP_MODE, ParamID::SRC_BPM_SYNC,
        ParamID::SRC_ORIGINAL_BPM, ParamID::SRC_ROOT_NOTE,
        ParamID::SRC_PLAYBACK_MODE, ParamID::SRC_KEYTRACK,

        ParamID::CHOP_ON, ParamID::CHOP_AMOUNT, ParamID::CHOP_RATE, ParamID::CHOP_GATE,
        ParamID::CHOP_SWING, ParamID::CHOP_RANDOM, ParamID::CHOP_REVERSE_CHANCE, ParamID::CHOP_SMOOTH,

        ParamID::PTEX_ON, ParamID::PTEX_FREEZE, ParamID::PTEX_GRAIN_SIZE, ParamID::PTEX_DENSITY,
        ParamID::PTEX_POSITION, ParamID::PTEX_PITCH_SPREAD, ParamID::PTEX_SMEAR,
        ParamID::PTEX_WIDTH, ParamID::PTEX_MIX,

        ParamID::PERF_MODE,
        ParamID::PERF_FX_STUTTER, ParamID::PERF_FX_REVERSE, ParamID::PERF_FX_HALF_TIME,
        ParamID::PERF_FX_FREEZE, ParamID::PERF_FX_TAPE_STOP, ParamID::PERF_FX_SCATTER,
        ParamID::PERF_FX_PITCH_DROP, ParamID::PERF_FX_FILTER_SWEEP,
    };
}

inline juce::StringArray allPerformanceParamIDs()
{
    using namespace AviatorKeyz;
    juce::StringArray ids = allSchemaParamIDs();
    for (int step = 0; step < ParamID::CHOP_STEP_COUNT; ++step)
    {
        ids.add (ParamID::chopStepParamId (step, "on"));
        ids.add (ParamID::chopStepParamId (step, "vol"));
        ids.add (ParamID::chopStepParamId (step, "offset"));
        ids.add (ParamID::chopStepParamId (step, "rev"));
        ids.add (ParamID::chopStepParamId (step, "pitch"));
    }
    return ids;
}

inline constexpr int kExpectedSchemaParamCount = 249;
