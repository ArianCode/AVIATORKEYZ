#include "SampleLibrary.h"

SampleLibrary::SampleLibrary()
{
    // Register supported formats
    formatManager.registerBasicFormats(); // WAV, AIFF, and others if available
}

SampleLibrary::~SampleLibrary() = default;

bool SampleLibrary::loadSample (const juce::File& file,
                                 int   rootNote,
                                 int   noteMin,
                                 int   noteMax,
                                 float velocityMin,
                                 float velocityMax)
{
    lastError.clear();

    // --- Validate extension ---
    const auto ext = file.getFileExtension().toLowerCase();
    if (ext != ".wav" && ext != ".aif" && ext != ".aiff")
    {
        lastError = "Unsupported file format: " + ext
                    + ". Please use WAV or AIFF.";
        return false;
    }

    // --- Open reader ---
    std::unique_ptr<juce::AudioFormatReader> reader (
        formatManager.createReaderFor (file));

    if (reader == nullptr)
    {
        lastError = "Could not open file: " + file.getFileName()
                    + ". The file may be corrupted or an unsupported format.";
        return false;
    }

    // --- Load into buffer ---
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

    region.rootNote    = juce::jlimit (0, 127, rootNote);
    region.noteMin     = juce::jlimit (0, 127, noteMin);
    region.noteMax     = juce::jlimit (0, 127, noteMax);
    region.velocityMin = juce::jlimit (0.0f, 1.0f, velocityMin);
    region.velocityMax = juce::jlimit (0.0f, 1.0f, velocityMax);
    region.name        = file.getFileNameWithoutExtension();

    sampleMap.push_back (std::move (region));
    return true;
}

void SampleLibrary::clearAll()
{
    sampleMap.clear();
    lastError.clear();
}
