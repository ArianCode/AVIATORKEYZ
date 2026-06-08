#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

#include "../State/SampleLibrary.h"

// =============================================================================
//  SamplerEngine — polyphonic sample + sine fallback
//
//  Audio thread: process() performs no allocations, file I/O, or parsing.
//  Sample map is read from SampleLibrary::AudioSnapshot (double-buffered).
// =============================================================================

class SamplerEngine
{
public:
    SamplerEngine();
    ~SamplerEngine();

    void prepare (const juce::dsp::ProcessSpec& spec);
    void releaseResources();

    /** Message thread — pointer must remain valid until the next publish + allSoundOff. */
    void setSampleSnapshot (const SampleLibrary::AudioSnapshot* snapshot) noexcept;

    void setEnvelopeTimesMs (float attackMs, float decayMs, float sustain01, float releaseMs) noexcept;

    void setPolyphony (int voices) noexcept;
    void setPlayMode (int mode) noexcept;
    void setGlideMode (int mode) noexcept;

    void setPhraseParams (bool enabled,
                          float startNorm,
                          float lengthNorm,
                          int pitchSemis,
                          bool loop,
                          bool tempoSync,
                          bool keySync,
                          double hostBpm) noexcept;

    void noteOn (int midiNote, float velocity, bool reverse, float glideTimeMs) noexcept;
    void noteOff (int midiNote) noexcept;
    void allNotesOff() noexcept;
    void allSoundOff() noexcept;

    void process (juce::AudioBuffer<float>& buffer);

private:
    enum class EnvStage
    {
        idle,
        attack,
        decay,
        sustain,
        release
    };

    enum class PlayMode : int { poly = 0, mono, legato };
    enum class GlideMode : int { off = 0, legato, always };

    struct Voice
    {
        bool     active = false;
        int      noteNumber = 0;
        float    velocity = 0.f;
        float    phase = 0.f;
        float    readPos = 0.f;
        bool     reversed = false;
        float    currentPitch = 60.f;
        float    targetPitch = 60.f;
        float    glideIncPerSample = 0.f;
        bool     gliding = false;
        EnvStage envStage = EnvStage::idle;
        float    envLevel = 0.f;
        float    envLinearStep = 0.f;
        int      envSegSamplesLeft = 0;

        const float* sampleData = nullptr;
        int          sampleNumFrames = 0;
        int          sampleRootNote = 60;
    };

    void startVoice (Voice& v,
                     int midiNote,
                     float velocity,
                     bool reverse,
                     float glideTimeMs,
                     const SampleLibrary::AudioRegion* region) noexcept;
    void enterRelease (Voice& v) noexcept;
    float renderVoiceSample (Voice& v) noexcept;
    void advanceEnvelope (Voice& v) noexcept;
    void advanceGlide (Voice& v) noexcept;
    int   findFreeOrStealVoice() noexcept;

    static float midiNoteToHz (float note) noexcept;

    static constexpr int kMaxVoices = 16;

    Voice    voices[kMaxVoices];
    double   sampleRate = 44100.0;

    const SampleLibrary::AudioSnapshot* sampleSnapshot = nullptr;

    float attackMs = 5.f;
    float decayMs = 300.f;
    float sustainLevel = 1.f;
    float releaseMs = 150.f;

    int maxVoices { 16 };
    PlayMode playMode { PlayMode::poly };
    GlideMode glideMode { GlideMode::off };
    int lastNoteForGlide = -1;
    int monoVoiceIndex { -1 };

    bool phraseEnabled { false };
    float phraseStartNorm { 0.f };
    float phraseLengthNorm { 1.f };
    int phrasePitchSemis { 0 };
    bool phraseLoop { false };
    bool phraseTempoSync { false };
    bool phraseKeySync { true };
    double phraseHostBpm { 120.0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SamplerEngine)
};
