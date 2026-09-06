#pragma once

#include "../State/SampleLibrary.h"
#include "Performance/PerformanceTypes.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <atomic>
#include <memory>
#include <vector>

// =============================================================================
//  StretchPlayer — pitch-preserving phrase voice (STRETCH playback mode).
//
//  Wraps Signalsmith Stretch (MIT). One mono voice: the phrase is fed into the
//  stretcher at `timeRatio` input samples per output sample (0.25x .. 4x, and
//  host-BPM sync on top), so tempo changes never move the pitch. KEY TRACK
//  becomes a transpose in semitones instead of a speed change, so a phrase can
//  be played chromatically at constant length. The MANEUVER lever reverses
//  the input stream in place.
//
//  Audio thread: render() performs no allocations. The stretcher's own
//  buffers are sized in prepare() (including one warm seek()).
// =============================================================================

class StretchPlayer
{
public:
    struct Params
    {
        float  speed { 1.f };            // 0.25 .. 4 (time ratio, pitch preserved)
        bool   bpmSync { false };
        float  originalBpm { 120.f };
        double hostBpm { 120.0 };
        float  tuneSemis { 0.f };
        bool   keytrack { false };
        float  start { 0.f };            // source window (normalised)
        float  end { 1.f };
        LoopMode loopMode { LoopMode::Gate };
        bool   reverse { false };
        float  attackMs { 0.f }, decayMs { 0.f }, sustain { 1.f }, releaseMs { 10.f };
        float  velocitySensitivity { 0.f };
    };

    StretchPlayer();
    ~StretchPlayer();

    void prepare (double sampleRate, int maxBlockSize);
    void reset();

    /** Message thread (processing suspended): the mono region to play, or nullptr. */
    void setRegion (const SampleLibrary::AudioRegion* region) noexcept { this->region = region; }

    void setParams (const Params& p) noexcept { params = p; }

    void noteOn (int midiNote, float velocity) noexcept;
    void noteOff (int midiNote) noexcept;
    void allNotesOff() noexcept;
    void allSoundOff() noexcept;

    bool isActive() const noexcept { return active; }
    float getPlayheadNorm() const noexcept { return playhead.load (std::memory_order_relaxed); }
    float getEnvLevel() const noexcept { return envLevel; }

    /** Adds the voice into both channels of `buffer`. */
    void render (juce::AudioBuffer<float>& buffer) noexcept;

    /** Effective input-samples-per-output-sample for the current params. */
    double currentTimeRatio() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl;

    enum class Stage { idle, attack, decay, sustain, release };
    void enterRelease() noexcept;
    void advanceEnvelope() noexcept;
    void finishAttack() noexcept;
    int  fillInput (float* dst, int numSamples) noexcept; // returns samples actually available

    const SampleLibrary::AudioRegion* region { nullptr };
    Params params;

    double sampleRate { 44100.0 };
    int    maxBlock { 512 };
    bool   prepared { false };

    bool   active { false };
    int    noteNumber { -1 };
    float  velocityGain { 1.f };
    double readPos { 0.0 };
    double inputAccum { 0.0 };
    bool   materialExhausted { false };

    Stage  stage { Stage::idle };
    float  envLevel { 0.f };
    float  envStep { 0.f };
    int    envSamplesLeft { 0 };

    std::vector<float> inBuf;
    std::vector<float> outBuf;
    std::atomic<float> playhead { 0.f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StretchPlayer)
};
