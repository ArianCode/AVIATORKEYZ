#pragma once

#include <functional>

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
    juce::String getCurrentSampleId() const;
    int          getCurrentRootNote() const;

    /** Update preset/sample tracking without reloading preset XML (host state restore). */
    void setPresetIdentity (const juce::String& category,
                            const juce::String& name,
                            const juce::String& sampleId,
                            int rootNote);

    int  getTotalPresetCount() const;
    int  getCurrentPresetIndex() const;
    bool loadPresetByFlatIndex (int index);
    bool loadAdjacentPreset (int delta);
    /** Steps within the active category only (PATCH prev/next). */
    bool loadAdjacentPresetInCategory (int delta);
    int  getCurrentPresetIndexInCategory() const;
    /** Steps across factory categories (CAT prev/next). */
    bool loadAdjacentCategory (int delta);

    /** category, preset name, sampleId, rootNote — message thread only */
    std::function<void (const juce::String& category,
                        const juce::String& name,
                        const juce::String& sampleId,
                        int rootNote)> onPresetLoaded;

private:
    static int inferRootNoteFromPresetName (const juce::String& presetName,
                                            const juce::String& sampleId);
    static int parseRootNoteAttribute (const juce::XmlElement* presetRoot);

    struct FlatPreset
    {
        juce::String category;
        juce::String name;
    };

    juce::Array<FlatPreset> buildFlatPresetList() const;
    int findCurrentFlatIndex (const juce::Array<FlatPreset>& list) const;

    juce::AudioProcessorValueTreeState& apvts;
    juce::String currentPresetName;
    juce::String currentCategory;
    juce::String currentSampleId { AviatorKeyz::SampleID::DEFAULT };
    int          currentRootNote { 60 };

    juce::File getFactoryPresetsDir() const;
    juce::File getUserPresetsDir()    const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetManager)
};
