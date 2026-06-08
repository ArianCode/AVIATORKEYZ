#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

/** Maps performance macro knobs (0–1) to bipolar parameter offsets. */
struct PerformanceMacroOffsets
{
    float textureAmount { 0.f };
    float filterCutoff  { 0.f };
    float reverbAmount  { 0.f };
    float smear         { 0.f };
    float tone          { 0.f };
    float grainRate     { 0.f };
    float osc1Level     { 0.f };
    float osc2Level     { 0.f };
};

class PerformanceMacroEngine
{
public:
    static PerformanceMacroOffsets compute (float macro1,
                                            float macro2,
                                            float macro3,
                                            float macro4) noexcept;
};
