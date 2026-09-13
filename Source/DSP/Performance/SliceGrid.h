#pragma once

#include "PerformanceTypes.h"
#include <juce_core/juce_core.h>

// =============================================================================
//  SliceGrid — the one place that turns (trim window, SLICE settings) into pad
//  boundaries. Used by the SLICE keyboard mode, the arpeggiator's SLICES target
//  and the MANEUVER lever's SLICE window, so all three agree on where pad N
//  starts. Header-only and allocation-free: safe on the audio thread.
// =============================================================================

namespace SliceGrid
{
    static constexpr int kMaxSlices = 16;
    static constexpr int kSliceBaseNote = 36; // C1 = pad 0 (mirrors Arp::kSliceBaseNote)
    static constexpr int kDivisionTable[] = { 3, 4, 6, 8, 16 };
    static constexpr int kNumDivisionChoices = 5;

    inline int divisionsForChoice (int choice) noexcept
    {
        return kDivisionTable[juce::jlimit (0, kNumDivisionChoices - 1, choice)];
    }

    inline int choiceForDivisions (int divisions) noexcept
    {
        for (int i = 0; i < kNumDivisionChoices; ++i)
            if (kDivisionTable[i] == divisions)
                return i;
        return kNumDivisionChoices - 1;
    }

    /** Frame of cut `i` (0..n) inside [windowStart, windowEnd] with the user's
        nudge applied. Offsets are capped at ± half a slice so cuts never cross. */
    inline int cutFrame (const SliceSettings& s, int windowStart, int windowEnd, int i) noexcept
    {
        const int n = juce::jlimit (1, kMaxSlices, s.divisions);
        if (i <= 0) return windowStart;
        if (i >= n) return windowEnd;
        const float sliceLen = static_cast<float> (juce::jmax (1, windowEnd - windowStart)) / static_cast<float> (n);
        const float base = static_cast<float> (windowStart) + sliceLen * static_cast<float> (i);
        const float nudge = juce::jlimit (-1.f, 1.f, s.cutOffsets[static_cast<size_t> (i - 1)]) * sliceLen * 0.5f;
        return juce::jlimit (windowStart, windowEnd, static_cast<int> (base + nudge));
    }

    /** [start, end) frames of pad `index` (wrapped into 0..n-1); never shorter than 2 frames. */
    inline void sliceBounds (const SliceSettings& s, int windowStart, int windowEnd, int index,
                             int& startOut, int& endOut) noexcept
    {
        const int n = juce::jlimit (1, kMaxSlices, s.divisions);
        const int idx = ((index % n) + n) % n;
        int a = cutFrame (s, windowStart, windowEnd, idx);
        int b = cutFrame (s, windowStart, windowEnd, idx + 1);
        if (b < a + 2) b = juce::jmin (windowEnd, a + 2);
        if (b < a + 2) a = juce::jmax (windowStart, b - 2);
        startOut = a;
        endOut = b;
    }

    /** Pad a key selects: C1 = pad 0, wrapping every `divisions` keys. */
    inline int indexForNote (int note, int divisions) noexcept
    {
        const int n = juce::jlimit (1, kMaxSlices, divisions);
        const int rel = note - kSliceBaseNote;
        return ((rel % n) + n) % n;
    }
} // namespace SliceGrid
