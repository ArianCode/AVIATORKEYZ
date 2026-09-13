#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include "Reverb/AviationReverb.h"

// =============================================================================
//  ReverbTail — Space section reverb
//
//  Runs the AviationReverb engine (Source/DSP/Reverb) behind the main page
//  reverb controls.
//
//  Parameter mapping:
//    reverb_amount   dry/wet, equal-power (dry = cos, wet = sin)
//    reverb_size     engine size (decay time + space scale)
//    fx_reverb_damp  engine damping
//    fx_reverb_mode  algorithm (Plate / Hall / Room / Cloud / Hardware)
//    fx_reverb_color Modern / Vintage
//    fx_reverb_on    bypass — the mix glides to dry before the engine stops
//
//  Call process() every block, including while off, so on/off and amount
//  changes ramp instead of clicking. Once fully dry it costs nothing and the
//  tail is cleared, so re-enabling never replays a stale tail.
//
//  Threading: called from processBlock — audio thread only, no allocations.
// =============================================================================

class ReverbTail
{
public:
    ReverbTail();
    ~ReverbTail();

    void prepare (const juce::dsp::ProcessSpec& spec);
    void reset();

    // reverbAmount: 0.0–1.0 dry/wet
    // reverbSize:   0.0–1.0 size / decay
    // reverbOn:     glide to dry and stop when false
    // damping:      0.0–1.0 high-frequency damping
    void process (juce::AudioBuffer<float>& buffer,
                  float reverbAmount,
                  float reverbSize,
                  bool reverbOn,
                  float damping,
                  AviationReverb::Algorithm algorithm = AviationReverb::Algorithm::plate,
                  AviationReverb::Color color = AviationReverb::Color::modern);

    /** True while the reverb is audible (wet mix above silence or still ramping). */
    bool isActive() const noexcept { return active; }

private:
    AviationReverb::Engine engine;
    juce::AudioBuffer<float> wet;
    juce::SmoothedValue<float> mix;
    bool active   { false };
    bool prepared { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ReverbTail)
};
