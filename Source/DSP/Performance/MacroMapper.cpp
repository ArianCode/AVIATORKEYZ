#include "MacroMapper.h"
#include "PerformanceApvtsReader.h"
#include <cmath>

namespace
{
float applyCurve (float value01, float curve) noexcept
{
    value01 = juce::jlimit (0.f, 1.f, value01);
    curve = juce::jmax (0.01f, curve);
    return std::pow (value01, curve);
}

void addToField (EngineState& s, MacroDestination dest, float delta) noexcept
{
    switch (dest)
    {
        case MacroDestination::SampleStart:   s.source.start = juce::jlimit (0.f, 0.99f, s.source.start + delta * 0.25f); break;
        case MacroDestination::SampleEnd:     s.source.end = juce::jlimit (0.01f, 1.f, s.source.end + delta * 0.25f); break;
        case MacroDestination::Speed:         s.source.speed = juce::jlimit (0.25f, 4.f, s.source.speed + delta * 2.f); break;
        case MacroDestination::Pitch:         s.source.tune = juce::jlimit (-24.f, 24.f, s.source.tune + delta * 12.f); break;
        case MacroDestination::ChopAmount:    s.chop.amount = juce::jlimit (0.f, 1.f, s.chop.amount + delta); break;
        case MacroDestination::GateAmount:    s.chop.gate = juce::jlimit (0.f, 1.f, s.chop.gate + delta); break;
        case MacroDestination::Swing:         s.chop.swing = juce::jlimit (0.f, 1.f, s.chop.swing + delta); break;
        case MacroDestination::RandomAmount:  s.chop.random = juce::jlimit (0.f, 1.f, s.chop.random + delta); break;
        case MacroDestination::ReverseChance: s.chop.reverseChance = juce::jlimit (0.f, 1.f, s.chop.reverseChance + delta); break;
        case MacroDestination::TextureMix:    s.texture.mix = juce::jlimit (0.f, 1.f, s.texture.mix + delta); break;
        case MacroDestination::GrainSize:     s.texture.grainSize = juce::jlimit (0.f, 1.f, s.texture.grainSize + delta); break;
        case MacroDestination::Density:       s.texture.density = juce::jlimit (0.f, 1.f, s.texture.density + delta); break;
        case MacroDestination::TexturePosition: s.texture.position = juce::jlimit (0.f, 1.f, s.texture.position + delta); break;
        case MacroDestination::PitchSpread:   s.texture.pitchSpread = juce::jlimit (0.f, 1.f, s.texture.pitchSpread + delta); break;
        case MacroDestination::Smear:         s.smearOffset += delta; break;
        case MacroDestination::Width:         s.widthOffset += delta; break;
        case MacroDestination::FilterTone:    s.toneOffset += delta; break;
        case MacroDestination::Drive:         s.driveOffset += delta; break;
        case MacroDestination::DelayMix:      s.delayMixOffset += delta; break;
        case MacroDestination::ReverbMix:     s.reverbMixOffset += delta; break;
        case MacroDestination::OutputLevel:   s.outputLevelOffset += delta; break;
        case MacroDestination::None:
        default: break;
    }
}

MacroMapping makeMap (MacroDestination d, float amt, float curve = 1.f)
{
    MacroMapping m;
    m.destination = d;
    m.amount = amt;
    m.curve = curve;
    return m;
}

void setMacro (std::array<MacroControl, 4>& arr, int idx, const char* name,
               std::initializer_list<MacroMapping> maps)
{
    auto& mc = arr[static_cast<size_t> (idx)];
    mc.name = name;
    mc.mappings.fill ({});
    size_t i = 0;
    for (const auto& m : maps)
    {
        if (i < mc.mappings.size())
            mc.mappings[i++] = m;
    }
}
} // namespace

