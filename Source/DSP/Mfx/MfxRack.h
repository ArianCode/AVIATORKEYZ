#pragma once

#include "MfxDescriptors.h"
#include "MfxEffects.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <atomic>
#include <memory>

// =============================================================================
//  MfxRack — two MFX slots in series (A -> B) plus a shared reverb send/return.
//
//  Audio thread: process() reads the slot parameters through cached atomics,
//  denormalises them through the effect descriptor, applies the ASSIGN
//  modulation and the texture macro offsets, then runs the active effect.
//  Every effect instance is prepared up front so switching effects never
//  allocates; the previous effect is reset when a slot changes effect.
// =============================================================================

class MfxRack
{
public:
    struct ModSources
    {
        float modWheel { 0.f };
        float velocity { 0.f };
        float aftertouch { 0.f };
        float lfo1 { 0.f };
        float envelope { 0.f };
        float macro1 { 0.5f };
        float notePitch { 0.5f };

        float get (Mfx::ModSource s) const noexcept
        {
            switch (s)
            {
                case Mfx::ModSource::modWheel:   return modWheel;
                case Mfx::ModSource::velocity:   return velocity;
                case Mfx::ModSource::aftertouch: return aftertouch;
                case Mfx::ModSource::lfo1:       return lfo1;
                case Mfx::ModSource::envelope:   return envelope;
                case Mfx::ModSource::macro1:     return macro1;
                case Mfx::ModSource::notePitch:  return notePitch;
                case Mfx::ModSource::off:
                case Mfx::ModSource::count:
                default:                         return 0.f;
            }
        }
    };

    /** Macro offsets (delta from base) for a slot hosting Grain Cloud. */
    struct TextureMacroOffsets
    {
        float mix { 0.f }, grainSize { 0.f }, density { 0.f }, position { 0.f },
              pitchSpread { 0.f }, smear { 0.f }, width { 0.f };
    };

    MfxRack();

    /** Message thread, once: cache raw parameter pointers. */
    void attachParameters (juce::AudioProcessorValueTreeState& apvts);

    void prepare (const juce::dsp::ProcessSpec& spec);
    void reset();

    void process (juce::AudioBuffer<float>& buffer,
                  const Mfx::Clock& clock,
                  const ModSources& mods,
                  const TextureMacroOffsets& textureOffsets) noexcept;

    /** UI: per-slot activity meter (post-slot peak). */
    float getSlotLevel (int slot) const noexcept { return slotLevel[(size_t) juce::jlimit (0, 1, slot)].load (std::memory_order_relaxed); }

    /** UI: the last modulated real value per slot param (what the effect heard). */
    float getLiveValue (int slot, int param) const noexcept
    {
        return liveValues[(size_t) juce::jlimit (0, 1, slot)][(size_t) juce::jlimit (0, Mfx::kParamsPerSlot - 1, param)]
            .load (std::memory_order_relaxed);
    }

    static Mfx::EffectProcessor* makeEffect (Mfx::Effect e);

private:
    struct Slot
    {
        std::atomic<float>* on { nullptr };
        std::atomic<float>* effect { nullptr };
        std::atomic<float>* send { nullptr };
        std::atomic<float>* level { nullptr };
        std::array<std::atomic<float>*, Mfx::kParamsPerSlot> params {};
        std::array<std::atomic<float>*, Mfx::kNumAssigns> assignSrc {};
        std::array<std::atomic<float>*, Mfx::kNumAssigns> assignAmt {};

        std::array<std::unique_ptr<Mfx::EffectProcessor>, (size_t) Mfx::Effect::count> effects;
        int activeEffect { -1 };
        juce::SmoothedValue<float> levelSmoothed;
    };

    void processSlot (int index, juce::AudioBuffer<float>& buffer, const Mfx::Clock& clock,
                      const ModSources& mods, const TextureMacroOffsets& textureOffsets) noexcept;

    std::array<Slot, Mfx::kNumSlots> slots;
    std::array<std::atomic<float>, Mfx::kNumSlots> slotLevel {};
    std::array<std::array<std::atomic<float>, Mfx::kParamsPerSlot>, Mfx::kNumSlots> liveValues {};

    juce::Reverb sendReverb;
    juce::AudioBuffer<float> sendBus;
    bool prepared { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MfxRack)
};
