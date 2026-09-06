#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

class SamplerEngine;
class SynthEngine;

// =============================================================================
//  MidiHandler — audio-thread MIDI routing (no allocations).
// =============================================================================

class MidiHandler
{
public:
    MidiHandler() = default;
    ~MidiHandler() = default;

    /** Dispatches every message in the buffer (block-quantised) and clears it.
        forceSynth routes note events to the synth regardless of sourceBlend
        (LAYER MIX osc layers). */
    void process (juce::MidiBuffer& midiBuffer,
                  SamplerEngine& samplerEngine,
                  SynthEngine& synthEngine,
                  bool reverse,
                  float glideTimeMs,
                  float sourceBlend,
                  bool forceSynth = false);

    /** Dispatches one message — used by the processor's sample-accurate
        segment renderer (arpeggiator / flip lever timeline). */
    void handleMessage (const juce::MidiMessage& message,
                        SamplerEngine& samplerEngine,
                        SynthEngine& synthEngine,
                        bool reverse,
                        float glideTimeMs,
                        float sourceBlend,
                        bool forceSynth = false);

    /** Bypass path: update sustain/key-held state only; do not start or stop voices. */
    void processBypassed (juce::MidiBuffer& midiBuffer);

    void setMidiChannel (int channel) noexcept { midiChannel = channel; }

    // Performance controllers tracked for the MFX ASSIGN sources (audio thread).
    float getModWheel() const noexcept   { return modWheel; }
    float getAftertouch() const noexcept { return aftertouch; }
    float getLastVelocity() const noexcept { return lastVelocity; }
    int   getLastNote() const noexcept   { return lastNote; }

private:
    int midiChannel { 0 };
    bool sustainPedal { false };
    bool keyHeld[128] {};
    float modWheel { 0.f };
    float aftertouch { 0.f };
    float lastVelocity { 0.f };
    int   lastNote { 60 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiHandler)
};
