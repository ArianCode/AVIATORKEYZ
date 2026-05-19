#include "SampleLibrary.h"

SampleLibrary::SampleLibrary()
{
    formatManager.registerBasicFormats();
    storages[0].snapshot = std::make_unique<AudioSnapshot>();
    storages[1].snapshot = std::make_unique<AudioSnapshot>();
}

SampleLibrary::~SampleLibrary() = default;

const SampleLibrary::AudioSnapshot* SampleLibrary::getPublishedSnapshot() const noexcept
{
    const int idx = readIndex.load (std::memory_order_acquire);
    return storages[idx].snapshot.get();
}

const float* SampleLibrary::getPrimaryWaveformData (int& numFramesOut) const noexcept
{
    numFramesOut = 0;
    const auto* snap = getPublishedSnapshot();
    if (snap == nullptr || snap->regions.empty())
        return nullptr;

    const auto& r = snap->regions.front();
    numFramesOut = r.numFrames;
    return r.data;
}

const SampleLibrary::AudioRegion* SampleLibrary::findRegionForNote (const AudioSnapshot& snapshot,
                                                                     int midiNote,
                                                                     float velocity) noexcept
{
    if (snapshot.regions.empty())
        return nullptr;

    const auto vel = juce::jlimit (0.0f, 1.0f, velocity);
    const SampleLibrary::AudioRegion* fallback = nullptr;

    for (const auto& region : snapshot.regions)
    {
        if (midiNote < region.noteMin || midiNote > region.noteMax)
            continue;

        if (vel < region.velocityMin || vel > region.velocityMax)
        {
            if (fallback == nullptr)
                fallback = &region;
            continue;
        }

        return &region;
    }

    if (fallback != nullptr)
        return fallback;

    return &snapshot.regions.front();
}

bool SampleLibrary::loadFromMemory (const void* data,
                                     size_t numBytes,
                                     const juce::String& displayName,
                                     int rootNote,
                                     int noteMin,
                                     int noteMax,
                                     float velocityMin,
                                     float velocityMax)
{
    lastError.clear();

    if (data == nullptr || numBytes == 0)
    {
        lastError = "Empty sample data.";
        return false;
    }

    auto stream = std::make_unique<juce::MemoryInputStream> (data, numBytes, false);
    std::unique_ptr<juce::AudioFormatReader> reader (
        formatManager.createReaderFor (std::move (stream)));

    if (reader == nullptr)
    {
        lastError = "Could not decode embedded sample: " + displayName;
        return false;
    }

    const int numChannels = static_cast<int> (reader->numChannels);
    const int numSamples  = static_cast<int> (reader->lengthInSamples);

    SampleRegion region;
    region.buffer.setSize (juce::jmax (1, juce::jmin (numChannels, 2)), numSamples);
    region.buffer.clear();

    if (! reader->read (&region.buffer, 0, numSamples, 0, true, true))
    {
        lastError = "Failed to read embedded sample: " + displayName;
        return false;
    }

    region.rootNote     = juce::jlimit (0, 127, rootNote);
    region.noteMin      = juce::jlimit (0, 127, noteMin);
    region.noteMax      = juce::jlimit (0, 127, noteMax);
    region.velocityMin  = juce::jlimit (0.0f, 1.0f, velocityMin);
    region.velocityMax  = juce::jlimit (0.0f, 1.0f, velocityMax);
    region.name         = displayName;

    pendingMap.push_back (std::move (region));
    return true;
}

bool SampleLibrary::loadSample (const juce::File& file,
                                 int   rootNote,
                                 int   noteMin,
                                 int   noteMax,
                                 float velocityMin,
                                 float velocityMax)
{
    lastError.clear();

    const auto ext = file.getFileExtension().toLowerCase();
    if (ext != ".wav" && ext != ".aif" && ext != ".aiff")
    {
        lastError = "Unsupported file format: " + ext
                    + ". Please use WAV or AIFF.";
        return false;
    }

    std::unique_ptr<juce::AudioFormatReader> reader (
        formatManager.createReaderFor (file));

    if (reader == nullptr)
    {
        lastError = "Could not open file: " + file.getFileName()
                    + ". The file may be corrupted or an unsupported format.";
        return false;
    }

    const int numChannels = static_cast<int> (reader->numChannels);
    const int numSamples  = static_cast<int> (reader->lengthInSamples);

    SampleRegion region;
    region.buffer.setSize (juce::jmax (1, juce::jmin (numChannels, 2)),
                           numSamples);
    region.buffer.clear();

    if (! reader->read (&region.buffer, 0, numSamples, 0, true, true))
    {
        lastError = "Failed to read audio data from: " + file.getFileName();
        return false;
    }

    region.rootNote     = juce::jlimit (0, 127, rootNote);
    region.noteMin      = juce::jlimit (0, 127, noteMin);
    region.noteMax      = juce::jlimit (0, 127, noteMax);
    region.velocityMin  = juce::jlimit (0.0f, 1.0f, velocityMin);
    region.velocityMax  = juce::jlimit (0.0f, 1.0f, velocityMax);
    region.name         = file.getFileNameWithoutExtension();

    pendingMap.push_back (std::move (region));
    return true;
}

void SampleLibrary::clearAll()
{
    pendingMap.clear();
    lastError.clear();
}

void SampleLibrary::buildSnapshotInto (const int storageIndex)
{
    auto& storage = storages[storageIndex];
    storage.monoBuffers.clear();
    storage.snapshot = std::make_unique<AudioSnapshot>();

    for (const auto& region : pendingMap)
    {
        const int numFrames = region.buffer.getNumSamples();
        if (numFrames <= 0)
            continue;

        const int numChannels = region.buffer.getNumChannels();
        juce::HeapBlock<float> mono;
        mono.malloc (static_cast<size_t> (numFrames));

        if (numChannels > 1)
        {
            auto* dst = mono.getData();
            for (int i = 0; i < numFrames; ++i)
                dst[i] = 0.5f * (region.buffer.getSample (0, i) + region.buffer.getSample (1, i));
        }
        else
        {
            juce::FloatVectorOperations::copy (mono.getData(),
                                               region.buffer.getReadPointer (0),
                                               numFrames);
        }

        storage.monoBuffers.push_back (std::move (mono));

        AudioRegion published;
        published.data        = storage.monoBuffers.back().getData();
        published.numFrames   = numFrames;
        published.rootNote    = region.rootNote;
        published.noteMin     = region.noteMin;
        published.noteMax     = region.noteMax;
        published.velocityMin = region.velocityMin;
        published.velocityMax = region.velocityMax;

        storage.snapshot->regions.push_back (published);
    }
}

void SampleLibrary::publish()
{
    const int writeIdx = 1 - readIndex.load (std::memory_order_relaxed);
    buildSnapshotInto (writeIdx);
    readIndex.store (writeIdx, std::memory_order_release);
}
