#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <cmath>

/** Header-only audio-thread math helpers — no STL containers. */
namespace AviatorFastMath
{
constexpr float kLn2Over12  = 0.05776226504666215f;  // ln(2) / 12
constexpr float kMidiA4Hz     = 440.f;
constexpr float kMidiA4Note   = 69.f;

inline float semitoneRatio (float semitones) noexcept
{
    return std::exp (semitones * kLn2Over12);
}

inline float midiNoteToHz (float note) noexcept
{
    return kMidiA4Hz * semitoneRatio (note - kMidiA4Note);
}

inline void constantPowerPan (float pan, float& left, float& right) noexcept
{
    // pan -1..+1 maps to 0..pi/2 so that L²+R² = 1 across the whole range
    // and L == R == 1/sqrt(2) at center.
    const float ang = (juce::jlimit (-1.f, 1.f, pan) + 1.f)
                      * (juce::MathConstants<float>::halfPi * 0.5f);
    left  = std::cos (ang);
    right = std::sin (ang);
}

inline float fastSin (float x) noexcept
{
    // Parabolic sine approximation — ~0.1% max error, no libm call.
    constexpr float invPi  = 1.f / juce::MathConstants<float>::pi;
    constexpr float invHalfPi = 2.f * invPi;

    x = std::fmod (x + juce::MathConstants<float>::pi,
                   juce::MathConstants<float>::twoPi);
    if (x < 0.f)
        x += juce::MathConstants<float>::twoPi;
    x -= juce::MathConstants<float>::pi;

    const float y = x * (invHalfPi - invPi * std::abs (x));
    return y * (0.775f + 0.225f * y * y);
}

inline float fastSinPhase01 (float phase01) noexcept
{
    return fastSin (phase01 * juce::MathConstants<float>::twoPi);
}

inline float hannWindow (float phase01) noexcept
{
    return 0.5f * (1.f - fastSinPhase01 (phase01));
}

inline float wrapPhase01 (float phase) noexcept
{
    phase = std::fmod (phase, 1.f);
    if (phase < 0.f)
        phase += 1.f;
    return phase;
}

} // namespace AviatorFastMath
