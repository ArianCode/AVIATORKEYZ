#pragma once

#include <juce_core/juce_core.h>
#include <array>
#include <memory>

// =============================================================================
//  AviationReverb — multi-topology algorithmic reverb engine
//
//  Replaces the Freeverb (juce::Reverb) comb/all-pass network everywhere in
//  the plugin. Each algorithm is a different topology, not a preset of one
//  network, because topology changes the character far more than size/decay:
//
//    Plate     Dattorro figure-eight tank, sine-modulated decay all-passes
//    Hall      8-line Hadamard FDN, random delay modulation, light early refl.
//    Room      8-line Hadamard FDN, short lines, strong early reflections
//    Cloud     8-line FDN with in-loop all-pass diffusion, slow random drift
//    Hardware  ring of nested all-pass sections with a band-limited loop
//
//  Color is a separate stage applied around the active tank:
//    Modern    clean, full bandwidth
//    Vintage   converter-style band limiting + 12-bit magnitude truncation on
//              the tank input and wet output, reduced modulation depth
//
//  Output is WET ONLY; callers own the dry/wet law. Wet level is calibrated so
//  size 0.5 / damping 0.4 carries roughly the same energy as the juce::Reverb
//  it replaces (wetLevel 1, width 1), keeping existing mix/send amounts valid.
//
//  Threading: prepare()/reset() allocate or clear and belong on the message
//  thread (or prepareToPlay). setSettings() and process() are real-time safe:
//  no allocations, no locks. Changing the algorithm crossfades the old tank
//  out while the new one fades in.
// =============================================================================

namespace AviationReverb
{
enum class Algorithm : int { plate = 0, hall, room, cloud, hardware, count };
enum class Color : int { modern = 0, vintage, count };

/** Choice labels in enum order (used for APVTS choice params). */
juce::StringArray algorithmNames();
juce::StringArray colorNames();

inline Algorithm algorithmFromIndex (int index) noexcept
{
    return (Algorithm) juce::jlimit (0, (int) Algorithm::count - 1, index);
}

inline Color colorFromIndex (int index) noexcept
{
    return (Color) juce::jlimit (0, (int) Color::count - 1, index);
}

struct Settings
{
    Algorithm algorithm { Algorithm::plate };
    Color     color     { Color::modern };
    float size       { 0.5f };   // 0–1: decay time and space scale
    float damping    { 0.4f };   // 0–1: high-frequency decay (0 = bright)
    float width      { 1.0f };   // 0–1: wet stereo width
    float preDelayMs { -1.0f };  // < 0: algorithm default; otherwise 0–250 ms
};

/** Mid-band RT60 (seconds) an algorithm targets at a given size. */
float targetRt60Seconds (Algorithm algorithm, float size) noexcept;

class Engine
{
public:
    Engine();
    ~Engine();

    void prepare (double sampleRate, int maxBlockSize);
    void reset();
    bool isPrepared() const noexcept { return prepared; }

    void setSettings (const Settings& newSettings) noexcept;

    /** Reads inL/inR and writes the wet signal to outL/outR. In-place is fine. */
    void process (const float* inL, const float* inR,
                  float* outL, float* outR, int numSamples) noexcept;

    struct Impl;

private:
    std::unique_ptr<Impl> impl;
    bool prepared { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Engine)
};
} // namespace AviationReverb
