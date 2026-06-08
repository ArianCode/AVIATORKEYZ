#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

/** Lightweight mono pitch analysis for tests and import validation. */
struct PitchProbeResult
{
    float detectedMidi { 60.f };
    float confidence { 0.f };
};

class PitchProbe
{
public:
    static PitchProbeResult analyzeMono (const float* data,
                                         int numSamples,
                                         double sampleRate,
                                         int targetMidiHint = 60) noexcept;

    /** Pitch-shift mono buffer toward targetMidi; returns chosen root note. */
    static int correctBufferToRoot (juce::AudioBuffer<float>& monoBuffer,
                                    double sampleRate,
                                    int targetMidi = 60) noexcept;

private:
    static float foldMidiNearTarget (float midi, int target) noexcept;
};
