#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <array>

class SamplerEngine;

// =============================================================================
//  MidiHandler — audio-thread MIDI routing (no allocations).
// =============================================================================

class MidiHandler
{
public:
    MidiHandler() = default;
    ~MidiHandler() = default;

    void process (juce::MidiBuffer& midiBuffer,
                  SamplerEngine& samplerEngine,
                  bool reverse,
                  float glideTimeMs);

    /** Bypass path: update sustain/key-held state only; do not start or stop voices. */
    void processBypassed (juce::MidiBuffer& midiBuffer);

    void setMidiChannel (int channel) noexcept { midiChannel = channel; }

private:
    int midiChannel { 0 };
    bool sustainPedal { false };
    std::array<bool, 128> keyHeld {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiHandler)
};
