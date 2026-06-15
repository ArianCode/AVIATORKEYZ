#pragma once

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>

#if defined (AVIATORKEYZ_DEBUG) && AVIATORKEYZ_DEBUG
  #define AK_ASSERT(x) jassert (x)
  #define AK_LOG(msg)  DBG (msg)

  #if JUCE_DEBUG
    inline void printBufferLevel (const juce::String& stage,
                                  const juce::AudioBuffer<float>& buffer)
    {
        float peak = 0.0f;

        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
            peak = std::max (peak, buffer.getMagnitude (channel, 0, buffer.getNumSamples()));

        const float peakDb = juce::Decibels::gainToDecibels (peak, -120.0f);
        DBG (stage + ": " + juce::String (peakDb, 2) + " dBFS");
    }
  #else
    inline void printBufferLevel (const juce::String&, const juce::AudioBuffer<float>&) {}
  #endif
#else
  #define AK_ASSERT(x)
  #define AK_LOG(msg)
  inline void printBufferLevel (const juce::String&, const juce::AudioBuffer<float>&) {}
#endif
