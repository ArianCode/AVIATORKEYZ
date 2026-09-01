#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>

#include "../State/CategorySoundPolicy.h"
#include "../State/SampleLibrary.h"
#include "GlideEngine.h"
#include "Performance/PerformanceTypes.h"

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

    /** 0 = ignore MIDI velocity (full level); 1 = scale by velocity. */
    void setVelocitySensitivity (float sensitivity01) noexcept;

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

    void setSourceSettings (const SourceSettings& settings, double hostBpm) noexcept;

    void setPlaybackContext (AviatorKeyz::SoundType soundType,
                             const juce::String& category) noexcept;

    void setChopPlaybackState (const ChopPlaybackState& state) noexcept;

    int getPrimarySampleNumFrames() const noexcept;

    void noteOn (int midiNote, float velocity, bool reverse, float glideTimeMs) noexcept;
    void noteOff (int midiNote) noexcept;
    void allNotesOff() noexcept;
    void allSoundOff() noexcept;

    NoteGatePolicy getNoteGatePolicy() const noexcept { return noteGatePolicy; }
    RetriggerPolicy getRetriggerPolicy() const noexcept { return retriggerPolicy; }

    /** @deprecated Use getNoteGatePolicy() == TriggerToEnd */
    bool isOneShotPlayback() const noexcept;

    bool hasActiveVoices() const noexcept { return activeVoiceCount > 0; }
    int  getNumActiveVoices() const noexcept;

    /** Message thread: sanity-check snapshot, voices, and envelope after preset load. */
    bool validateCurrentState() const noexcept;

    /** sensitivity 0 = full level; 1 = linear velocity scaling. */
    static float calculateVelocityGain (float velocity, float sensitivity) noexcept;

    /** Unit tests: read increment of first active voice (0 if none). */
    float getActiveVoiceReadIncrementForTest() const noexcept;

    /** RT-safe snapshot of the most recent note-on pitch calculation (for tests / debug). */
    struct NotePitchDiag
    {
        int   midiNote { 0 };
        int   rootNote { 60 };
        bool  keytrack { false };
        int   playbackMode { 0 };
        float semitoneOffset { 0.f };
        float pitchRatio { 1.f };
        float sourceRateRatio { 1.f };
        float timeRatio { 1.f };
        float finalIncrement { 1.f };
        int   voiceIndex { -1 };
        uint32_t sequence { 0 };
    };

    NotePitchDiag getLastNotePitchDiag() const noexcept;

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
        GlideEngine glideEngine;
        EnvStage envStage = EnvStage::idle;
        float    envLevel = 0.f;
        float    envLinearStep = 0.f;
        int      envSegSamplesLeft = 0;
        float    declickGain = 1.f;
        float    declickStep = 0.f;

        const float* sampleData = nullptr;
        int          sampleNumFrames = 0;
        double       fileSampleRate = 44100.0;
        int          sampleRootNote = 60;
        PlaybackRates playbackRates {};
        // Future PhraseTimeStretch hook: use playbackRates.pitchSemitones with a
        // dedicated stretch engine — do NOT fold pitch into the sample read cursor.
        int          phraseStartFrame = 0;
        int          phraseEndFrame = 0;
        int          chopPitchOffsetSemis = 0;
        uint32_t     voiceInstanceId = 0;
    };

    void resetVoiceState (Voice& v, bool wasActive) noexcept;
    void chokeVoice (Voice& v) noexcept;
    void pushNoteVoice (int midiNote, int voiceIndex) noexcept;
    int  popNoteVoiceFifo (int midiNote) noexcept;
    void purgeNoteVoiceFromStack (int midiNote, int voiceIndex) noexcept;
    void clearAllNoteStacks() noexcept;
    void chokeSameNoteVoices (int midiNote) noexcept;
    void startVoice (Voice& v,
                     int midiNote,
                     float velocity,
                     bool reverse,
                     float glideTimeMs,
                     const SampleLibrary::AudioRegion* region,
                     bool declickFadeIn = false) noexcept;
    void enterRelease (Voice& v) noexcept;
    void finishAttack (Voice& v) noexcept;
    float renderVoiceSample (Voice& v) noexcept;
    void advanceEnvelope (Voice& v) noexcept;
    void advanceGlide (Voice& v) noexcept;
    void updateVoicePlaybackRates (Voice& v) noexcept;
    float voiceReadIncrement (const Voice& v) const noexcept;
    SamplePlaybackMode getEffectivePlaybackMode() const noexcept;
    bool usesPhraseWindow() const noexcept;
    void updatePlaybackPolicies() noexcept;
    int   findFreeOrStealVoice() noexcept;
    void chokeActiveVoicesForPhrase() noexcept;

    static float midiNoteToHz (float note) noexcept;

    static constexpr int kMaxVoices = 16;
    static constexpr int kMaxStackPerNote = 8;
    static constexpr float kChokeFadeMs = 5.f;
    static constexpr float kDeclickFadeMs = 1.5f;

    Voice    voices[kMaxVoices];
    int      noteVoiceStack[128][kMaxStackPerNote];
    uint8_t  noteVoiceStackCount[128] {};
    uint32_t nextVoiceInstanceId { 1 };
    double   sampleRate = 44100.0;

    const SampleLibrary::AudioSnapshot* sampleSnapshot = nullptr;

    float attackMs = 0.f;
    float decayMs = 0.f;
    float sustainLevel = 1.f;
    float releaseMs = 10.f;
    float velocitySensitivity { 0.f };

    int maxVoices { 16 };
    int activeVoiceCount { 0 };
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
    bool phraseKeySync { false };
    double phraseHostBpm { 120.0 };

    SourceSettings sourceSettings {};
    double sourceHostBpm { 120.0 };
    ChopPlaybackState chopState {};

    AviatorKeyz::SoundType currentSoundType { AviatorKeyz::SoundType::Phrase };
    juce::String currentCategory;
    NoteGatePolicy noteGatePolicy { NoteGatePolicy::Gated };
    RetriggerPolicy retriggerPolicy { RetriggerPolicy::Polyphonic };

    NotePitchDiag lastNotePitchDiag {};
    std::atomic<uint32_t> notePitchDiagSequence { 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SamplerEngine)
};
