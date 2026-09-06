#pragma once

// =============================================================================
//  StateSchema.h — AviatorKeyz
//
//  CRITICAL: This file is the single source of truth for all parameter IDs,
//  state schema versions, and serialization keys.
//
//  Rules:
//    1. NEVER rename or remove an ID after it has been shipped.
//       Hosts store parameter IDs in saved projects. Changing them breaks recall.
//    2. When adding a parameter, append to the list — do not insert in the middle.
//    3. When the binary state format changes incompatibly, increment
//       STATE_SCHEMA_VERSION and add a migration branch in setStateInformation().
//    4. Document every ID with its purpose, range summary, and version introduced.
// =============================================================================

#include <juce_core/juce_core.h>

namespace AviatorKeyz {

// ---------------------------------------------------------------------------
// Content pipeline (v0 lock)
//
// Factory audio: mono/stereo WAV embedded via CMake juce_add_binary_data
//   (namespace AviatorKeyzBinary, see BinaryData.h). Default key map sample:
//   factory_default_wav — loaded once in prepareToPlay (message/audio prep),
//   never from processBlock.
// Factory presets: XML next to wav in Resources/, same binary_data target.
//   Presets wrap APVTS XML in <Preset category name schemaVersion> root.
// Presets reference sound via sampleId on the <Preset> root (see SampleID).
// Factory WAVs live in Resources/Factory/*.wav and embed via juce_add_binary_data.
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// State schema version
// Bump this integer whenever the serialized state format changes in a way that
// requires migration logic (e.g. a parameter is split, rescaled, or removed).
// Minor additions (new params with sensible defaults) do not require a bump.
// ---------------------------------------------------------------------------
static constexpr int STATE_SCHEMA_VERSION = 1;

// ---------------------------------------------------------------------------
// Preset browser / category keys (not APVTS params — stored in preset XML)
// ---------------------------------------------------------------------------
namespace PresetKey {
    static constexpr const char* CATEGORY    = "category";
    static constexpr const char* NAME        = "name";
    static constexpr const char* AUTHOR      = "author";
    static constexpr const char* SCHEMA_VER  = "schemaVersion";
    static constexpr const char* SAMPLE_ID   = "sampleId";
    /** MIDI note at which embedded sample plays at native pitch (default 60 = C4). */
    static constexpr const char* ROOT_NOTE   = "rootNote";
    static constexpr const char* SOUND_TYPE  = "soundType";
    /** Sample tempo used for host BPM sync (varispeed until a stretch engine exists). */
    static constexpr const char* ORIGINAL_BPM = "originalBpm";
}

// ---------------------------------------------------------------------------
// Factory sample IDs — referenced by preset XML sampleId attribute.
// v1.0+: one embedded WAV per factory preset (factory_<category>_<slug>).
// Legacy category IDs (factory_leads, etc.) remain as aliases for fallback lookup.
// ---------------------------------------------------------------------------
namespace SampleID {
    static constexpr const char* DEFAULT      = "factory_default";
    static constexpr const char* LEADS        = "factory_leads";
    static constexpr const char* BRASS        = "factory_brass";
    static constexpr const char* ENSEMBLES    = "factory_ensembles";
    static constexpr const char* STRINGS      = "factory_strings";
    static constexpr const char* PADS         = "factory_pads";
    static constexpr const char* CHORDS       = "factory_chords";
    static constexpr const char* SYNTHS       = "factory_synths";
    static constexpr const char* ARPS         = "factory_arps";
    static constexpr const char* VOCALS       = "factory_vocals";
    static constexpr const char* BELLS        = "factory_bells";
}

// ---------------------------------------------------------------------------
// Parameter IDs — maps to AudioProcessorValueTreeState parameter IDs.
//
// Format: lower_snake_case string literals.
// Version column = milestone version in which the parameter was introduced.
// ---------------------------------------------------------------------------
namespace ParamID {

    // --- Signal path ---
    // v1: input gain, -24 to +12 dB, default 0 dB
    static constexpr const char* INPUT_GAIN     = "input_gain";

    // v1: output gain, -24 to +12 dB, default 0 dB
    static constexpr const char* OUTPUT_GAIN    = "output_gain";

    // --- Creative Engine ---
    // v1: reverse mode, boolean (false = forward, true = reversed playback)
    static constexpr const char* REVERSE        = "reverse";

    // v1: glide/portamento time, 0–500 ms, default 0 (off), skewed curve
    static constexpr const char* GLIDE_TIME     = "glide_time";