MacroDestination MacroMapper::destinationFromString (const juce::String& name) noexcept
{
    const auto n = name.trim();
    if (n.equalsIgnoreCase ("ChopAmount"))     return MacroDestination::ChopAmount;
    if (n.equalsIgnoreCase ("GateAmount"))     return MacroDestination::GateAmount;
    if (n.equalsIgnoreCase ("ReverseChance"))  return MacroDestination::ReverseChance;
    if (n.equalsIgnoreCase ("TextureMix"))     return MacroDestination::TextureMix;
    if (n.equalsIgnoreCase ("GrainSize"))      return MacroDestination::GrainSize;
    if (n.equalsIgnoreCase ("Density"))        return MacroDestination::Density;
    if (n.equalsIgnoreCase ("TexturePosition")) return MacroDestination::TexturePosition;
    if (n.equalsIgnoreCase ("PitchSpread"))    return MacroDestination::PitchSpread;
    if (n.equalsIgnoreCase ("Smear"))          return MacroDestination::Smear;
    if (n.equalsIgnoreCase ("Width"))            return MacroDestination::Width;
    if (n.equalsIgnoreCase ("FilterTone"))     return MacroDestination::FilterTone;
    if (n.equalsIgnoreCase ("Drive"))          return MacroDestination::Drive;
    if (n.equalsIgnoreCase ("DelayMix"))       return MacroDestination::DelayMix;
    if (n.equalsIgnoreCase ("ReverbMix"))      return MacroDestination::ReverbMix;
    if (n.equalsIgnoreCase ("Speed"))          return MacroDestination::Speed;
    if (n.equalsIgnoreCase ("Pitch"))          return MacroDestination::Pitch;
    if (n.equalsIgnoreCase ("Swing"))          return MacroDestination::Swing;
    if (n.equalsIgnoreCase ("RandomAmount"))   return MacroDestination::RandomAmount;
    if (n.equalsIgnoreCase ("SampleStart"))    return MacroDestination::SampleStart;
    if (n.equalsIgnoreCase ("SampleEnd"))      return MacroDestination::SampleEnd;
    if (n.equalsIgnoreCase ("OutputLevel"))    return MacroDestination::OutputLevel;
    return MacroDestination::None;
}

juce::String MacroMapper::destinationToString (MacroDestination dest) noexcept
{
    switch (dest)
    {
        case MacroDestination::ChopAmount: return "ChopAmount";
        case MacroDestination::GateAmount: return "GateAmount";
        case MacroDestination::TextureMix: return "TextureMix";
        case MacroDestination::GrainSize: return "GrainSize";
        case MacroDestination::Density: return "Density";
        case MacroDestination::ReverbMix: return "ReverbMix";
        case MacroDestination::FilterTone: return "FilterTone";
        case MacroDestination::Width: return "Width";
        default: return {};
    }
}

