#pragma once

#include <array>
#include <functional>

#include <juce_audio_processors/juce_audio_processors.h>
#include "CategorySoundPolicy.h"
#include "StateSchema.h"
#include "../DSP/Performance/PerformanceTypes.h"

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
    float        getCurrentOriginalBpm() const noexcept { return currentOriginalBpm; }
    AviatorKeyz::SoundType getCurrentSoundType() const noexcept { return currentSoundType; }

    /** Update preset/sample tracking without reloading preset XML (host state restore). */
    void setPresetIdentity (const juce::String& category,
                            const juce::String& name,
                            const juce::String& sampleId,
                            int rootNote,
                            AviatorKeyz::SoundType soundType = AviatorKeyz::SoundType::Phrase,
                            float originalBpm = 120.f);

    /** Keep root note in sync after sample load resolves smpl vs preset. */
    void setCurrentRootNote (int rootNote) noexcept;

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

    /** Parsed macro mappings from preset XML — message thread only */
    std::function<void (const std::array<MacroControl, 4>& macros)> onMacroMapsLoaded;

private:
    static int inferRootNoteFromPresetName (const juce::String& presetName,
                                            const juce::String& sampleId);
    static int parseRootNoteAttribute (const juce::XmlElement* presetRoot);
    static float parseOriginalBpmAttribute (const juce::XmlElement* presetRoot);

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
    float        currentOriginalBpm { 120.f };
    AviatorKeyz::SoundType currentSoundType { AviatorKeyz::SoundType::Phrase };

    juce::File getFactoryPresetsDir() const;
    juce::File getUserPresetsDir()    const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetManager)
};
