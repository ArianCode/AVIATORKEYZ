#include "PresetManager.h"
#include "FactoryResources.h"

PresetManager::PresetManager (juce::AudioProcessorValueTreeState& apvtsRef)
    : apvts (apvtsRef)
    , currentPresetName ("Init")
    , currentCategory (AviatorKeyz::Category::LEADS)
{
}

PresetManager::~PresetManager() = default;

juce::StringArray PresetManager::getAllCategories() const
{
    return {
        AviatorKeyz::Category::LEADS,
        AviatorKeyz::Category::BRASS,
        AviatorKeyz::Category::ENSEMBLES,
        AviatorKeyz::Category::STRINGS,
        AviatorKeyz::Category::PADS,
        AviatorKeyz::Category::CHORDS,
        AviatorKeyz::Category::SYNTHS,
        AviatorKeyz::Category::ARPS,
        AviatorKeyz::Category::VOCALS,
        AviatorKeyz::Category::BELLS
    };
}

juce::StringArray PresetManager::getPresetsForCategory (const juce::String& category) const
{
    auto names = FactoryResources::getPresetNamesForCategory (category);

    const auto dir = getUserPresetsDir().getChildFile (category);
    if (dir.isDirectory())
    {
        juce::Array<juce::File> files;
        dir.findChildFiles (files, juce::File::findFiles, false, "*.xml");
        for (const auto& f : files)
        {
            const auto n = f.getFileNameWithoutExtension();
            if (! names.contains (n, true))
                names.add (n);
        }
    }

    return names;
}

bool PresetManager::loadPreset (const juce::String& category, const juce::String& name)
{
    std::unique_ptr<juce::XmlElement> parsed;
    juce::String sampleId = AviatorKeyz::SampleID::DEFAULT;

    if (const auto* factory = [&]() -> const FactoryResources::PresetEntry* {
            for (const auto& e : FactoryResources::getFactoryPresets())
            {
                if (e.category.equalsIgnoreCase (category) && e.name.equalsIgnoreCase (name))
                    return &e;
            }
            return nullptr;
        }())
    {
        parsed = juce::XmlDocument::parse (juce::String::fromUTF8 (factory->xmlData, factory->xmlSize));
        sampleId = factory->sampleId;
    }
    else
    {
        const auto file = getUserPresetsDir().getChildFile (category).getChildFile (name + ".xml");
        if (! file.existsAsFile())
            return false;
        parsed = juce::XmlDocument::parse (file);
        if (parsed != nullptr && parsed->hasTagName ("Preset"))
            sampleId = parsed->getStringAttribute (AviatorKeyz::PresetKey::SAMPLE_ID,
                                                 AviatorKeyz::SampleID::DEFAULT);
    }

    if (parsed == nullptr)
        return false;

    juce::XmlElement* stateEl = nullptr;

    if (parsed->hasTagName ("Preset"))
    {
        if (sampleId.isEmpty())
            sampleId = parsed->getStringAttribute (AviatorKeyz::PresetKey::SAMPLE_ID,
                                                   AviatorKeyz::SampleID::DEFAULT);
        stateEl = parsed->getChildByName ("AviatorKeyzState");
    }
    else
        stateEl = parsed->getChildByName ("AviatorKeyzState");

    if (stateEl == nullptr && parsed->hasTagName ("AviatorKeyzState"))
        stateEl = parsed.get();

    if (stateEl == nullptr)
        return false;

    auto state = juce::ValueTree::fromXml (*stateEl);

    if (! state.isValid())
        return false;

    apvts.replaceState (state);
    currentCategory = category;
    currentPresetName = name;
    currentSampleId = sampleId;

    if (onPresetLoaded)
        onPresetLoaded (category, name, sampleId);

    return true;
}

bool PresetManager::saveUserPreset (const juce::String& category, const juce::String& name)
{
    auto dir = getUserPresetsDir().getChildFile (category);
    if (! dir.createDirectory())
        return false;

    auto state = apvts.copyState();
    state.setProperty ("stateVersion", AviatorKeyz::STATE_SCHEMA_VERSION, nullptr);

    juce::XmlElement preset ("Preset");
    preset.setAttribute (AviatorKeyz::PresetKey::CATEGORY, category);
    preset.setAttribute (AviatorKeyz::PresetKey::NAME, name);
    preset.setAttribute (AviatorKeyz::PresetKey::SCHEMA_VER, AviatorKeyz::STATE_SCHEMA_VERSION);
    preset.setAttribute (AviatorKeyz::PresetKey::SAMPLE_ID,
                          currentSampleId.isNotEmpty() ? currentSampleId
                                                       : juce::String (AviatorKeyz::SampleID::DEFAULT));

    if (auto inner = state.createXml())
        preset.addChildElement (inner.release());

    const auto file = dir.getChildFile (name + ".xml");
    return preset.writeTo (file);
}

juce::String PresetManager::getCurrentPresetName() const { return currentPresetName; }
juce::String PresetManager::getCurrentCategory() const { return currentCategory; }
juce::String PresetManager::getCurrentSampleId() const { return currentSampleId; }

juce::File PresetManager::getFactoryPresetsDir() const
{
    return {};
}

juce::File PresetManager::getUserPresetsDir() const
{
    return juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
        .getChildFile ("AviatorKeyz/Presets");
}
