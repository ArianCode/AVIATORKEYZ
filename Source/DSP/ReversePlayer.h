#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

// =============================================================================
//  ReversePlayer — M2
//
//  Implements the "Reverse" control.
//
//  Design:
//    - When Reverse is ON, a triggered voice plays its sample backward
//    - Crucially: the note's rhythmic position in the host timeline is preserved
//      The sample simply reads from the END of the buffer toward the START
//    - This differs from a "tape reverse" effect that would need to buffer
//      future audio — here we reverse the sample itself at playback time
//
//  Implementation:
//    - SamplerEngine voices receive a "reversed" flag at note-on
//    - When reversed: readPosition starts at (sampleLength - 1) and decrements
//    - Looping in reverse: wraps from 0 back to (sampleLength - 1)
//    - Pitch shifting (from Glide) is applied to read increment magnitude;
//      direction is inverted when reversed
//
//  This is a per-voice flag rather than a global buffer-flip, so individual
//  notes can be triggered reversed at their natural onset time.
//
//  Implemented in M2.
// =============================================================================

class ReversePlayer
{
public:
    ReversePlayer()  = default;
    ~ReversePlayer() = default;

    // Returns the adjusted read increment for a voice given reverse state and pitch ratio
    // pitchRatio: resampling ratio (1.0 = unity, >1 = pitched up)
    // reversed: true when Reverse param is active
    static float getReadIncrement (float pitchRatio, bool reversed) noexcept;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ReversePlayer)
};
