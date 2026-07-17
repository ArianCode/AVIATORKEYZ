#pragma once

#include "LfoEngine.h"
#include "State/StateSchema.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>

/** Modulation destinations addressable from the Advanced mod matrix. */
enum class ModDest : int
{
    none = 0,
    inputGain,
    smear,
    filter = smear,
    tone,
    reverbAmount,
    reverbSize,
    stereoWidth, // brightness (legacy enum name — maps to stereo_width param)
    brightness = stereoWidth,
    pan,
    delayMix,
    chorusMix,
    lofiAmount,
    distDrive,
    filterCutoff,
    filterResonance,
    osc1Level,
    osc2Level,
    textureAmount,
    grainRate,
    grainSize,
    count
};

/** Reads mod-matrix APVTS rows and accumulates LFO-driven offsets per destination. */
class ModMatrix
{
public:
    struct Offsets
    {
        float inputGainDb   { 0.f };
        float smear         { 0.f };
        float tone          { 0.f };
        float reverbAmount  { 0.f };
        float reverbSize    { 0.f };
        float brightness      { 0.f };
        float pan           { 0.f };
        float delayMix      { 0.f };
        float chorusMix     { 0.f };
        float lofiAmount    { 0.f };
        float distDrive     { 0.f };
        float filterCutoff  { 0.f };
        float filterReso    { 0.f };
        float osc1Level     { 0.f };
        float osc2Level     { 0.f };
        float textureAmount { 0.f };
        float grainRate     { 0.f };
        float grainSize     { 0.f };
    };

    void updateFromApvts (const juce::AudioProcessorValueTreeState& apvts,
                          const LfoEngine& lfos);

    const Offsets& getOffsets() const noexcept { return offsets; }

    static juce::StringArray sourceNames();
    static juce::StringArray destNames();

private:
    static float sourceValue (int sourceIndex, const LfoEngine& lfos);
    static void applyToDest (ModDest dest, float delta, Offsets& o);

    Offsets offsets {};
};
