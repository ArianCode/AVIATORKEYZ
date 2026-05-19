#include "FactoryResources.h"
#include "StateSchema.h"

#include <BinaryData.h>
#include <juce_audio_formats/juce_audio_formats.h>

namespace
{
juce::Array<FactoryResources::PresetEntry> buildPresetIndex()
{
    juce::Array<FactoryResources::PresetEntry> entries;

    for (int i = 0; i < AviatorKeyzBinary::namedResourceListSize; ++i)
    {
        const juce::String orig (AviatorKeyzBinary::originalFilenames[i]);
        if (! orig.endsWithIgnoreCase (".xml"))
            continue;
        if (! orig.containsIgnoreCase ("Presets/Factory"))
            continue;

        const char* resName = AviatorKeyzBinary::namedResourceList[i];
        int size = 0;
        const char* data = AviatorKeyzBinary::getNamedResource (resName, size);
        if (data == nullptr || size <= 0)
            continue;

        const auto xmlText = juce::String::fromUTF8 (data, size);
        const auto parsed = juce::XmlDocument::parse (xmlText);
        if (parsed == nullptr || ! parsed->hasTagName ("Preset"))
            continue;

        FactoryResources::PresetEntry e;
        e.category = parsed->getStringAttribute (AviatorKeyz::PresetKey::CATEGORY);
        e.name     = parsed->getStringAttribute (AviatorKeyz::PresetKey::NAME);
        e.sampleId = parsed->getStringAttribute (AviatorKeyz::PresetKey::SAMPLE_ID,
                                                 AviatorKeyz::SampleID::DEFAULT);
        e.xmlData  = data;
        e.xmlSize  = size;
        entries.add (std::move (e));
    }

    struct PresetComparator
    {
        static int compareElements (const FactoryResources::PresetEntry& a,
                                    const FactoryResources::PresetEntry& b)
        {
            const int c = a.category.compareIgnoreCase (b.category);
            if (c != 0) return c;
            return a.name.compareIgnoreCase (b.name);
        }
    };
    PresetComparator cmp;
    entries.sort (cmp);

    return entries;
}

const juce::Array<FactoryResources::PresetEntry>& presetIndex()
{
    static juce::Array<FactoryResources::PresetEntry> cached = buildPresetIndex();
    return cached;
}

const FactoryResources::PresetEntry* findPreset (const juce::String& category,
                                               const juce::String& name)
{
    for (const auto& e : presetIndex())
    {
        if (e.category.equalsIgnoreCase (category) && e.name.equalsIgnoreCase (name))
            return &e;
    }
    return nullptr;
}

const char* findWavResource (const juce::String& sampleId, int& sizeOut)
{
    const auto needle = sampleId + ".wav";
    for (int i = 0; i < AviatorKeyzBinary::namedResourceListSize; ++i)
    {
        const juce::String orig (AviatorKeyzBinary::originalFilenames[i]);
        if (! orig.endsWithIgnoreCase (needle))
            continue;
        return AviatorKeyzBinary::getNamedResource (AviatorKeyzBinary::namedResourceList[i], sizeOut);
    }
    return nullptr;
}
} // namespace

const juce::Array<FactoryResources::PresetEntry>& FactoryResources::getFactoryPresets()
{
    return presetIndex();
}

juce::StringArray FactoryResources::getPresetNamesForCategory (const juce::String& category)
{
    juce::StringArray names;
    for (const auto& e : presetIndex())
    {
        if (e.category.equalsIgnoreCase (category))
            names.add (e.name);
    }
    return names;
}

juce::String FactoryResources::sampleIdForPreset (const juce::String& category,
                                                const juce::String& name)
{
    if (const auto* e = findPreset (category, name))
        return e->sampleId;
    return AviatorKeyz::SampleID::DEFAULT;
}

const void* FactoryResources::getEmbeddedWavData (const juce::String& sampleId, int& numBytesOut)
{
    numBytesOut = 0;
    if (const char* data = findWavResource (sampleId, numBytesOut))
        return data;

    return findWavResource (AviatorKeyz::SampleID::DEFAULT, numBytesOut);
}

bool FactoryResources::loadEmbeddedSampleMono (const juce::String& sampleId,
                                               juce::HeapBlock<float>& monoOut,
                                               int& numFramesOut,
                                               int defaultRootNote)
{
    juce::ignoreUnused (defaultRootNote);

    int size = 0;
    const char* data = findWavResource (sampleId, size);
    if (data == nullptr || size <= 0)
        data = findWavResource (AviatorKeyz::SampleID::DEFAULT, size);
    if (data == nullptr || size <= 0)
        return false;

    juce::AudioFormatManager fm;
    fm.registerBasicFormats();
    auto stream = std::make_unique<juce::MemoryInputStream> (data, static_cast<size_t> (size), false);

    if (auto reader = std::unique_ptr<juce::AudioFormatReader> (fm.createReaderFor (std::move (stream))))
    {
        numFramesOut = static_cast<int> (reader->lengthInSamples);
        const int ch = juce::jmax (1, static_cast<int> (reader->numChannels));
        juce::AudioBuffer<float> tmp (ch, numFramesOut);
        reader->read (&tmp, 0, numFramesOut, 0, true, true);

        monoOut.malloc (static_cast<size_t> (numFramesOut));
        if (ch > 1)
        {
            auto* d = monoOut.getData();
            for (int i = 0; i < numFramesOut; ++i)
                d[i] = 0.5f * (tmp.getSample (0, i) + tmp.getSample (1, i));
        }
        else
        {
            juce::FloatVectorOperations::copy (monoOut.getData(), tmp.getReadPointer (0), numFramesOut);
        }
        return numFramesOut > 0;
    }

    return false;
}
