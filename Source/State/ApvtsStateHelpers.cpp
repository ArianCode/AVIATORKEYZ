#include "ApvtsStateHelpers.h"
#include "StateSchema.h"
#include "../Debug/AviatorDebug.h"
#include "../DSP/Performance/PerformanceApvtsReader.h"

namespace AviatorKeyz
{
namespace
{
// Full APVTS parameter count. A state tree with fewer PARAM children is a
// partial factory preset (or a project saved by an older build) and is merged
// onto defaults instead of replacing the whole tree.
constexpr int kExpectedApvtsParamCount = 322; // 266 + MFX rack (2 slots x 28)

void setParamNormalised (juce::RangedAudioParameter* param, float normalised)
{
    if (param != nullptr)
        param->setValueNotifyingHost (normalised);
}

void setParamDefault (juce::AudioProcessorParameter* param)
{
    if (param != nullptr)
        param->setValueNotifyingHost (param->getDefaultValue());
}

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

        const float value = [&]
        {
            float v = static_cast<float> (child.getProperty ("value"));
            if (id == ParamID::STEREO_WIDTH)
            {
                if (v > 1.0f)
                    v *= 0.5f; // migrate legacy 0–2 width scale to 0–1 brightness
                else if (std::abs (v - 1.0f) < 0.0001f)
                    v = 0.5f; // legacy neutral width (1.0) → neutral brightness (0.5)
            }
            return v;
        }();

        if (auto* param = apvts.getParameter (id))
            setParamNormalised (param, param->convertTo0to1 (value));
    }
}

void applyKnownGoodAdvancedDefaults (juce::AudioProcessorValueTreeState& apvts)
{
    auto setFloat = [&] (const char* id, float value)
    {
        if (auto* param = apvts.getParameter (id))
            setParamNormalised (param, param->convertTo0to1 (value));
    };

    auto setBool = [&] (const char* id, bool value)
    {
        if (auto* param = apvts.getParameter (id))
            setParamNormalised (param, value ? 1.f : 0.f);
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

    // LAYER MIX layers start silent; the sampler is the only sounding layer.
    setFloat (ParamID::OSC1_LEVEL, 0.f);
    setFloat (ParamID::OSC2_LEVEL, 0.f);
    setBool (ParamID::ARP_ON, false);
    setBool (ParamID::ARP_HOLD, false);
    setBool (ParamID::ENV_ENABLED, true);

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
        setParamDefault (param);
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
        auto merged = state.createCopy();
        migrateLegacyAdvancedParams (merged);
        applyParamChildrenFromState (apvts, merged);

        if (auto* blend = apvts.getParameter (ParamID::SOURCE_BLEND))
            setParamNormalised (blend, blend->convertTo0to1 (0.f));

        return;
    }

    apvts.replaceState (state);
    migrateLegacyAdvancedParams (apvts.state);
}

} // namespace AviatorKeyz