    // v1: macro highpass filter, 0 (off) to 1 (aggressive low-cut), default 0
    // ID kept as smear for host/preset recall compatibility.
    static constexpr const char* SMEAR          = "smear";

    // v1: tone tilt, -1.0 (dark/warm) to +1.0 (bright/clean), default 0
    static constexpr const char* TONE           = "tone";

    // --- Space / Output section ---
    // v1: reverb wet amount, 0–1, default 0
    static constexpr const char* REVERB_AMOUNT  = "reverb_amount";

    // v1: reverb size/decay, 0–1, default 0.5
    static constexpr const char* REVERB_SIZE    = "reverb_size";

    // v1: overall tonal brightness, 0 (dark) to 1.0 (bright), default 0.5 (neutral).
    // ID kept as stereo_width for host/preset recall compatibility.
    static constexpr const char* STEREO_WIDTH   = "stereo_width";

    // v1: envelope attack time, 0.5–5000 ms, default ~5 ms
    static constexpr const char* ENV_ATTACK     = "env_attack";

    // v1: envelope release time, 5–10000 ms, default 150 ms
    static constexpr const char* ENV_RELEASE    = "env_release";

    // v1: output pan, -1.0 (L) to +1.0 (R), default 0
    static constexpr const char* PAN            = "pan";

    // --- Advanced tab (v2) — LFO, extended FX, mod matrix ---
    static constexpr const char* LFO1_RATE   = "lfo1_rate";
    static constexpr const char* LFO1_DEPTH  = "lfo1_depth";
    static constexpr const char* LFO1_SHAPE  = "lfo1_shape";
    static constexpr const char* LFO1_SYNC   = "lfo1_sync";

    static constexpr const char* LFO2_RATE   = "lfo2_rate";
    static constexpr const char* LFO2_DEPTH  = "lfo2_depth";
    static constexpr const char* LFO2_SHAPE  = "lfo2_shape";
    static constexpr const char* LFO2_SYNC   = "lfo2_sync";

    static constexpr const char* LFO3_RATE   = "lfo3_rate";
    static constexpr const char* LFO3_DEPTH  = "lfo3_depth";
    static constexpr const char* LFO3_SHAPE  = "lfo3_shape";
    static constexpr const char* LFO3_SYNC   = "lfo3_sync";

    static constexpr const char* FX_DELAY_ON        = "fx_delay_on";
    static constexpr const char* FX_DELAY_TIME      = "fx_delay_time";
    static constexpr const char* FX_DELAY_FEEDBACK  = "fx_delay_feedback";
    static constexpr const char* FX_DELAY_MIX        = "fx_delay_mix";
    static constexpr const char* FX_DELAY_SYNC      = "fx_delay_sync";

    static constexpr const char* FX_CHORUS_ON    = "fx_chorus_on";
    static constexpr const char* FX_CHORUS_RATE  = "fx_chorus_rate";
    static constexpr const char* FX_CHORUS_DEPTH = "fx_chorus_depth";
    static constexpr const char* FX_CHORUS_MIX   = "fx_chorus_mix";

    static constexpr const char* FX_LOFI_ON     = "fx_lofi_on";
    static constexpr const char* FX_LOFI_AMOUNT = "fx_lofi_amount";

    static constexpr const char* FX_DIST_ON    = "fx_dist_on";
    static constexpr const char* FX_DIST_DRIVE = "fx_dist_drive";

    // Mod matrix — 8 rows × (enable, source, dest, amount)
    static constexpr int MOD_MATRIX_ROWS = 8;

