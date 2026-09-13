#pragma once

#include <juce_core/juce_core.h>
#include <array>
#include <cmath>

// =============================================================================
//  MfxDescriptors — the MFX slot concept.
//
//  A slot hosts one effect at a time. Every effect exposes up to 16 GENERIC
//  parameter slots (mfxN_p01..p16, stored normalised 0..1 in the APVTS). The
//  descriptor below supplies, per slot: the label, the musical range the
//  knob maps to, the (narrower) range the randomiser is allowed to roll, the
//  default, the unit, and whether the slot is "cautious" (feedback, drive,
//  resonance: the randomiser stays well inside its range).
//
//  Adding an effect later costs zero new parameter IDs — only a descriptor
//  and a DSP class.
// =============================================================================

namespace Mfx
{
    static constexpr int kNumSlots      = 2;   // rack: slot A -> slot B in series
    static constexpr int kParamsPerSlot = 16;
    static constexpr int kNumAssigns    = 4;
    static constexpr int kMaxPresets    = 8;

    enum class Effect : int
    {
        grainCloud = 0,
        sweepFilter,
        aviationDelay,   // multi-model delay (index 2, formerly Tape Echo — saved slots keep a delay)
        saturator,
        stutter,
        freeze,
        bitcrusher,
        compressor,
        chorus,
        space,
        count
    };

    enum class Category : int { texture = 0, filter, delay, drive, performance, lofi, dynamics, modulation, space };

    enum class ModSource : int { off = 0, modWheel, velocity, aftertouch, lfo1, envelope, macro1, notePitch, count };

    struct ParamSpec
    {
        const char* label { nullptr };  // nullptr = slot unused by this effect
        float min { 0.f };
        float max { 1.f };
        float rollMin { 0.f };          // randomiser range (musical), in real units
        float rollMax { 1.f };
        float def { 0.f };              // default, real units
        const char* unit { "" };
        bool  logScale { false };       // Hz / ms style mapping
        int   steps { 0 };              // > 0: integer stepped (choices), value = round
        bool  cautious { false };       // rate-limited by the randomiser

        bool used() const noexcept { return label != nullptr; }

        /** normalised 0..1 -> real units */
        float denormalise (float n) const noexcept
        {
            n = n < 0.f ? 0.f : (n > 1.f ? 1.f : n);
            float v;
            if (logScale && min > 0.f && max > min)
                v = std::exp (std::log (min) + n * (std::log (max) - std::log (min)));
            else
                v = min + n * (max - min);
            if (steps > 0)
                v = std::round (v);
            return v;
        }

        /** real units -> normalised 0..1 */
        float normalise (float v) const noexcept
        {
            v = v < min ? min : (v > max ? max : v);
            if (max <= min)
                return 0.f;
            if (logScale && min > 0.f)
                return (std::log (v) - std::log (min)) / (std::log (max) - std::log (min));
            return (v - min) / (max - min);
        }
    };

    struct Preset
    {
        const char* name { nullptr };
        std::array<float, kParamsPerSlot> values {}; // real units; unused slots ignored
    };

    struct Descriptor
    {
        Effect      effect;
        const char* id;          // stable short id (state / presets)
        const char* name;
        Category    category;
        std::array<ParamSpec, kParamsPerSlot> params;
        std::array<int, kNumAssigns> assignTargets;   // param index each ASSIGN row modulates (-1 = none)
        std::array<Preset, kMaxPresets> presets;

        int numUsedParams() const noexcept
        {
            int n = 0;
            for (const auto& p : params) if (p.used()) ++n;
            return n;
        }
    };

    const Descriptor& descriptor (Effect e) noexcept;
    inline const Descriptor& descriptor (int index) noexcept
    {
        return descriptor (static_cast<Effect> (juce::jlimit (0, (int) Effect::count - 1, index)));
    }

    const char* categoryName (Category c) noexcept;
    const char* modSourceName (ModSource s) noexcept;
    juce::StringArray effectNames();
    juce::StringArray modSourceNames();

    /** Normalised defaults for an effect (what a fresh slot loads). */
    std::array<float, kParamsPerSlot> defaultsNormalised (Effect e) noexcept;

    /** Preset -> normalised values (unused slots get the default). */
    std::array<float, kParamsPerSlot> presetNormalised (Effect e, int presetIndex) noexcept;

    /**
        Randomise within each slot's musical roll range.
        amount 0..1 blends from the current value toward the roll; locked slots
        are untouched; cautious slots roll only the lower 70% of their range.
        Pure function so it can be unit tested. Returns normalised values.
    */
    std::array<float, kParamsPerSlot> reroll (Effect e,
                                              const std::array<float, kParamsPerSlot>& currentNormalised,
                                              uint32_t lockMask,
                                              float amount,
                                              juce::Random& rng) noexcept;

    /** Human-readable value for a slot ("1.2k", "-4.0dB", "1/8", ...). */
    juce::String valueText (const ParamSpec& spec, float real);

    // --- parameter IDs ---------------------------------------------------------
    // mfx1_on, mfx1_effect, mfx1_send, mfx1_level, mfx1_p01..p16,
    // mfx1_asg1_src, mfx1_asg1_amt .. mfx1_asg4_*   (slot index 0 -> "mfx1_")
    juce::String slotPrefix (int slot);
    juce::String paramId (int slot, int paramIndex);         // p01..p16
    juce::String assignSourceId (int slot, int assignIndex);
    juce::String assignAmountId (int slot, int assignIndex);
    inline juce::String onId (int slot)     { return slotPrefix (slot) + "on"; }
    inline juce::String effectId (int slot) { return slotPrefix (slot) + "effect"; }
    inline juce::String sendId (int slot)   { return slotPrefix (slot) + "send"; }
    inline juce::String levelId (int slot)  { return slotPrefix (slot) + "level"; }

    static constexpr int kIdsPerSlot = 4 + kParamsPerSlot + kNumAssigns * 2; // 28
} // namespace Mfx
