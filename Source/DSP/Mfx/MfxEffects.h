#pragma once

#include "MfxDescriptors.h"
#include "../TextureEngine.h"
#include "../FilterProcessor.h"
#include "../Reverb/AviationReverb.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <array>

// =============================================================================
//  MfxEffects — the DSP behind each slot effect. All buffers are allocated in
//  prepare(); process() is allocation-free and takes the 16 slot values in
//  REAL units (already denormalised + modulated by the rack).
// =============================================================================

namespace Mfx
{
    struct Clock
    {
        double bpm { 120.0 };
        double beatPos { 0.0 };      // beats at block start
        double sampleRate { 44100.0 };
        double beatsPerSample() const noexcept { return bpm / 60.0 / sampleRate; }
    };

    using Values = std::array<float, kParamsPerSlot>;

    class EffectProcessor
    {
    public:
        virtual ~EffectProcessor() = default;
        virtual void prepare (const juce::dsp::ProcessSpec& spec) = 0;
        virtual void reset() = 0;
        virtual void process (juce::AudioBuffer<float>& buffer, const Values& v, const Clock& clock) noexcept = 0;
    };

    /** Wet/dry helper: out = dry*(1-mix) + wet*mix, mix 0..1 (buffer holds wet, dry supplied). */
    void blendDry (juce::AudioBuffer<float>& wet, const juce::AudioBuffer<float>& dry, float mix) noexcept;

    // -------------------------------------------------------------------------
    class GrainCloud final : public EffectProcessor
    {
    public:
        void prepare (const juce::dsp::ProcessSpec& spec) override;
        void reset() override;
        void process (juce::AudioBuffer<float>& buffer, const Values& v, const Clock& clock) noexcept override;
    private:
        TextureEngine engine;
        juce::AudioBuffer<float> dry;
    };

    // -------------------------------------------------------------------------
    class SweepFilter final : public EffectProcessor
    {
    public:
        void prepare (const juce::dsp::ProcessSpec& spec) override;
        void reset() override;
        void process (juce::AudioBuffer<float>& buffer, const Values& v, const Clock& clock) noexcept override;
        /** Envelope follower level for the ENV AMT slot (set by the rack each block). */
        void setEnvelopeLevel (float l) noexcept { envLevel = l; }
    private:
        FilterProcessor filter;
        juce::AudioBuffer<float> dry;
        double sampleRate { 44100.0 };
        float lfoPhase { 0.f };
        float envLevel { 0.f };
    };

    // (the delay slot lives in AviationDelay.h)

    // -------------------------------------------------------------------------
    class Saturator final : public EffectProcessor
    {
    public:
        void prepare (const juce::dsp::ProcessSpec& spec) override;
        void reset() override;
        void process (juce::AudioBuffer<float>& buffer, const Values& v, const Clock& clock) noexcept override;
    private:
        float toneStateL { 0.f }, toneStateR { 0.f };
        float dcL { 0.f }, dcR { 0.f }, dcInL { 0.f }, dcInR { 0.f };
        double sampleRate { 44100.0 };
        juce::AudioBuffer<float> dry;
    };

    // -------------------------------------------------------------------------
    class Stutter final : public EffectProcessor
    {
    public:
        void prepare (const juce::dsp::ProcessSpec& spec) override;
        void reset() override;
        void process (juce::AudioBuffer<float>& buffer, const Values& v, const Clock& clock) noexcept override;
    private:
        static constexpr double kMaxCaptureSec = 2.5;
        juce::AudioBuffer<float> capture;   // circular
        int captureLen { 1 };
        int writePos { 0 };
        int loopStart { 0 };
        int loopLen { 1 };
        double readPos { 0.0 };
        long long lastDivisionIndex { -1 };
        double sampleRate { 44100.0 };
        float repeatGain { 1.f };
        juce::Random rng;
        juce::AudioBuffer<float> dry;
    };

    // -------------------------------------------------------------------------
    class Freeze final : public EffectProcessor
    {
    public:
        void prepare (const juce::dsp::ProcessSpec& spec) override;
        void reset() override;
        void process (juce::AudioBuffer<float>& buffer, const Values& v, const Clock& clock) noexcept override;
    private:
        struct Grain { double readPos { 0.0 }; float phase { 1.f }; int len { 1 }; float gain { 1.f }; };
        static constexpr double kRingSec = 2.0;
        float readRing (int ch, double pos) const noexcept;
        void spawn (Grain& g, int grainLen, double anchor, float startPhase) noexcept;

        juce::AudioBuffer<float> ring;
        int ringLen { 1 };
        int writePos { 0 };
        bool frozen { false };
        double freezePoint { 0.0 };
        float cycleGain { 1.f };
        Grain grains[2];
        int  warmSamples { 0 };     // audio captured since reset (freeze needs material)
        int  warmTarget { 22050 };
        double sampleRate { 44100.0 };
        juce::Random rng;
        juce::AudioBuffer<float> dry;
    };

    // -------------------------------------------------------------------------
    class Bitcrusher final : public EffectProcessor
    {
    public:
        void prepare (const juce::dsp::ProcessSpec& spec) override;
        void reset() override;
        void process (juce::AudioBuffer<float>& buffer, const Values& v, const Clock& clock) noexcept override;
    private:
        double sampleRate { 44100.0 };
        float phase { 0.f };
        float holdL { 0.f }, holdR { 0.f };
        float lpL { 0.f }, lpR { 0.f };
        float jitterScale { 1.f };
        juce::Random rng;
        juce::AudioBuffer<float> dry;
    };

    // -------------------------------------------------------------------------
    class Compressor final : public EffectProcessor
    {
    public:
        void prepare (const juce::dsp::ProcessSpec& spec) override;
        void reset() override;
        void process (juce::AudioBuffer<float>& buffer, const Values& v, const Clock& clock) noexcept override;
    private:
        juce::dsp::Compressor<float> comp;
        juce::AudioBuffer<float> dry;
    };

    // -------------------------------------------------------------------------
    class Chorus final : public EffectProcessor
    {
    public:
        void prepare (const juce::dsp::ProcessSpec& spec) override;
        void reset() override;
        void process (juce::AudioBuffer<float>& buffer, const Values& v, const Clock& clock) noexcept override;
    private:
        juce::dsp::Chorus<float> chorus;
    };

    // -------------------------------------------------------------------------
    class Space final : public EffectProcessor
    {
    public:
        void prepare (const juce::dsp::ProcessSpec& spec) override;
        void reset() override;
        void process (juce::AudioBuffer<float>& buffer, const Values& v, const Clock& clock) noexcept override;
    private:
        AviationReverb::Engine reverb;    // Hall; pre-delay handled by the engine
        float hpL { 0.f }, hpR { 0.f }, hpInL { 0.f }, hpInR { 0.f };
        double sampleRate { 44100.0 };
        juce::AudioBuffer<float> dry;
    };
} // namespace Mfx
