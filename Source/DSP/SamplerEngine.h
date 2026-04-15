#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

// =============================================================================
//  SamplerEngine — polyphonic sample + sine fallback
//
//  Audio thread: process() performs no allocations, file I/O, or parsing.
//  Sample table pointer is installed from prepareToPlay / message-prep only.
// =============================================================================

class SamplerEngine
{
public:
    SamplerEngine();
    ~SamplerEngine();

    void prepare (const juce::dsp::ProcessSpec& spec);
    void releaseResources();

    // Message / preparation thread only — completes before audio runs.
    void setSampleTable (const float* monoSamples,
                         int numFrames,
                         int rootMidiNote) noexcept;

    void setEnvelopeTimesMs (float attackMs, float releaseMs) noexcept;

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
        sustain,
        release
    };

    struct Voice
    {
        bool     active = false;
        int      noteNumber = 0;
        float    velocity = 0.f;
        float    phase = 0.f; // sine
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
    };

    void startVoice (Voice& v, int midiNote, float velocity, bool reverse, float glideTimeMs) noexcept;
    void enterRelease (Voice& v) noexcept;
    float renderVoiceSample (Voice& v) noexcept;
    void advanceEnvelope (Voice& v) noexcept;
    void advanceGlide (Voice& v) noexcept;
    int   findFreeOrStealVoice() noexcept;

    static float midiNoteToHz (float note) noexcept;

    static constexpr int kMaxVoices = 16;

    Voice    voices[kMaxVoices];
    double   sampleRate = 44100.0;

    const float* sampleData = nullptr;
    int          sampleNumFrames = 0;
    int          sampleRootNote = 60;

    float attackMs = 5.f;
    float releaseMs = 150.f;

    int lastNoteForGlide = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SamplerEngine)
};
