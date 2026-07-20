#pragma once

#include "PerformanceTypes.h"
#include "PerformanceApvtsReader.h"
#include <juce_audio_processors/juce_audio_processors.h>

class MacroMapper
{
public:
    static MacroDestination destinationFromString (const juce::String& name) noexcept;
    static juce::String destinationToString (MacroDestination dest) noexcept;

    static std::array<MacroControl, 4> defaultsForCategory (const juce::String& category);

    static EngineState applyMacros (const EngineState& base,
                                    const std::array<MacroControl, 4>& macros,
                                    const PerformanceApvtsReader::ParamCache& cache) noexcept;
};