    static constexpr const char* MOD0_ON      = "mod_0_on";
    static constexpr const char* MOD0_SOURCE  = "mod_0_source";
    static constexpr const char* MOD0_DEST    = "mod_0_dest";
    static constexpr const char* MOD0_AMOUNT  = "mod_0_amount";
    static constexpr const char* MOD1_ON      = "mod_1_on";
    static constexpr const char* MOD1_SOURCE  = "mod_1_source";
    static constexpr const char* MOD1_DEST    = "mod_1_dest";
    static constexpr const char* MOD1_AMOUNT  = "mod_1_amount";
    static constexpr const char* MOD2_ON      = "mod_2_on";
    static constexpr const char* MOD2_SOURCE  = "mod_2_source";
    static constexpr const char* MOD2_DEST    = "mod_2_dest";
    static constexpr const char* MOD2_AMOUNT  = "mod_2_amount";
    static constexpr const char* MOD3_ON      = "mod_3_on";
    static constexpr const char* MOD3_SOURCE  = "mod_3_source";
    static constexpr const char* MOD3_DEST    = "mod_3_dest";
    static constexpr const char* MOD3_AMOUNT  = "mod_3_amount";
    static constexpr const char* MOD4_ON      = "mod_4_on";
    static constexpr const char* MOD4_SOURCE  = "mod_4_source";
    static constexpr const char* MOD4_DEST    = "mod_4_dest";
    static constexpr const char* MOD4_AMOUNT  = "mod_4_amount";
    static constexpr const char* MOD5_ON      = "mod_5_on";
    static constexpr const char* MOD5_SOURCE  = "mod_5_source";
    static constexpr const char* MOD5_DEST    = "mod_5_dest";
    static constexpr const char* MOD5_AMOUNT  = "mod_5_amount";
    static constexpr const char* MOD6_ON      = "mod_6_on";
    static constexpr const char* MOD6_SOURCE  = "mod_6_source";
    static constexpr const char* MOD6_DEST    = "mod_6_dest";
    static constexpr const char* MOD6_AMOUNT  = "mod_6_amount";
    static constexpr const char* MOD7_ON      = "mod_7_on";
    static constexpr const char* MOD7_SOURCE  = "mod_7_source";
    static constexpr const char* MOD7_DEST    = "mod_7_dest";
    static constexpr const char* MOD7_AMOUNT  = "mod_7_amount";

    // --- Source engine (v3) ---
    static constexpr const char* OSC1_TYPE   = "osc1_type";
    static constexpr const char* OSC1_TUNE   = "osc1_tune";
    static constexpr const char* OSC1_FINE   = "osc1_fine";
    static constexpr const char* OSC1_SHAPE  = "osc1_shape";
    static constexpr const char* OSC1_LEVEL  = "osc1_level";
    static constexpr const char* OSC1_PAN    = "osc1_pan";
    static constexpr const char* OSC2_TYPE   = "osc2_type";
    static constexpr const char* OSC2_TUNE   = "osc2_tune";
    static constexpr const char* OSC2_FINE   = "osc2_fine";
    static constexpr const char* OSC2_SHAPE  = "osc2_shape";
    static constexpr const char* OSC2_LEVEL  = "osc2_level";
    static constexpr const char* OSC2_PAN    = "osc2_pan";
    static constexpr const char* SOURCE_BLEND = "source_blend";

    // --- Synth / filter (v3) ---
    /** When false, the shared filter is hard-bypassed (no LP/HP coloring at default cutoff). */
    static constexpr const char* FILTER_ENABLED   = "filter_enabled";
    static constexpr const char* FILTER_CUTOFF    = "filter_cutoff";
    static constexpr const char* FILTER_RESONANCE = "filter_resonance";
    static constexpr const char* FILTER_TYPE      = "filter_type";
    static constexpr const char* FILTER_DRIVE     = "filter_drive";
    static constexpr const char* ENV_AMP_DECAY    = "env_amp_decay";
    static constexpr const char* ENV_AMP_SUSTAIN  = "env_amp_sustain";
    static constexpr const char* ENV_FLT_ATTACK   = "env_flt_attack";
    static constexpr const char* ENV_FLT_DECAY    = "env_flt_decay";
    static constexpr const char* ENV_FLT_SUSTAIN  = "env_flt_sustain";
    static constexpr const char* ENV_FLT_RELEASE  = "env_flt_release";
    static constexpr const char* ENV_FLT_AMOUNT   = "env_flt_amount";
    /** 0 = full-level playback regardless of MIDI velocity; 1 = normal velocity scaling. */
    static constexpr const char* VELOCITY_SENSITIVITY = "velocity_sensitivity";
    static constexpr const char* VOICE_POLYPHONY  = "voice_polyphony";
    static constexpr const char* VOICE_GLIDE_MODE = "voice_glide_mode";
    static constexpr const char* VOICE_PLAY_MODE  = "voice_play_mode";
    static constexpr const char* OUTPUT_LIMITER   = "output_limiter";

    static constexpr const char* LFO1_PHASE = "lfo1_phase";
    static constexpr const char* LFO2_PHASE = "lfo2_phase";
    static constexpr const char* LFO3_PHASE = "lfo3_phase";

