#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

// =============================================================================
//  ReverbTail — M2
//
//  Implements the Space section: plate reverb driven by reverb_amount + reverb_size.
//
//  Design:
//    - Wraps juce::dsp::Reverb (Schroeder/Moorer plate reverb algorithm)
//    - reverb_amount (0–1) controls wet/dry mix
//    - reverb_size   (0–1) maps to reverb roomSize parameter
//    - Stereo in → stereo out
//
//  Parameter mapping (M2):
//    roomSize   = reverb_size
//    damping    = 0.5 (fixed; could become a param in a future version)
//    wetLevel   = reverb_amount
//    dryLevel   = 1.0 - reverb_amount
//    width      = 1.0 (full width; stereo_width param handles width separately)
//    freezeMode = 0.0 (off)
//
//  Threading: called from processBlock — audio thread only, no allocations.
//
//  Implemented in M2.
// =============================================================================

class ReverbTail
{
public:
    ReverbTail();
    ~ReverbTail();

    void prepare (const juce::dsp::ProcessSpec& spec);
    void reset();

    // reverbAmount: smoothed 0.0–1.0 (wet level)
    // reverbSize:   0.0–1.0 (room size / decay)
    void process (juce::AudioBuffer<float>& buffer,
                  float reverbAmount,
                  float reverbSize);

private:
    juce::dsp::Reverb reverb;
    bool              prepared { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ReverbTail)
};
