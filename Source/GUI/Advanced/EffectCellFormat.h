#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

/** Shared value formatting for EffectCell and EffectCellChoice readouts. */
namespace EffectCellFormat
{
    enum class Format
    {
        percent,
        hz,
        ms,
        seconds,
        cents,
        semitones,
        pan,
        cutoff,
        decibels,
        integer,
        plain,
        bipolarPercent
    };

    juce::String formatValue (juce::AudioProcessorValueTreeState& apvts,
                              const juce::String& paramId,
                              Format format,
                              float rawValue);

    juce::String formatPan (float v);
}
