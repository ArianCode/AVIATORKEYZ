#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

// =============================================================================
//  GlideEngine — M2
//
//  Implements the "Glide" control: portamento pitch transition between notes.
//
//  Design:
//    - When glide time > 0, pitch slides from the previous note to the new one
//    - Slide is a linear semitone ramp (matches SamplerEngine / SynthEngine)
//    - Glide time parameter (0–500 ms) controls transition duration
//    - Mode: last-note priority (monophonic glide tracking)
//      Polyphonic glide per-voice is a stretch goal for a later version
//
//  Interaction with Reverse:
//    - Glide applies to the pitch source regardless of reverse mode
//    - Reverse affects buffer read direction, not pitch tracking
//    - These two are independent — no special interaction handling needed
//
//  Implemented in M2.
// =============================================================================

class GlideEngine
{
public:
    GlideEngine();
    ~GlideEngine();

    void setSampleRate (double sr);

  // Set ramp origin without starting a glide (used with last-note portamento)
    void snapToPitch (float semitones) noexcept;

    // Call on note-on with new MIDI note and current glide time in ms
    void noteOn (int midiNote, float glideTimeMs);

    // Returns current pitch in semitones — called per sample by SamplerEngine
    float getCurrentPitchSemitones() const noexcept;

    bool isGliding() const noexcept { return glideActive; }

    // Advances the internal glide state by one sample
    void tick() noexcept;

private:
    double sampleRate       { 44100.0 };
    float  currentPitch     { 60.0f };
    float  targetPitch      { 60.0f };
    float  glideIncrement   { 0.0f };
    bool   glideActive      { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GlideEngine)
};