std::array<MacroControl, 4> MacroMapper::defaultsForCategory (const juce::String& category)
{
    std::array<MacroControl, 4> macros {};
    const auto cat = category.toLowerCase();

    if (cat.contains ("vocal"))
    {
        setMacro (macros, 0, "Chop", { makeMap (MacroDestination::ChopAmount, 0.8f), makeMap (MacroDestination::GateAmount, 0.45f), makeMap (MacroDestination::ReverseChance, 0.25f, 1.2f) });
        setMacro (macros, 1, "Texture", { makeMap (MacroDestination::TextureMix, 0.75f), makeMap (MacroDestination::GrainSize, -0.3f), makeMap (MacroDestination::Density, 0.6f) });
        setMacro (macros, 2, "Space", { makeMap (MacroDestination::ReverbMix, 0.7f), makeMap (MacroDestination::DelayMix, 0.4f), makeMap (MacroDestination::Width, 0.5f) });
        setMacro (macros, 3, "Tone", { makeMap (MacroDestination::FilterTone, 0.5f), makeMap (MacroDestination::PitchSpread, 0.35f) });
    }
    else if (cat.contains ("ensemble") || cat.contains ("guitar"))
    {
        setMacro (macros, 0, "Drive", { makeMap (MacroDestination::Drive, 0.6f), makeMap (MacroDestination::FilterTone, 0.3f) });
        setMacro (macros, 1, "Chop", { makeMap (MacroDestination::ChopAmount, 0.75f), makeMap (MacroDestination::GateAmount, 0.5f) });
        setMacro (macros, 2, "Width", { makeMap (MacroDestination::Width, 0.6f), makeMap (MacroDestination::PitchSpread, 0.25f) });
        setMacro (macros, 3, "Ambience", { makeMap (MacroDestination::ReverbMix, 0.65f), makeMap (MacroDestination::DelayMix, 0.35f) });
    }
    else if (cat.contains ("brass"))
    {
        setMacro (macros, 0, "Punch", { makeMap (MacroDestination::ChopAmount, 0.5f), makeMap (MacroDestination::GateAmount, 0.6f) });
        setMacro (macros, 1, "Growl", { makeMap (MacroDestination::Drive, 0.55f), makeMap (MacroDestination::FilterTone, -0.3f) });
        setMacro (macros, 2, "Brightness", { makeMap (MacroDestination::FilterTone, 0.5f), makeMap (MacroDestination::Smear, -0.2f) });
        setMacro (macros, 3, "Room", { makeMap (MacroDestination::ReverbMix, 0.7f), makeMap (MacroDestination::Width, 0.4f) });
    }
    else if (cat.contains ("string"))
    {
        setMacro (macros, 0, "Motion", { makeMap (MacroDestination::ChopAmount, 0.4f), makeMap (MacroDestination::TextureMix, 0.35f) });
        setMacro (macros, 1, "Warmth", { makeMap (MacroDestination::FilterTone, -0.45f), makeMap (MacroDestination::Smear, -0.25f) });
        setMacro (macros, 2, "Air", { makeMap (MacroDestination::Width, 0.55f), makeMap (MacroDestination::PitchSpread, 0.3f) });
        setMacro (macros, 3, "Space", { makeMap (MacroDestination::ReverbMix, 0.75f), makeMap (MacroDestination::DelayMix, 0.3f) });
    }
    else
    {
        setMacro (macros, 0, "Chop", { makeMap (MacroDestination::ChopAmount, 0.7f), makeMap (MacroDestination::GateAmount, 0.45f) });
        setMacro (macros, 1, "Texture", { makeMap (MacroDestination::TextureMix, 0.65f), makeMap (MacroDestination::Density, 0.5f) });
        setMacro (macros, 2, "Space", { makeMap (MacroDestination::ReverbMix, 0.6f), makeMap (MacroDestination::Width, 0.45f) });
        setMacro (macros, 3, "Tone", { makeMap (MacroDestination::FilterTone, 0.4f), makeMap (MacroDestination::Smear, 0.2f) });
    }

    return macros;
}

EngineState MacroMapper::applyMacros (const EngineState& base,
                                      const std::array<MacroControl, 4>& macros,
                                      const PerformanceApvtsReader::ParamCache& cache) noexcept
{
    EngineState s = base;

    for (int m = 0; m < 4; ++m)
    {
        const float macroVal = PerformanceApvtsReader::readMacroValue (cache, m);
        const float shaped = applyCurve (macroVal, 1.f);
        const float bipolar = (shaped - 0.5f) * 2.f;

        for (const auto& mapping : macros[static_cast<size_t> (m)].mappings)
        {
            if (mapping.destination == MacroDestination::None || std::abs (mapping.amount) < 1.0e-6f)
                continue;

            const float curved = applyCurve (macroVal, mapping.curve);
            const float delta = mapping.amount >= 0.f
                                    ? curved * mapping.amount
                                    : bipolar * std::abs (mapping.amount);
            addToField (s, mapping.destination, delta);
        }
    }

    return s;
}
