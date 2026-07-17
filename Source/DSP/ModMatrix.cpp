#include "ModMatrix.h"

namespace
{
struct RowIds
{
    const char* on;
    const char* source;
    const char* dest;
    const char* amount;
};

constexpr RowIds kRows[AviatorKeyz::ParamID::MOD_MATRIX_ROWS] {
    { AviatorKeyz::ParamID::MOD0_ON, AviatorKeyz::ParamID::MOD0_SOURCE, AviatorKeyz::ParamID::MOD0_DEST, AviatorKeyz::ParamID::MOD0_AMOUNT },
    { AviatorKeyz::ParamID::MOD1_ON, AviatorKeyz::ParamID::MOD1_SOURCE, AviatorKeyz::ParamID::MOD1_DEST, AviatorKeyz::ParamID::MOD1_AMOUNT },
    { AviatorKeyz::ParamID::MOD2_ON, AviatorKeyz::ParamID::MOD2_SOURCE, AviatorKeyz::ParamID::MOD2_DEST, AviatorKeyz::ParamID::MOD2_AMOUNT },
    { AviatorKeyz::ParamID::MOD3_ON, AviatorKeyz::ParamID::MOD3_SOURCE, AviatorKeyz::ParamID::MOD3_DEST, AviatorKeyz::ParamID::MOD3_AMOUNT },
    { AviatorKeyz::ParamID::MOD4_ON, AviatorKeyz::ParamID::MOD4_SOURCE, AviatorKeyz::ParamID::MOD4_DEST, AviatorKeyz::ParamID::MOD4_AMOUNT },
    { AviatorKeyz::ParamID::MOD5_ON, AviatorKeyz::ParamID::MOD5_SOURCE, AviatorKeyz::ParamID::MOD5_DEST, AviatorKeyz::ParamID::MOD5_AMOUNT },
    { AviatorKeyz::ParamID::MOD6_ON, AviatorKeyz::ParamID::MOD6_SOURCE, AviatorKeyz::ParamID::MOD6_DEST, AviatorKeyz::ParamID::MOD6_AMOUNT },
    { AviatorKeyz::ParamID::MOD7_ON, AviatorKeyz::ParamID::MOD7_SOURCE, AviatorKeyz::ParamID::MOD7_DEST, AviatorKeyz::ParamID::MOD7_AMOUNT },
};
} // namespace

juce::StringArray ModMatrix::sourceNames()
{
    return { "Off", "LFO 1", "LFO 2", "LFO 3" };
}

juce::StringArray ModMatrix::destNames()
{
    return { "Off", "Input Gain", "Filter", "Tone", "Reverb", "Reverb Size", "Brightness", "Pan",
             "Delay Mix", "Chorus Mix", "Lo-Fi", "Dist Drive", "Filter Cutoff", "Filter Reso",
             "Osc1 Level", "Osc2 Level", "Texture Amount", "Grain Rate", "Grain Size" };
}

namespace
{
int choiceIndex (const juce::AudioProcessorValueTreeState& apvts, const char* id) noexcept
{
    return static_cast<juce::AudioParameterChoice*> (apvts.getParameter (id))->getIndex();
}
} // namespace

float ModMatrix::sourceValue (int sourceIndex, const LfoEngine& lfos)
{
    switch (sourceIndex)
    {
        case 1: return lfos.getValue (0);
        case 2: return lfos.getValue (1);
        case 3: return lfos.getValue (2);
        default: return 0.f;
    }
}

void ModMatrix::applyToDest (ModDest dest, float delta, Offsets& o)
{
    switch (dest)
    {
        case ModDest::inputGain:   o.inputGainDb  += delta * 6.f; break;
        case ModDest::smear:       o.smear        += delta * 0.5f; break;
        case ModDest::tone:        o.tone         += delta * 0.5f; break;
        case ModDest::reverbAmount:o.reverbAmount += delta * 0.5f; break;
        case ModDest::reverbSize:  o.reverbSize   += delta * 0.5f; break;
        case ModDest::brightness:  o.brightness   += delta * 0.5f; break;
        case ModDest::pan:         o.pan          += delta * 0.5f; break;
        case ModDest::delayMix:    o.delayMix     += delta * 0.5f; break;
        case ModDest::chorusMix:   o.chorusMix    += delta * 0.5f; break;
        case ModDest::lofiAmount:  o.lofiAmount   += delta * 0.5f; break;
        case ModDest::distDrive:   o.distDrive     += delta * 0.5f; break;
        case ModDest::filterCutoff:  o.filterCutoff  += delta * 0.5f; break;
        case ModDest::filterResonance: o.filterReso += delta * 0.5f; break;
        case ModDest::osc1Level:   o.osc1Level     += delta * 0.5f; break;
        case ModDest::osc2Level:   o.osc2Level     += delta * 0.5f; break;
        case ModDest::textureAmount: o.textureAmount += delta * 0.5f; break;
        case ModDest::grainRate:   o.grainRate     += delta * 0.5f; break;
        case ModDest::grainSize:   o.grainSize     += delta * 0.5f; break;
        default: break;
    }
}

void ModMatrix::updateFromApvts (const juce::AudioProcessorValueTreeState& apvts,
                                 const LfoEngine& lfos)
{
    offsets = {};

    for (const auto& row : kRows)
    {
        if (apvts.getRawParameterValue (row.on)->load() < 0.5f)
            continue;

        const int src = choiceIndex (apvts, row.source);
        const int dst = choiceIndex (apvts, row.dest);
        const float amt = apvts.getRawParameterValue (row.amount)->load();

        if (dst <= 0 || amt < 0.001f)
            continue;

        const float mod = sourceValue (src, lfos) * amt;
        applyToDest (static_cast<ModDest> (dst), mod, offsets);
    }
}
