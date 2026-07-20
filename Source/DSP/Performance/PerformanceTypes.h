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
    bool reverse = false;
    bool bpmSync = true;
    float originalBpm = 120.0f;
    /** Mirrored from the loaded sample for state/UI; pitch math uses voice sampleRootNote. */
    int rootNote = 60;
    LoopMode loopMode = LoopMode::Gate;
    SamplePlaybackMode playbackMode = SamplePlaybackMode::ChromaticResample;
    /** Deprecated — playback tracking is mode-driven from preset soundType. Kept for state compat. */
    bool keytrack = false;
};

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

struct EngineState
{
    SourceSettings source;
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
