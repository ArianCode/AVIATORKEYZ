#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>

// =============================================================================
//  SampleLibrary — M1 (load) / M3 (user import)
//
//  Responsibilities:
//    - Maintain the registry of loaded audio samples
//    - Load .wav and .aiff files from disk into memory
//    - Build SampleMap entries from loaded files (root note, note range, velocity range)
//    - Expose the current sample map for SamplerEngine to consume
//    - Handle user-imported samples (drag-and-drop or file picker, M3+)
//
//  Threading:
//    - All file I/O and buffer loading on the message thread only
//    - SamplerEngine reads a snapshot via double-buffer swap
//    - Audio thread never touches SampleLibrary directly
//
//  File format support (via juce::AudioFormatManager):
//    - WAV (.wav)      — primary format
//    - AIFF (.aif, .aiff) — secondary
//    - Other formats: rejected with user-visible error message
//
//  Error handling:
//    - Unsupported format: no crash — store an error string, notify UI
//    - Null reader (corrupted file): same — no crash
//
//  Implemented in M1 (load from factory map) + M3 (user import UI).
// =============================================================================

// One mapped region: a sample buffer assigned to a range of MIDI notes and velocities
struct SampleRegion
{
    juce::AudioBuffer<float> buffer;
    int   rootNote   { 60 };    // MIDI note at which this sample plays at unity pitch
    int   noteMin    { 0 };     // lowest MIDI note this region responds to
    int   noteMax    { 127 };   // highest MIDI note
    float velocityMin { 0.0f }; // 0.0–1.0
    float velocityMax { 1.0f };
    juce::String name;          // display name for UI
};

class SampleLibrary
{
public:
    SampleLibrary();
    ~SampleLibrary();

    // Load a single audio file into a SampleRegion and append to map.
    // Returns true on success, false on format/read error.
    // Call from message thread only.
    bool loadSample (const juce::File& file,
                     int   rootNote    = 60,
                     int   noteMin     = 0,
                     int   noteMax     = 127,
                     float velocityMin = 0.0f,
                     float velocityMax = 1.0f);

    // Clear all loaded samples
    void clearAll();

    // Get the current sample map (read-only).
    // SamplerEngine calls this once at prepareToPlay and on map-change notifications.
    const std::vector<SampleRegion>& getSampleMap() const noexcept { return sampleMap; }

    // Error from last load attempt (empty string = success)
    juce::String getLastError() const noexcept { return lastError; }

private:
    juce::AudioFormatManager       formatManager;
    std::vector<SampleRegion>      sampleMap;
    juce::String                   lastError;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SampleLibrary)
};
