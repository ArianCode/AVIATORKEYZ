#pragma once

#include <atomic>
#include <memory>
#include <vector>

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>

// =============================================================================
//  SampleLibrary — M1 (load) / M3 (user import)
//
//  Message thread: load files / embedded bytes, then publish().
//  Audio thread: read-only snapshot via getPublishedSnapshot() (no allocations).
// =============================================================================

struct SampleRegion
{
    juce::AudioBuffer<float> buffer;
    double fileSampleRate { 44100.0 };
    int   rootNote   { 60 };
    int   noteMin    { 0 };
    int   noteMax    { 127 };
    float velocityMin { 0.0f };
    float velocityMax { 1.0f };
    juce::String name;
};

class SampleLibrary
{
public:
    struct AudioRegion
    {
        const float* data       { nullptr };
        int          numFrames  { 0 };
        double       fileSampleRate { 44100.0 };
        int          rootNote   { 60 };
        int          noteMin    { 0 };
        int          noteMax    { 127 };
        float        velocityMin { 0.0f };
        float        velocityMax { 1.0f };
    };

    struct AudioSnapshot
    {
        std::vector<AudioRegion> regions;
    };

    SampleLibrary();
    ~SampleLibrary();

    bool loadFromMemory (const void* data,
                         size_t numBytes,
                         const juce::String& displayName,
                         int   rootNote    = 60,
                         int   noteMin     = 0,
                         int   noteMax     = 127,
                         float velocityMin = 0.0f,
                         float velocityMax = 1.0f);

    bool loadSample (const juce::File& file,
                     int   rootNote    = 60,
                     int   noteMin     = 0,
                     int   noteMax     = 127,
                     float velocityMin = 0.0f,
                     float velocityMax = 1.0f);

    void clearAll();

    /** Build mono snapshot and atomically publish (message thread). */
    void publish();

    /** Audio thread: current published map. Never nullptr (empty regions = sine fallback). */
    const AudioSnapshot* getPublishedSnapshot() const noexcept;

    /** Message / UI thread: first region of published snapshot for waveform display. */
    const float* getPrimaryWaveformData (int& numFramesOut) const noexcept;

    const std::vector<SampleRegion>& getPendingMap() const noexcept { return pendingMap; }

    juce::String getLastError() const noexcept { return lastError; }

    /** Peak-normalize to targetPeakDb (default −1 dBFS). Message thread only. */
    static bool normalizeSampleBuffer (juce::AudioBuffer<float>& sample,
                                       float targetPeakDb = -1.0f);

    static const AudioRegion* findRegionForNote (const AudioSnapshot& snapshot,
                                                 int midiNote,
                                                 float velocity) noexcept;

private:
    void buildSnapshotInto (int storageIndex);

    juce::AudioFormatManager formatManager;
    std::vector<SampleRegion> pendingMap;
    juce::String lastError;

    struct SnapshotStorage
    {
        std::vector<juce::HeapBlock<float>> monoBuffers;
        std::unique_ptr<AudioSnapshot>    snapshot;
    };

    SnapshotStorage storages[2];
    std::atomic<int> readIndex { 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SampleLibrary)
};
