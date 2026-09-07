#pragma once

#include <juce_core/juce_core.h>

// =============================================================================
//  SampleAnalysis — message-thread tempo + key estimation for CARGO HOLD imports.
//
//  Tempo: onset-energy autocorrelation over 60–200 BPM with an octave check.
//  Key:   FFT chroma correlated against Krumhansl–Kessler major/minor profiles.
//  Both are lightweight heuristics meant to pre-fill ORIG BPM / ROOT for a
//  dropped phrase; the user can still override either value.
// =============================================================================

namespace SampleAnalysis
{
    struct Result
    {
        float bpm { 0.f };             // 0 = undetermined
        float bpmConfidence { 0.f };   // 0..1
        int   keyPitchClass { -1 };    // 0 = C … 11 = B, -1 = undetermined
        bool  keyMinor { false };
        float keyConfidence { 0.f };   // 0..1
    };

    /** Analyses a mono buffer. Uses at most the first ~30 s. */
    Result analyzeMono (const float* mono, int numFrames, double sampleRate);

    /** "A min", "C# maj", or "—" when undetermined. */
    juce::String keyName (int pitchClass, bool minor);

    /** MIDI root note used for pitch: the key's tonic folded nearest to C4 (60). */
    int rootNoteForPitchClass (int pitchClass);
} // namespace SampleAnalysis
