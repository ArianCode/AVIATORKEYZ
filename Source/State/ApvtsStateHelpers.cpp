#include "ApvtsStateHelpers.h"
#include "StateSchema.h"
#include "../Debug/AviatorDebug.h"

namespace AviatorKeyz
{
namespace
{
constexpr int kExpectedApvtsParamCount = 130;
} // namespace

bool stateTreeUsesParamChildren (const juce::ValueTree& state)
{
    for (int i = 0; i < state.getNumChildren(); ++i)
        if (state.getChild (i).hasType ("PARAM"))
            return true;

    return false;
}

bool isPartialFactoryPresetState (const juce::ValueTree& state)
{
    if (! stateTreeUsesParamChildren (state))
        return false;

    return state.getNumChildren() < kExpectedApvtsParamCount;
}

void resetApvtsToDefaults (juce::AudioProcessorValueTreeState& apvts)
{
    for (auto* param : apvts.processor.getParameters())
        if (param != nullptr)
            param->setValueNotifyingHost (param->getDefaultValue());
}

void applyKnownGoodAdvancedDefaults (juce::AudioProcessorValueTreeState& apvts)
{
    auto setFloat = [&] (const char* id, float value)
    {
        if (auto* param = apvts.getParameter (id))
            if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (param))
                param->setValueNotifyingHost (ranged->convertTo0to1 (value));
    };

    auto setBool = [&] (const char* id, bool value)
    {
        if (auto* param = apvts.getParameter (id))
            param->setValueNotifyingHost (value ? 1.f : 0.f);
    };

    setFloat (ParamID::PERF_MACRO_1, 0.5f);
    setFloat (ParamID::PERF_MACRO_2, 0.5f);
    setFloat (ParamID::PERF_MACRO_3, 0.5f);
    setFloat (ParamID::PERF_MACRO_4, 0.5f);

    setBool (ParamID::TEX_ENABLED, false);
    setFloat (ParamID::TEX_AMOUNT, 0.f);

    setBool (ParamID::MOD0_ON, false);
    setBool (ParamID::MOD1_ON, false);
    setBool (ParamID::MOD2_ON, false);
}

void applyStateTreeToApvts (juce::AudioProcessorValueTreeState& apvts,
                            const juce::ValueTree& state)
{
    if (! state.isValid())
        return;

    const bool partialFactory = isPartialFactoryPresetState (state);

    if (partialFactory)
    {
        AK_LOG ("applyStateTreeToApvts: partial factory preset ("
                + juce::String (state.getNumChildren()) + " PARAM nodes) — reset defaults");
        resetApvtsToDefaults (apvts);
    }

    apvts.replaceState (state);

    if (partialFactory)
    {
        applyKnownGoodAdvancedDefaults (apvts);

        if (auto* blend = apvts.getParameter (ParamID::SOURCE_BLEND))
        {
            if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (blend))
                blend->setValueNotifyingHost (ranged->convertTo0to1 (0.f));
        }
    }
}

} // namespace AviatorKeyz
