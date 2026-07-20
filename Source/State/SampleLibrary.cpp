#include "SampleLibrary.h"
#include "Debug/AviatorDebug.h"
#include <cstring>

// ---------------------------------------------------------------------------
// Read MIDI unity note from a WAV smpl chunk, if present.
// Returns -1 when no smpl chunk is found (caller uses preset XML value instead).
//
// smpl chunk layout (IEEE 1666 / MMA spec):
//   bytes  0- 3: manufacturer
//   bytes  4- 7: product
//   bytes  8-11: sample period (ns)
//   bytes 12-15: MIDI unity note  ← what we want
//   bytes 16-19: MIDI pitch fraction
//   ...
// ---------------------------------------------------------------------------
static int readSmplRootNote (const void* wavData, size_t numBytes) noexcept
{
    const auto* p   = static_cast<const uint8_t*> (wavData);
    const auto* end = p + numBytes;

    // Skip RIFF header (12 bytes) and scan for 'smpl' chunk tag
    if (numBytes < 12) return -1;
    p += 12;

    while (p + 8 <= end)
    {
        uint32_t tag, chunkSize;
        std::memcpy (&tag,       p,     4);
        std::memcpy (&chunkSize, p + 4, 4);
        // Little-endian 'smpl' = 0x736d706c
        if (tag == 0x6c706d73u)   // 's','m','p','l' in little-endian memory
        {
            const auto* chunkBody = p + 8;
            if (chunkBody + 16 <= end)
            {
                uint32_t midiUnity;
                std::memcpy (&midiUnity, chunkBody + 12, 4);
                if (midiUnity <= 127)
                {
                    AK_LOG ("SampleLibrary: smpl chunk root note = " + juce::String (midiUnity));
                    return static_cast<int> (midiUnity);
                }
            }
            break;  // found smpl but malformed — stop searching
        }
        const uint32_t advance = 8 + ((chunkSize + 1) & ~1u);  // align to 2 bytes
        if (advance == 0) break;
        p += advance;
    }
    return -1;
}

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

int SampleLibrary::getPrimaryRootNote() const noexcept
{
    const auto* snap = getPublishedSnapshot();
    if (snap != nullptr && ! snap->regions.empty())
        return snap->regions.front().rootNote;

    if (! pendingMap.empty())
        return pendingMap.front().rootNote;

    return 60;
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

bool SampleLibrary::normalizeSampleBuffer (juce::AudioBuffer<float>& sample,
                                           float targetPeakDb)
{
    if (sample.getNumSamples() == 0 || sample.getNumChannels() == 0)
        return false;

    float peak = 0.0f;

    for (int channel = 0; channel < sample.getNumChannels(); ++channel)
        peak = std::max (peak, sample.getMagnitude (channel, 0, sample.getNumSamples()));

    constexpr float silenceThreshold = 1.0e-7f;

    if (! std::isfinite (peak) || peak <= silenceThreshold)
        return false;

    const float targetPeak = juce::Decibels::decibelsToGain (targetPeakDb);
    sample.applyGain (targetPeak / peak);
    return true;
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

    if (! normalizeSampleBuffer (region.buffer, -1.0f))
    {
        AK_LOG ("Sample normalization failed: empty or silent sample — " + displayName);
        lastError = "Sample is empty or silent: " + displayName;
        return false;
    }

    // Prefer the smpl chunk root note when the WAV contains one —
    // it is authoritative over the preset XML inference.
    const int smplRoot = readSmplRootNote (data, numBytes);
    const int effectiveRoot = (smplRoot >= 0) ? smplRoot : rootNote;

    if (smplRoot >= 0 && smplRoot != rootNote)
        AK_LOG ("SampleLibrary: smpl chunk (" + juce::String (smplRoot)
                + ") overrides preset XML rootNote (" + juce::String (rootNote)
                + ") for " + displayName);

    region.rootNote     = juce::jlimit (0, 127, effectiveRoot);
    region.fileSampleRate = reader->sampleRate;
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

    if (! normalizeSampleBuffer (region.buffer, -1.0f))
    {
        AK_LOG ("Sample normalization failed: empty or silent sample — " + file.getFileName());
        lastError = "Sample is empty or silent: " + file.getFileName();
        return false;
    }

    region.rootNote     = juce::jlimit (0, 127, rootNote);
    region.fileSampleRate = reader->sampleRate;
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
        published.fileSampleRate = region.fileSampleRate;
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
    AK_LOG ("SampleLibrary::publish idx=" + juce::String (writeIdx)
            + " regions=" + juce::String (storages[writeIdx].snapshot->regions.size()));
}
