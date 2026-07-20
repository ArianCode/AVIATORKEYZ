#pragma once

#include "../DSP/Performance/PerformanceTypes.h"
#include "../DSP/Performance/MacroMapper.h"
#include <juce_core/juce_core.h>

namespace MacroPresetParser
{
std::array<MacroControl, 4> parseFromPresetXml (const juce::XmlElement* presetRoot,
                                                  const juce::String& category);
}
