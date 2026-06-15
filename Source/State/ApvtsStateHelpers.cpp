#include "ApvtsStateHelpers.h"
#include "StateSchema.h"
#include "../Debug/AviatorDebug.h"

namespace AviatorKeyz
{
namespace
{
constexpr int kExpectedApvtsParamCount = 132;

void applyParamChildrenFromState (juce::AudioProcessorValueTreeState& apvts,
                                  const juce::ValueTree& state)
{
    for (int i = 0; i < state.getNumChildren(); ++i)
    {
        const auto child = state.getChild (i);
        if (! child.hasType ("PARAM"))
            continue;

        const auto id = child.getProperty ("id").toString();
        if (id.isEmpty())
            continue;

        const float value = static_cast<float> (child.getProperty ("value"));
        if (auto* param = apvts.getParameter (id))
            if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (param))
                param->setValueNotifyingHost (ranged->convertTo0to1 (value));
    }
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

    // Neutral sampler baseline — applied before factory preset PARAM nodes merge.
    setFloat (ParamID::SOURCE_BLEND, 0.f);
    setFloat (ParamID::INPUT_GAIN, 0.f);
    setFloat (ParamID::OUTPUT_GAIN, 0.f);
    setFloat (ParamID::PAN, 0.f);

    setFloat (ParamID::ENV_ATTACK, 0.f);
    setFloat (ParamID::ENV_AMP_DECAY, 0.f);
    setFloat (ParamID::ENV_AMP_SUSTAIN, 1.f);
    setFloat (ParamID::ENV_RELEASE, 10.f);
    setFloat (ParamID::VELOCITY_SENSITIVITY, 0.f);

    setBool (ParamID::FILTER_ENABLED, false);
    setFloat (ParamID::FILTER_DRIVE, 0.f);
    setFloat (ParamID::ENV_FLT_AMOUNT, 0.f);

    setFloat (ParamID::TONE, 0.f);
    setFloat (ParamID::SMEAR, 0.f);
    setFloat (ParamID::REVERB_AMOUNT, 0.f);
    setBool (ParamID::FX_REVERB_ON, false);

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

void applyStateTreeToApvts (juce::AudioProcessorValueTreeState& apvts,
                            const juce::ValueTree& state)
{
    if (! state.isValid())
        return;

    const bool partialFactory = isPartialFactoryPresetState (state);

    if (partialFactory)
    {
        AK_LOG ("applyStateTreeToApvts: partial factory preset ("
                + juce::String (state.getNumChildren()) + " PARAM nodes) — merge onto defaults");
        resetApvtsToDefaults (apvts);
        applyKnownGoodAdvancedDefaults (apvts);
        applyParamChildrenFromState (apvts, state);

        if (auto* blend = apvts.getParameter (ParamID::SOURCE_BLEND))
            if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (blend))
                blend->setValueNotifyingHost (ranged->convertTo0to1 (0.f));

        return;
    }

    apvts.replaceState (state);
}

} // namespace AviatorKeyz
