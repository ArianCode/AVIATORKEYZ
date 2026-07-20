#pragma once

#include <atomic>
#include <cmath>
#include <juce_audio_basics/juce_audio_basics.h>

/** RT-safe playback probes — atomics only, no allocation in processBlock. */
namespace PlaybackProbe
{
inline std::atomic<int>  activeSamplerVoices { 0 };
inline std::atomic<int>  activePhraseVoices { 0 };
inline std::atomic<int>  lastNoteNumber { -1 };
inline std::atomic<float> samplerPeak { 0.f };
inline std::atomic<float> texturePeak { 0.f };
inline std::atomic<float> fxPeak { 0.f };

inline void updateSamplerVoices (int count, int phraseCount) noexcept
{
    activeSamplerVoices.store (count, std::memory_order_relaxed);
    activePhraseVoices.store (phraseCount, std::memory_order_relaxed);
}

inline void updateLastNote (int note) noexcept
{
    lastNoteNumber.store (note, std::memory_order_relaxed);
}

inline void updatePeak (std::atomic<float>& slot, float peak) noexcept
{
    const float prev = slot.load (std::memory_order_relaxed);
    if (peak > prev)
        slot.store (peak, std::memory_order_relaxed);
}

inline float bufferPeak (const juce::AudioBuffer<float>& buffer) noexcept
{
    float peak = 0.f;
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        peak = std::max (peak, buffer.getMagnitude (ch, 0, buffer.getNumSamples()));
    return peak;
}
} // namespace PlaybackProbe
