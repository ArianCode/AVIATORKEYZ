#include "MacroPresetParser.h"

namespace MacroPresetParser
{
std::array<MacroControl, 4> parseFromPresetXml (const juce::XmlElement* presetRoot,
                                                  const juce::String& category)
{
    std::array<MacroControl, 4> macros = MacroMapper::defaultsForCategory (category);

    if (presetRoot == nullptr)
        return macros;

    const auto* macrosEl = presetRoot->getChildByName ("Macros");
    if (macrosEl == nullptr)
        return macros;

    for (auto* macroEl : macrosEl->getChildIterator())
    {
        if (! macroEl->hasTagName ("Macro"))
            continue;

        const int idx = macroEl->getIntAttribute ("index", -1);
        if (idx < 0 || idx >= 4)
            continue;

        auto& mc = macros[static_cast<size_t> (idx)];
        mc.name = macroEl->getStringAttribute ("name", mc.name);
        mc.mappings.fill ({});

        int mapIdx = 0;
        for (auto* mapEl : macroEl->getChildIterator())
        {
            if (! mapEl->hasTagName ("Mapping") || mapIdx >= static_cast<int> (mc.mappings.size()))
                continue;

            MacroMapping m;
            m.destination = MacroMapper::destinationFromString (mapEl->getStringAttribute ("destination"));
            m.amount = static_cast<float> (mapEl->getDoubleAttribute ("amount", 0.0));
            m.curve = static_cast<float> (mapEl->getDoubleAttribute ("curve", 1.0));
            mc.mappings[static_cast<size_t> (mapIdx++)] = m;
        }
    }

    return macros;
}
} // namespace MacroPresetParser
