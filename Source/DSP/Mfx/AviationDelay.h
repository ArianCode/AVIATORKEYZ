#pragma once

#include "MfxEffects.h"
#include <array>
#include <cstdint>
#include <vector>

// =============================================================================
//  AviationDelay — the multi-model delay behind the MFX "Aviation Delay" slot.
//
//  MODE changes what the delay line *is*; STYLE changes how lines are routed.
//
//     in ──┬──────────────────────────────────────────────────── dry ─────┐
//          └─► (+) ◄──────────── feedback × FB ◄────────┐                  │
//               │                                       │                  │
//               ▼  RECORD PATH (per line)               │                  │
//            lo cut → hi cut → diffusion → mode model   │                  │
//               │   tape : record saturation, gap loss, │                  │
//               │          asperity noise               │                  │
//               │   BBD  : compressor, clock S&H, noise │                  │
//               │   lo-fi: converter rate + bits        │                  │
//               │   pitch: rotating-head shifter        │                  │
//               ▼                                       │                  │
//            delay line ──► PLAYBACK HEAD(S) ───────────┤                  │
//                           wow/flutter · reverse ·     ▼                  │
//                           multi-tap · BBD expander   stereo routing ─► duck ─► width ─► mix
//
//  Everything that colours the echo sits inside the loop, so the first repeat
//  is coloured once and every further repeat once more: tape repeats darken
//  and saturate, BBD repeats breathe and alias, pitch repeats climb, diffused
//  repeats smear from echo into wash.
//
//  Real-time contract: every buffer is sized in prepare(); process() never
//  allocates or locks, and the loop stays bounded at 100 % feedback.
// =============================================================================

namespace Mfx
{
    class AviationDelay final : public EffectProcessor
    {
    public:
        enum class Mode  : int { clean = 0, tape, analog, bbd, lofi, pitch, reverse, cloud, count };
        enum class Style : int { single = 0, stereo, pingPong, dual, ratio, quad, count };

        /** Indices into the 16 generic slot values (matches the descriptor table). */
        enum Param : int
        {
            pMode = 0, pStyle, pTime, pSync, pFeedback, pMix, pDiffusion, pModDepth,
            pModRate, pLoCut, pHiCut, pAge, pDuck, pPitch, pRatio, pWidth
        };

        static constexpr double kMaxTimeSec  = 2.0;
        static constexpr int    kMaxTaps     = 4;
        static constexpr int    kCleanStages = 6;
        static constexpr int    kCloudStages = 8;

        void prepare (const juce::dsp::ProcessSpec& spec) override;
        void reset() override;
        void process (juce::AudioBuffer<float>& buffer, const Values& v, const Clock& clock) noexcept override;

        /** Beats for a SYNC choice (0 = free). */
        static double syncBeats (int choice) noexcept;

        /** RATIO style: the nearest musical ratio (1/4 .. 1) to a 0.25..1 setting. */
        static float snappedRatio (float ratio) noexcept;

    private:
        // ---- building blocks -------------------------------------------------
        struct Line
        {
            std::vector<float> data;
            int size { 8 };
            int write { 0 };

            void allocate (int n);
            void clear() noexcept;
            void push (float x) noexcept { data[(size_t) write] = x; if (++write >= size) write = 0; }
            /** 4-point Hermite read; delay in samples, 1 = most recent (clamped >= 2). */
            float read (float delay) const noexcept;
            /** Linear read for the short modulated all-pass stages (clamped >= 1). */
            float readLinear (float delay) const noexcept;
        };

        /** Cytomic TPT state-variable filter, coefficients set per block. */
        struct Svf
        {
            float g { 0.f }, k { 1.41421356f }, a1 { 0.f }, a2 { 0.f }, a3 { 0.f };
            float ic1 { 0.f }, ic2 { 0.f };
            void set (float hz, float q, double sampleRate) noexcept;
            void clear() noexcept { ic1 = ic2 = 0.f; }
            float lowpass (float x) noexcept;
            float highpass (float x) noexcept;
        };

        /** Two crossfaded rotating heads over a short buffer (classic delay-line shifter). */
        struct Shifter
        {
            Line buf;
            double phase { 0.0 };
            float process (float x, double ratio, int window) noexcept;
        };

        /** Everything one delay line owns. Line A = left / mono, line B = right. */
        struct Voice
        {
            Line line;
            std::array<Line, kCloudStages> diffusers;
            Shifter shifter;
            Svf loCut, hiCut, convAA, convRecon;
            std::array<double, kMaxTaps> reversePhase {};
            double timeSm { 0.0 };            // smoothed delay time, samples
            float gapLp { 0.f };              // tape gap-loss one-pole
            float toneLp { 0.f };             // pitch-path top-end roll-off
            float noiseLp { 0.f };
            float satEnv { 0.f };             // tape asperity follower
            float noiseGate { 0.f };          // signal presence: keeps hiss out of an idle mix
            float compEnv { 0.f }, expEnv { 0.f }, expGain { 1.f };
            float holdValue { 0.f };
            double holdPhase { 0.0 };
            float dcIn { 0.f }, dcOut { 0.f };
            float drift { 0.f }, driftTarget { 0.f };
            int driftCountdown { 0 };

            /** Clears everything except the delay line itself (tails survive a mode change). */
            void clearState() noexcept;
        };

        struct BlockSettings;   // per-block derived values (defined in the .cpp)

        void resetVoices() noexcept;
        void configureBlock (const Values& v, const Clock& clock, BlockSettings& s) noexcept;
        float record (Voice& voice, int voiceIndex, float x, float diffScale, const BlockSettings& s) noexcept;
        float head (Voice& voice, int tap, double tapTimeSamples, double offset, const BlockSettings& s) noexcept;
        float nextNoise() noexcept;

        std::array<Voice, 2> voices;
        double sampleRate { 44100.0 };
        int pitchWindow { 2400 };
        std::array<std::array<float, kCloudStages>, 2> cleanStageLen {};   // samples
        std::array<std::array<float, kCloudStages>, 2> cloudStageLen {};
        std::array<double, kCloudStages> stageModPhase {};

        Mode  activeMode  { Mode::clean };
        Style activeStyle { Style::stereo };
        bool  firstBlock { true };
        bool  snapOnNextBlock { false };
        float switchGain { 1.f };
        int   switchDirection { 0 };          // -1 fading out for a mode/style change, +1 fading in

        // smoothed controls
        float fbCur { 0.f }, mixCur { 0.f }, widthCur { 1.f };
        float duckEnv { 0.f }, duckGain { 1.f };
        float diffEngage { 0.f };
        bool  diffusersDirty { false };

        // modulation
        double lfoPhase { 0.0 }, lfoPhase2 { 0.0 }, flutterPhase { 0.0 };

        // tape transport artefacts (splice bumps, dropouts)
        double spliceClock { 0.0 };
        int    degradeHold { 0 };
        float  degradeEnv { 0.f }, degradeDepth { 0.f };

        std::uint32_t rngState { 0x9e3779b9u };
    };
} // namespace Mfx
