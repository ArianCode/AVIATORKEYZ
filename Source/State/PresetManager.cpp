#include "PresetManager.h"

// =============================================================================
//  PresetManager — stub implementation (M0)
//  Full implementation in M3.
// =============================================================================

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

juce::StringArray PresetManager::getPresetsForCategory (const juce::String& /*category*/) const
{
    // M3: scan factory + user preset dirs and return names
    return {};
}

bool PresetManager::loadPreset (const juce::String& category, const juce::String& name)
{
    // M3: load XML from disk, migrate if old version, call apvts.replaceState()
    juce::ignoreUnused (category, name);
    return false;
}

bool PresetManager::saveUserPreset (const juce::String& category, const juce::String& name)
{
    // M3: serialize apvts state to XML, write to user presets dir
    juce::ignoreUnused (category, name);
    return false;
}

juce::String PresetManager::getCurrentPresetName() const { return currentPresetName; }
juce::String PresetManager::getCurrentCategory()   const { return currentCategory; }

juce::File PresetManager::getFactoryPresetsDir() const
{
    // Factory presets are bundled inside the plugin binary resources (M3)
    return {};
}

juce::File PresetManager::getUserPresetsDir() const
{
    return juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
               .getChildFile ("AviatorKeyz/Presets");
}
