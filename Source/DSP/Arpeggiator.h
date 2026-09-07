#pragma once

#include "ArpPattern.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <atomic>
#include <cstdint>

// =============================================================================
//  Arpeggiator — audio-thread MIDI note scheduler (no allocations).
//
//  Consumes the host's note on/off stream, keeps the held chord (with HOLD
//  latch and sustain), and emits sample-accurate arp events on a beat clock.
//  The clock follows the host PPQ position when the transport runs and
//  free-runs from the tempo otherwise, so the pattern locks to the DAW grid
//  but still plays in standalone.
//
//  Events carry the original held note plus a slice index so the processor
//  can route SLICES target steps to a phrase window instead of a pitch.
// =============================================================================

class Arpeggiator
{
public:
    struct Event
    {
        int   samplePos { 0 };
        bool  noteOn { false };
        int   note { 60 };
        float velocity { 1.f };
        int   sliceIndex { 0 };
    };

    static constexpr int kMaxEvents = 256;

    /** UI-facing snapshot (all atomics, read from the message thread). */
    struct UiState
    {
        std::atomic<uint64_t> heldMaskLo { 0 };
        std::atomic<uint64_t> heldMaskHi { 0 };
        std::atomic<int>      currentIndex { -1 };
        std::atomic<int>      sequenceLength { 0 };
        std::atomic<bool>     running { false };
    };

    Arpeggiator();

    void prepare (double sampleRate);
    void reset();

    void setSettings (const Arp::Settings& s) noexcept;
    const Arp::Settings& getSettings() const noexcept { return settings; }

    /** Sustain pedal acts as a temporary HOLD. */
    void setSustain (bool down) noexcept;

    /**
        Runs one block. Host note on/off messages in `in` are consumed as chord
        input (other messages are ignored here — the caller forwards them).
        `hostBeat` < 0 means no transport position (free-run).
        Returns the number of events written to `out` (sorted by samplePos,
        note-offs before note-ons at equal positions).
    */
    int process (const juce::MidiBuffer& in,
                 int numSamples,
                 double bpm,
                 double hostBeat,
                 bool hostPlaying,
                 Event* out) noexcept;

    /** Emits note-offs for every sounding arp note at sample 0 and clears state. */
    int flushAllNotesOff (Event* out) noexcept;

    bool hasHeldNotes() const noexcept { return numHeld > 0; }
    const UiState& getUiState() const noexcept { return uiState; }

    /** Current beat position of the internal clock (for tests / flip snapping). */
    double getBeatPosition() const noexcept { return beatPos; }

private:
    struct PendingOff
    {
        int note { -1 };
        int sliceIndex { 0 };
        int samplesLeft { 0 };
    };

    static constexpr int kMaxPendingOffs = 64;

    void noteOn (int note, float velocity, double beatAtEvent) noexcept;
    void noteOff (int note) noexcept;
    void addHeld (int note, float velocity) noexcept;
    void removeHeld (int note) noexcept;
    void clearHeld() noexcept;
    void purgeUnheldWhenLatchReleased() noexcept;
    void rebuildSequence() noexcept;
    void restartPattern (double beatAtEvent) noexcept;
    void publishUiState() noexcept;
    void emitStep (int samplePos, double stepBeats, double beatsPerSample,
                   Event* out, int& count) noexcept;
    void addPendingOff (int note, int sliceIndex, int samplesUntilOff) noexcept;

    double sampleRate { 44100.0 };
    Arp::Settings settings {};
    bool sustainDown { false };

    // held chord
    Arp::HeldNote held[Arp::kMaxHeld] {};
    int  numHeld { 0 };
    bool physical[128] {};
    int  physicalCount { 0 };
    int  orderCounter { 0 };
    bool chordComplete { false }; // latch: all keys lifted, chord still sounding

    // sequence
    Arp::Step sequence[Arp::kMaxSequence] {};
    int seqLen { 0 };
    int seqIndex { 0 };
    int stepCounter { 0 };

    // clock
    double beatPos { 0.0 };
    double nextStepBeat { 0.0 };
    double lastStepBeats { 0.0 };
    bool   running { false };

    PendingOff pendingOffs[kMaxPendingOffs] {};
    int numPendingOffs { 0 };

    juce::Random rng;
    UiState uiState;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Arpeggiator)
};
