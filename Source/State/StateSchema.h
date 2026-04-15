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
// Presets reference sound by convention: v0 uses the shared factory wav for
// all factory presets; per-preset sample IDs come later.
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

    // v1: smear (transient blur), 0–1, default 0
    static constexpr const char* SMEAR          = "smear";

    // v1: tone tilt, -1.0 (dark/warm) to +1.0 (bright/clean), default 0
    static constexpr const char* TONE           = "tone";

    // --- Space / Output section ---
    // v1: reverb wet amount, 0–1, default 0
    static constexpr const char* REVERB_AMOUNT  = "reverb_amount";

    // v1: reverb size/decay, 0–1, default 0.5
    static constexpr const char* REVERB_SIZE    = "reverb_size";

    // v1: stereo width via M-S matrix, 0 (mono) to 2.0 (hyper-wide), default 1.0
    static constexpr const char* STEREO_WIDTH   = "stereo_width";

    // v1: envelope attack time, 0.5–5000 ms, default ~5 ms
    static constexpr const char* ENV_ATTACK     = "env_attack";

    // v1: envelope release time, 5–10000 ms, default 150 ms
    static constexpr const char* ENV_RELEASE    = "env_release";

    // v1: output pan, -1.0 (L) to +1.0 (R), default 0
    static constexpr const char* PAN            = "pan";

    // --- Future parameters go BELOW this line ---
    // v2+: reserved for Chorus, Delay, Envelope, Filter, etc.
    // Remember: never change existing IDs above.

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
