#pragma once

#include <juce_core/juce_core.h>
#include <algorithm>
#include <cmath>

// =============================================================================
//  ArpPattern — pure, allocation-free arpeggiator sequencing helpers.
//
//  Shared by the audio-thread Arpeggiator and the FLIGHT DECK arp visualizer
//  so both derive the identical step sequence from the same held-note set.
// =============================================================================

namespace Arp
{
    enum class Mode : int { up = 0, down, upDown, random, asPlayed };
    enum class Rate : int { quarter = 0, eighth, sixteenth, thirtySecond };
    enum class Feel : int { straight = 0, triplet, dotted };
    enum class Target : int { slices = 0, notes };

    static constexpr int kMaxHeld     = 32;
    static constexpr int kMaxSequence = 256;
    static constexpr int kNumSlices   = 16;
    /** MIDI note that maps to slice 0 in SLICES target (C1). */
    static constexpr int kSliceBaseNote = 36;

    struct Settings
    {
        bool   on { false };
        Mode   mode { Mode::up };
        Rate   rate { Rate::sixteenth };
        Feel   feel { Feel::straight };
        int    octaves { 1 };
        float  gate { 0.7f };
        float  swing { 0.f };
        float  humanize { 0.f };
        float  octSpread { 0.f };
        bool   hold { false };
        Target target { Target::notes };
        int    numSlices { kNumSlices }; // pads across the window (SLICE_DIV)
    };

    struct HeldNote
    {
        int   note { 0 };
        float velocity { 1.f };
        int   order { 0 };
    };

    struct Step
    {
        int   note { 0 };       // final MIDI note (base + 12 * octave)
        int   baseNote { 0 };   // held note this step came from
        int   octave { 0 };     // 0..octaves-1
        float velocity { 1.f };
    };

    /** Length of one arp step in beats (quarter notes). */
    inline double stepLengthBeats (Rate rate, Feel feel) noexcept
    {
        double beats = 1.0;
        switch (rate)
        {
            case Rate::quarter:      beats = 1.0;   break;
            case Rate::eighth:       beats = 0.5;   break;
            case Rate::sixteenth:    beats = 0.25;  break;
            case Rate::thirtySecond: beats = 0.125; break;
        }
        switch (feel)
        {
            case Feel::triplet: beats *= 2.0 / 3.0; break;
            case Feel::dotted:  beats *= 1.5;       break;
            case Feel::straight: default: break;
        }
        return beats;
    }

    /** Slice index (0..numSlices-1) a held note selects in SLICES target: C1 = slice 0. */
    inline int sliceIndexForNote (int baseNote, int octave = 0, int numSlices = kNumSlices) noexcept
    {
        const int n = numSlices > 0 ? numSlices : kNumSlices;
        const int rel = baseNote - kSliceBaseNote + octave * 4;
        return ((rel % n) + n) % n;
    }

    /**
        Builds the step sequence for a held-note set.
        Random mode returns the ordered pool; the caller picks a random index
        per step. Returns the number of steps written (<= maxOut).
    */
    inline int buildSequence (const HeldNote* held, int numHeld, Mode mode, int octaves,
                              Step* out, int maxOut) noexcept
    {
        if (held == nullptr || numHeld <= 0 || out == nullptr || maxOut <= 0)
            return 0;

        const int n = std::min (numHeld, kMaxHeld);
        octaves = std::clamp (octaves, 1, 4);

        HeldNote sorted[kMaxHeld];
        for (int i = 0; i < n; ++i)
            sorted[i] = held[i];

        if (mode == Mode::asPlayed)
            std::sort (sorted, sorted + n, [] (const HeldNote& a, const HeldNote& b) { return a.order < b.order; });
        else
            std::sort (sorted, sorted + n, [] (const HeldNote& a, const HeldNote& b) { return a.note < b.note; });

        int count = 0;
        auto push = [&] (const HeldNote& h, int oct)
        {
            if (count >= maxOut)
                return;
            Step s;
            s.baseNote = h.note;
            s.octave = oct;
            s.note = std::clamp (h.note + 12 * oct, 0, 127);
            s.velocity = h.velocity;
            out[count++] = s;
        };

        switch (mode)
        {
            case Mode::down:
                for (int oct = octaves - 1; oct >= 0; --oct)
                    for (int i = n - 1; i >= 0; --i)
                        push (sorted[i], oct);
                break;

            case Mode::upDown:
            {
                for (int oct = 0; oct < octaves; ++oct)
                    for (int i = 0; i < n; ++i)
                        push (sorted[i], oct);
                const int upLen = count;
                // Mirror back down, skipping both turnaround notes.
                for (int k = upLen - 2; k >= 1; --k)
                {
                    if (count >= maxOut) break;
                    out[count++] = out[k];
                }
                break;
            }

            case Mode::up:
            case Mode::random:
            case Mode::asPlayed:
            default:
                for (int oct = 0; oct < octaves; ++oct)
                    for (int i = 0; i < n; ++i)
                        push (sorted[i], oct);
                break;
        }

        return count;
    }
} // namespace Arp
