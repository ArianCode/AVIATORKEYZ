#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

/** Dual-oscillator subtractive synth voice engine (polyphonic). */
class SynthEngine
{
public:
    enum class PlayMode : int { poly = 0, mono, legato };
    enum class GlideMode : int { off = 0, legato, always };

    SynthEngine() = default;

    void prepare (const juce::dsp::ProcessSpec& spec);
    void releaseResources();

    void setPolyphony (int voices) noexcept;
    void setPlayMode (PlayMode mode) noexcept;
    void setGlideMode (GlideMode mode) noexcept;

    void setOscParams (int oscIndex,
                       int type,
                       float tuneSemis,
                       float fineCents,
                       float shape01,
                       float level01,
                       float pan) noexcept;

    void setAmpEnvelopeMs (float attackMs, float decayMs, float sustain01, float releaseMs) noexcept;
    void setFilterEnvelopeMs (float attackMs, float decayMs, float sustain01, float releaseMs) noexcept;

    void noteOn (int midiNote, float velocity, float glideTimeMs) noexcept;
    void noteOff (int midiNote) noexcept;
    void allNotesOff() noexcept;
    void allSoundOff() noexcept;

    bool hasActiveVoices() const noexcept { return activeVoiceCount > 0; }

    /** Per-block filter envelope level (0–1) for global filter modulation. */
    float getFilterEnvLevel() const noexcept { return filterEnvLevel; }

    void process (juce::AudioBuffer<float>& buffer);

private:
    enum class EnvStage { idle, attack, decay, sustain, release };

    struct Voice
    {
        bool active = false;
        int noteNumber = 0;
        float velocity = 0.f;
        float osc1Phase = 0.f;
        float osc2Phase = 0.f;
        float noiseSeed = 0.f;
        float currentPitch = 60.f;
        float targetPitch = 60.f;
        float glideIncPerSample = 0.f;
        bool gliding = false;
        EnvStage ampStage = EnvStage::idle;
        float ampLevel = 0.f;
        float ampStep = 0.f;
        int ampSegLeft = 0;
        float ampSustain = 0.8f;
    };

    static float midiNoteToHz (float note) noexcept;
    static float oscSample (int type, float shape01, float phase01, float& noiseSeed) noexcept;

    void startVoice (Voice& v, int midiNote, float velocity, float glideTimeMs) noexcept;
    void enterRelease (Voice& v) noexcept;
    void advanceAmpEnv (Voice& v) noexcept;
    void advanceGlide (Voice& v) noexcept;
    void advanceFilterEnv (EnvStage noteStage) noexcept;
    float renderVoice (Voice& v) noexcept;
    int findFreeOrStealVoice() noexcept;
    int findMonoVoice() noexcept;

    static constexpr int kMaxVoices = 16;

    Voice voices[kMaxVoices];
    double sampleRate { 44100.0 };
    int maxVoices { 8 };
    int activeVoiceCount { 0 };
    PlayMode playMode { PlayMode::poly };
    GlideMode glideMode { GlideMode::off };
    int lastNote { -1 };
    int monoVoiceIndex { -1 };

    struct OscParams
    {
        int type = 0;
        float tune = 0.f;
        float fine = 0.f;
        float shape = 0.3f;
        float level = 0.7f;
        float pan = 0.f;
        float panL = 0.707f;
        float panR = 0.707f;
    };

    OscParams osc1 {};
    OscParams osc2 {};

    float ampAttackMs { 5.f };
    float ampDecayMs { 300.f };
    float ampSustain { 0.8f };
    float ampReleaseMs { 150.f };

    float fltAttackMs { 10.f };
    float fltDecayMs { 300.f };
    float fltSustain { 0.6f };
    float fltReleaseMs { 300.f };

    EnvStage filterEnvStage { EnvStage::idle };
    float filterEnvLevel { 0.f };
    float filterEnvStep { 0.f };
    int filterEnvSegLeft { 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SynthEngine)
};