    // --- Texture engine (v3) ---
    static constexpr const char* TEX_ENABLED       = "tex_enabled";
    static constexpr const char* TEX_AMOUNT        = "tex_amount";
    static constexpr const char* TEX_WIDTH         = "tex_width";
    static constexpr const char* TEX_FREEZE        = "tex_freeze";
    static constexpr const char* TEX_GRAIN_SCAN    = "tex_grain_scan";
    static constexpr const char* TEX_GRAIN_RATE    = "tex_grain_rate";
    static constexpr const char* TEX_GRAIN_SIZE    = "tex_grain_size";
    static constexpr const char* TEX_GRAIN_PITCH   = "tex_grain_pitch";
    static constexpr const char* TEX_GRAIN_DENSITY  = "tex_grain_density";
    static constexpr const char* TEX_GRAIN_SPREAD   = "tex_grain_spread";
    static constexpr const char* TEX_GRAIN_PAN     = "tex_grain_pan";
    static constexpr const char* TEX_MOTION        = "tex_motion";
    static constexpr const char* TEX_DRIFT         = "tex_drift";
    static constexpr const char* TEX_AIR           = "tex_air";
    static constexpr const char* TEX_REVERSE       = "tex_reverse";

    // --- Phrase engine (v3) ---
    static constexpr const char* PHRASE_ENABLED      = "phrase_enabled";
    static constexpr const char* PHRASE_TEMPO_SYNC   = "phrase_tempo_sync";
    static constexpr const char* PHRASE_KEY_SYNC      = "phrase_key_sync";
    static constexpr const char* PHRASE_TRIGGER_MODE  = "phrase_trigger_mode";
    static constexpr const char* PHRASE_LOOP          = "phrase_loop";
    static constexpr const char* PHRASE_START         = "phrase_start";
    static constexpr const char* PHRASE_LENGTH        = "phrase_length";
    static constexpr const char* PHRASE_PITCH         = "phrase_pitch";

    // --- FX routing (v3) ---
    static constexpr const char* FX_REVERB_DAMP  = "fx_reverb_damp";
    static constexpr const char* FX_REVERB_ON    = "fx_reverb_on";
    /** When true, preset state includes audio FX parameter edits. */
    static constexpr const char* FX_EDITS_ON     = "fx_edits_on";

    // --- Performance macros (matrix page) ---
    static constexpr const char* PERF_MACRO_1 = "perf_macro_1";
    static constexpr const char* PERF_MACRO_2 = "perf_macro_2";
    static constexpr const char* PERF_MACRO_3 = "perf_macro_3";
    static constexpr const char* PERF_MACRO_4 = "perf_macro_4";

    // --- Performance / Texture engine (v4) ---
    static constexpr const char* SRC_START         = "src_start";
    static constexpr const char* SRC_END           = "src_end";
    static constexpr const char* SRC_TUNE          = "src_tune";
    static constexpr const char* SRC_SPEED         = "src_speed";
    static constexpr const char* SRC_REVERSE       = "src_reverse";
    static constexpr const char* SRC_LOOP_MODE     = "src_loop_mode";
    static constexpr const char* SRC_BPM_SYNC        = "src_bpm_sync";
    static constexpr const char* SRC_ORIGINAL_BPM    = "src_original_bpm";
    static constexpr const char* SRC_ROOT_NOTE       = "src_root_note";
    static constexpr const char* SRC_PLAYBACK_MODE = "src_playback_mode";
    static constexpr const char* SRC_KEYTRACK       = "src_keytrack";

    static constexpr const char* CHOP_ON             = "chop_on";
    static constexpr const char* CHOP_AMOUNT         = "chop_amount";
    static constexpr const char* CHOP_RATE           = "chop_rate";
    static constexpr const char* CHOP_GATE           = "chop_gate";
    static constexpr const char* CHOP_SWING          = "chop_swing";
    static constexpr const char* CHOP_RANDOM         = "chop_random";
    static constexpr const char* CHOP_REVERSE_CHANCE = "chop_reverse_chance";
    static constexpr const char* CHOP_SMOOTH         = "chop_smooth";
    static constexpr int CHOP_STEP_COUNT = 16;

    static constexpr const char* PTEX_ON             = "ptex_on";
    static constexpr const char* PTEX_FREEZE         = "ptex_freeze";
    static constexpr const char* PTEX_GRAIN_SIZE     = "ptex_grain_size";
    static constexpr const char* PTEX_DENSITY        = "ptex_density";
    static constexpr const char* PTEX_POSITION       = "ptex_position";
    static constexpr const char* PTEX_PITCH_SPREAD   = "ptex_pitch_spread";
    static constexpr const char* PTEX_SMEAR          = "ptex_smear";
    static constexpr const char* PTEX_WIDTH          = "ptex_width";
    static constexpr const char* PTEX_MIX            = "ptex_mix";

