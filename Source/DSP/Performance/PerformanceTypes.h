#pragma once

#include <array>
#include <juce_core/juce_core.h>

enum class PerformanceMode
{
    Normal = 0,
    Chop,
    Gate,
    Stutter,
    HalfTime,
    Reverse,
    Scatter,
    Freeze
};

enum class LoopMode
{
    OneShot = 0,
    Loop,
    Gate
};

/** How MIDI pitch affects sample read speed. */
enum class SamplePlaybackMode
{
    OneShotOriginal = 0,  // no MIDI pitch tracking
    PhraseOriginal,         // phrase at original speed / pitch
    ChromaticResample,      // classic sampler: pitch via speed
    PhraseTimeStretch,      // preserve length (time-stretch pitch later)
    SlicePhrase             // chop/slice: MIDI selects slices, not pitch
};

/** Whether MIDI note-off cuts playback or the sample runs to completion. */
enum class NoteGatePolicy
{
    Gated = 0,
    TriggerToEnd
};

/** How a new note-on interacts with voices already playing. */
enum class RetriggerPolicy
{
    Polyphonic = 0,
    PhraseChoke
};

/** Separated playback-rate components — never collapse into a single opaque ratio. */
struct PlaybackRates
{
    double sourceRateRatio = 1.0;
    double pitchRatio      = 1.0;
    double timeRatio       = 1.0;
    float  pitchSemitones  = 0.0f;
};

enum class MacroDestination
{
    None = 0,
    SampleStart,
    SampleEnd,
    Speed,
    Pitch,
    ChopAmount,
    GateAmount,
    Swing,
    RandomAmount,
    ReverseChance,
    TextureMix,
    GrainSize,
    Density,
    TexturePosition,
    PitchSpread,
    Smear,
    Width,
    FilterTone,
    Drive,
    DelayMix,
    ReverbMix,
    OutputLevel
};

struct StepData
{
    bool  enabled = true;
    float volume = 1.0f;
    float sliceOffset = 0.0f;
    bool  reverse = false;
    int   pitchOffset = 0;
};

struct ChopSettings
{
    bool enabled = false;
    float amount = 0.0f;
    int rateIndex = 2;
    float gate = 0.5f;
    float swing = 0.0f;
    float random = 0.0f;
    float reverseChance = 0.0f;
    float smooth = 0.01f;
    std::array<StepData, 16> steps {};
};

struct SourceSettings
{
    float start = 0.0f;
    float end = 1.0f;
    float tune = 0.0f;
    float speed = 1.0f;
    /** SPEED lock: speed plays on 0.25 steps (snapSpeedRatio). */
    bool speedSnap = false;
    bool reverse = false;
    bool bpmSync = true;
    float originalBpm = 120.0f;
    /** src_root_note: the sample's own root plus the user's re-root (state / UI mirror). */
    int rootNote = 60;
    /** User re-root in semitones, added to the sample region's own root at note-on.
        The processor derives it from rootNote every block. */
    int rootShift = 0;
    LoopMode loopMode = LoopMode::Gate;
    /** Sustain loop inside [start, end] for LoopMode::Loop, normalised to the
        whole sample. 0..1 loops the entire trim window. */
    float loopStart = 0.0f;
    float loopEnd = 1.0f;
    SamplePlaybackMode playbackMode = SamplePlaybackMode::ChromaticResample;
    /**
     * When true, MIDI note − sample root drives pitchRatio (chromatic resample).
     * Category policy sets this true for ChromaticResample presets and false for
     * fixed-pitch phrases / non-chromatic one-shots. Users can toggle it to
     * enable key tracking on phrases or lock a chromatic sound to original pitch.
     * Default true to match the ChromaticResample playbackMode default.
     */
    bool keytrack = true;
};

inline constexpr int   kMaxRootShiftSemis = 36;
inline constexpr float kSpeedSnapStep = 0.25f;

/** SPEED lock grid: nearest 0.25 step inside 0.25..4 (x0.5, x0.75, x1, x1.25 ...). */
inline float snapSpeedRatio (float speed) noexcept
{
    return juce::jlimit (0.25f, 4.0f, std::round (speed / kSpeedSnapStep) * kSpeedSnapStep);
}

struct TextureSettings
{
    bool enabled = false;
    bool freeze = false;
    float grainSize = 0.5f;
    float density = 0.5f;
    float position = 0.0f;
    float pitchSpread = 0.0f;
    float smear = 0.0f;
    float width = 0.5f;
    float mix = 0.0f;
};

struct MacroMapping
{
    MacroDestination destination = MacroDestination::None;
    float amount = 0.0f;
    float curve = 1.0f;
};

struct MacroControl
{
    juce::String name;
    std::array<MacroMapping, 8> mappings {};
};

struct PerformanceSettings
{
    PerformanceMode mode = PerformanceMode::Normal;
    bool stutter = false;
    bool reverse = false;
    bool halfTime = false;
    bool tapeStop = false;
    bool freeze = false;
    bool scatter = false;
    bool pitchDrop = false;
    bool filterSweep = false;
};

/** Slice pads: how the trim window is cut for SLICE mode / arp SLICES / flip SLICE. */
struct SliceSettings
{
    int divisions = 16;                   // 3 / 4 / 6 / 8 / 16 pads across the window
    float random = 0.0f;                  // chance (0..1) that a pad plays a random slice
    std::array<float, 15> cutOffsets {};  // per-cut nudge, -1..1 = ± half a slice
};

struct EngineState
{
    SourceSettings source;
    SliceSettings slice;
    ChopSettings chop;
    TextureSettings texture;
    PerformanceSettings performance;
    std::array<MacroControl, 4> macros;

    float toneOffset = 0.0f;
    float smearOffset = 0.0f;
    float reverbMixOffset = 0.0f;
    float delayMixOffset = 0.0f;
    float widthOffset = 0.0f;
    float driveOffset = 0.0f;
    float outputLevelOffset = 0.0f;
};

/** Runtime slice state pushed to SamplerEngine before render. */
struct ChopPlaybackState
{
    bool active = false;
    int sliceStartFrame = 0;
    int sliceEndFrame = 0;
    bool stepReverse = false;
    int pitchOffsetSemis = 0;
    float gateGain = 1.0f;
};
