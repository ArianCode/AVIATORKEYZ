#include "ModRoutingHub.h"

namespace
{
struct ParamDest { const char* id; ModDest dest; };

constexpr ParamDest kMap[] {
    { AviatorKeyz::ParamID::INPUT_GAIN,     ModDest::inputGain },
    { AviatorKeyz::ParamID::SMEAR,          ModDest::smear },
    { AviatorKeyz::ParamID::TONE,           ModDest::tone },
    { AviatorKeyz::ParamID::REVERB_AMOUNT,  ModDest::reverbAmount },
    { AviatorKeyz::ParamID::REVERB_SIZE,    ModDest::reverbSize },
    { AviatorKeyz::ParamID::STEREO_WIDTH,   ModDest::stereoWidth },
    { AviatorKeyz::ParamID::PAN,            ModDest::pan },
    { AviatorKeyz::ParamID::FX_DELAY_MIX,   ModDest::delayMix },
    { AviatorKeyz::ParamID::FX_CHORUS_MIX,  ModDest::chorusMix },
    { AviatorKeyz::ParamID::FX_LOFI_AMOUNT, ModDest::lofiAmount },
    { AviatorKeyz::ParamID::FX_DIST_DRIVE,  ModDest::distDrive },
    { AviatorKeyz::ParamID::FILTER_CUTOFF,  ModDest::filterCutoff },
    { AviatorKeyz::ParamID::FILTER_RESONANCE, ModDest::filterResonance },
    { AviatorKeyz::ParamID::OSC1_LEVEL,     ModDest::osc1Level },
    { AviatorKeyz::ParamID::TEX_AMOUNT,     ModDest::textureAmount },
    { AviatorKeyz::ParamID::TEX_GRAIN_RATE, ModDest::grainRate },
};
} // namespace

namespace
{
int choiceIndex (const juce::AudioProcessorValueTreeState& apvts, const char* id)
{
    if (auto* p = dynamic_cast<juce::AudioParameterChoice*> (apvts.getParameter (id)))
        return p->getIndex();
    return static_cast<int> (apvts.getRawParameterValue (id)->load());
}

void setChoiceIndex (juce::AudioProcessorValueTreeState& apvts, const char* id, int index)
{
    if (auto* p = dynamic_cast<juce::AudioParameterChoice*> (apvts.getParameter (id)))
    {
        const int n = p->choices.size();
        const float norm = n <= 1 ? 0.f : static_cast<float> (index) / static_cast<float> (n - 1);
        p->setValueNotifyingHost (norm);
    }
}
} // namespace

std::optional<ModDest> ModRoutingHub::modDestForParam (const juce::String& paramId)
{
    for (const auto& e : kMap)
        if (paramId == e.id)
            return e.dest;
    return std::nullopt;
}

juce::String ModRoutingHub::paramIdForModDest (ModDest dest)
{
    for (const auto& e : kMap)
        if (e.dest == dest)
            return e.id;
    return {};
}

std::optional<int> ModRoutingHub::rowForDest (const juce::AudioProcessorValueTreeState& apvts, ModDest dest)
{
    const int di = static_cast<int> (dest);
    for (int r = 0; r < kNumRows; ++r)
    {
        if (choiceIndex (apvts, kRows[r].dest) == di)
            return r;
    }
    return std::nullopt;
}

std::optional<int> ModRoutingHub::rowForParam (const juce::AudioProcessorValueTreeState& apvts,
                                               const juce::String& paramId)
{
    if (auto d = modDestForParam (paramId))
        return rowForDest (apvts, *d);
    return std::nullopt;
}

void ModRoutingHub::assignRow (juce::AudioProcessorValueTreeState& apvts,
                               int row,
                               int sourceIndex,
                               ModDest dest,
                               float amount,
                               bool enabled)
{
    if (! juce::isPositiveAndBelow (row, kNumRows))
        return;

    const auto& ids = kRows[row];
    if (auto* p = apvts.getParameter (ids.on))
        p->setValueNotifyingHost (enabled ? 1.f : 0.f);
    setChoiceIndex (apvts, ids.source, sourceIndex);
    setChoiceIndex (apvts, ids.dest, static_cast<int> (dest));
    if (auto* p = apvts.getParameter (ids.amount))
        p->setValueNotifyingHost (juce::jlimit (0.f, 1.f, amount));
}

void ModRoutingHub::clearRow (juce::AudioProcessorValueTreeState& apvts, int row)
{
    assignRow (apvts, row, 0, ModDest::none, 0.f, false);
}

juce::Colour ModRoutingHub::colourForSource (int sourceIndex)
{
    static const juce::Colour cols[] {
        juce::Colours::transparentBlack,
        juce::Colour (0xff00cfff),
        juce::Colour (0xffd4bc86),
        juce::Colour (0xfff5a623),
        juce::Colour (0xff7d97b3),
    };
    return cols[juce::jlimit (0, 4, sourceIndex)];
}

ModRoutingHub::KnobModState ModRoutingHub::stateForParam (const juce::AudioProcessorValueTreeState& apvts,
                                                          const juce::String& paramId)
{
    KnobModState s;
    const auto dest = modDestForParam (paramId);
    if (! dest.has_value())
        return s;

    const auto row = rowForDest (apvts, *dest);
    if (! row.has_value())
        return s;

    const auto& ids = kRows[*row];
    if (apvts.getRawParameterValue (ids.on)->load() < 0.5f)
        return s;

    s.row = *row;
    s.sourceIndex = choiceIndex (apvts, ids.source);
    s.amount = apvts.getRawParameterValue (ids.amount)->load();
    s.colour = colourForSource (s.sourceIndex);
    s.active = s.amount > 0.001f && s.sourceIndex > 0;
    return s;
}