    static constexpr const char* PERF_MODE           = "perf_mode";
    static constexpr const char* PERF_FX_STUTTER     = "perf_fx_stutter";
    static constexpr const char* PERF_FX_REVERSE     = "perf_fx_reverse";
    static constexpr const char* PERF_FX_HALF_TIME   = "perf_fx_half_time";
    static constexpr const char* PERF_FX_FREEZE      = "perf_fx_freeze";
    static constexpr const char* PERF_FX_TAPE_STOP     = "perf_fx_tape_stop";
    static constexpr const char* PERF_FX_SCATTER       = "perf_fx_scatter";
    static constexpr const char* PERF_FX_PITCH_DROP    = "perf_fx_pitch_drop";
    static constexpr const char* PERF_FX_FILTER_SWEEP  = "perf_fx_filter_sweep";

    // --- Flight Deck (v5) — MIDI arpeggiator + sample-flip lever ---
    // Arpeggiator: host-synced note scheduler in front of the sampler/synth.
    static constexpr const char* ARP_ON         = "arp_on";          // bool, default off
    static constexpr const char* ARP_MODE       = "arp_mode";        // choice: Up/Down/Up-Down/Random/As Played
    static constexpr const char* ARP_RATE       = "arp_rate";        // choice: 1/4 1/8 1/16 1/32
    static constexpr const char* ARP_FEEL       = "arp_feel";        // choice: Straight/Triplet/Dotted
    static constexpr const char* ARP_OCTAVES    = "arp_octaves";     // int 1..4
    static constexpr const char* ARP_GATE       = "arp_gate";        // 0.05..1 step length fraction
    static constexpr const char* ARP_SWING      = "arp_swing";       // 0..1 odd-step delay
    static constexpr const char* ARP_HUMANIZE   = "arp_humanize";    // 0..1 timing + velocity jitter
    static constexpr const char* ARP_OCT_SPREAD = "arp_oct_spread";  // 0..1 chance of a ±1 octave jump per step
    static constexpr const char* ARP_HOLD       = "arp_hold";        // bool latch
    static constexpr const char* ARP_TARGET     = "arp_target";      // choice: Slices/Notes
    // Sample flip (MANEUVER lever). `reverse` is the lever itself.
    static constexpr const char* FLIP_WINDOW    = "flip_window";     // choice: Phrase/Slice/Beat
    static constexpr const char* FLIP_SNAP      = "flip_snap";       // choice: Off/1/4/1/8/1/16
    static constexpr int ARP_NUM_SLICES = 16;

    // --- MFX rack (v5) — two generic effect slots, 28 IDs each -----------------
    // IDs are generated by Mfx::onId/effectId/sendId/levelId/paramId/assign*Id
    // in Source/DSP/Mfx/MfxDescriptors.h:
    //   mfx{1,2}_on, mfx{1,2}_effect, mfx{1,2}_send, mfx{1,2}_level,
    //   mfx{1,2}_p01..p16 (normalised 0..1, relabelled per effect),
    //   mfx{1,2}_asg{1..4}_src, mfx{1,2}_asg{1..4}_amt
    // New effects never add IDs: only a descriptor + DSP class.
    static constexpr int MFX_NUM_SLOTS = 2;
    static constexpr int MFX_PARAMS_PER_SLOT = 16;
    static constexpr int MFX_NUM_ASSIGNS = 4;

    /** chop_stepNN_on / vol / offset / rev / pitch — NN = 00..15 */
    inline juce::String chopStepParamId (int step, const char* suffix)
    {
        jassert (step >= 0 && step < CHOP_STEP_COUNT);
        return juce::String::formatted ("chop_step%02d_%s", step, suffix);
    }

} // namespace ParamID

// ---------------------------------------------------------------------------
// Preset category names — used in the browser and preset XML.
// Order here defines the display order in the browser.
// ---------------------------------------------------------------------------
namespace Category {
    static constexpr const char* LEADS      = "Leads";
    static constexpr const char* BRASS      = "Brass";
    static constexpr const char* ENSEMBLES  = "Ensembles";
    static constexpr const char* STRINGS    = "Strings";
    static constexpr const char* PADS       = "Pads";
    static constexpr const char* CHORDS     = "Chords";
    static constexpr const char* SYNTHS     = "Synths";
    static constexpr const char* ARPS       = "Arps";
    static constexpr const char* VOCALS     = "Vocals";
    static constexpr const char* BELLS      = "Bells";
}

} // namespace AviatorKeyz
