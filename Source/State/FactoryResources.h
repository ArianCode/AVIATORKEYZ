#pragma once

#include <juce_core/juce_core.h>

// =============================================================================
//  FactoryResources — embedded factory presets and WAV lookup (BinaryData)
// =============================================================================

namespace FactoryResources
{
    struct PresetEntry
    {
        juce::String category;
        juce::String name;
        juce::String sampleId;
        const char*  xmlData { nullptr };
        int          xmlSize { 0 };
    };

    /** All factory presets parsed from embedded XML (built once, message thread). */
    const juce::Array<PresetEntry>& getFactoryPresets();

    juce::StringArray getPresetNamesForCategory (const juce::String& category);

  /** Load embedded WAV by sampleId into mono buffer. Returns false if not found. */
    bool loadEmbeddedSampleMono (const juce::String& sampleId,
                                 juce::HeapBlock<float>& monoOut,
                                 int& numFramesOut,
                                 int defaultRootNote = 60);

    juce::String sampleIdForPreset (const juce::String& category, const juce::String& name);

} // namespace FactoryResources
