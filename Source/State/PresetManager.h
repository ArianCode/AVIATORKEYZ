#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "StateSchema.h"

// =============================================================================
//  PresetManager — M3
//
//  Responsibilities:
//    - Enumerate factory presets (bundled with plugin Resources/Presets/Factory/)
//    - Enumerate user presets (from user's Documents/AviatorKeyz/Presets/)
//    - Save current APVTS state as a named preset to disk
//    - Load a named preset from disk into APVTS
//    - Provide sorted, categorized preset lists to the UI
//
//  CompatibilityNote:
//    Preset XML format is versioned via "schemaVersion" attribute.
//    When STATE_SCHEMA_VERSION bumps, preset loader must migrate old files.
//    Factory presets are immutable — users cannot overwrite them.
//
//  Threading:
//    - All file I/O happens on the message thread
//    - APVTS::replaceState() is called from message thread (correct)
//    - Never called from audio thread
//
//  Implemented in M3.
// =============================================================================

class PresetManager
{
public:
    explicit PresetManager (juce::AudioProcessorValueTreeState& apvts);
    ~PresetManager();

    // Preset list
    juce::StringArray getPresetsForCategory (const juce::String& category) const;
    juce::StringArray getAllCategories() const;

    // Load/save
    bool loadPreset (const juce::String& category, const juce::String& name);
    bool saveUserPreset (const juce::String& category, const juce::String& name);

    // Current preset tracking (for header display)
    juce::String getCurrentPresetName() const;
    juce::String getCurrentCategory() const;

private:
    juce::AudioProcessorValueTreeState& apvts;
    juce::String currentPresetName;
    juce::String currentCategory;

    juce::File getFactoryPresetsDir() const;
    juce::File getUserPresetsDir()    const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetManager)
};
