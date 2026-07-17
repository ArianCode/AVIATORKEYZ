#include "ParameterLayout.h"
#include "StateSchema.h"
#include "../GUI/Advanced/AdvancedParameterLayout.h"
#include "../GUI/Advanced/PerformanceParameterLayout.h"

using namespace juce;

namespace AviatorKeyz
{

namespace
{
void appendCoreParameters (std::vector<std::unique_ptr<RangedAudioParameter>>& params)
{
    params.push_back (std::make_unique<AudioParameterFloat> (
        ParameterID { ParamID::INPUT_GAIN, 1 },
        "Input Gain",
        NormalisableRange<float> (-24.0f, 12.0f, 0.01f),
        0.0f,
        AudioParameterFloatAttributes()
            .withLabel ("dB")
            .withStringFromValueFunction ([] (float v, int) { return String (v, 1) + " dB"; })
            .withValueFromStringFunction ([] (const String& s) { return s.getFloatValue(); })));

    params.push_back (std::make_unique<AudioParameterFloat> (
        ParameterID { ParamID::OUTPUT_GAIN, 1 },
        "Output Gain",
        NormalisableRange<float> (-24.0f, 12.0f, 0.01f),
        0.0f,
        AudioParameterFloatAttributes()
            .withLabel ("dB")
            .withStringFromValueFunction ([] (float v, int) { return String (v, 1) + " dB"; })
            .withValueFromStringFunction ([] (const String& s) { return s.getFloatValue(); })));

    params.push_back (std::make_unique<AudioParameterBool> (
        ParameterID { ParamID::REVERSE, 1 },
        "Reverse",
        false));

    params.push_back (std::make_unique<AudioParameterFloat> (
        ParameterID { ParamID::GLIDE_TIME, 1 },
        "Glide",
        NormalisableRange<float> (0.0f, 500.0f, 0.1f, 0.35f),
        0.0f,
        AudioParameterFloatAttributes()
            .withLabel ("ms")
            .withStringFromValueFunction ([] (float v, int) {
                if (v < 1.0f) return String ("Off");
                return String (static_cast<int> (v)) + " ms";
            })));

    params.push_back (std::make_unique<AudioParameterFloat> (
        ParameterID { ParamID::SMEAR, 1 },
        "Filter",
        NormalisableRange<float> (0.0f, 1.0f, 0.001f),
        0.0f,
        AudioParameterFloatAttributes()
            .withStringFromValueFunction ([] (float v, int) {
                if (v < 0.001f)
                    return String ("Off");
                const float logMin = std::log (80.f);
                const float logMax = std::log (16000.f);
                const float hz = std::exp (juce::jmap (v, 0.f, 1.f, logMax, logMin));
                if (hz >= 1000.f)
                    return String (hz / 1000.f, 1) + "k";
                return String (static_cast<int> (hz));
            })));

    params.push_back (std::make_unique<AudioParameterFloat> (
        ParameterID { ParamID::TONE, 1 },
        "Tone",
        NormalisableRange<float> (-1.0f, 1.0f, 0.001f),
        0.0f,
        AudioParameterFloatAttributes()
            .withStringFromValueFunction ([] (float v, int) {
                if (std::abs (v) < 0.01f) return String ("Neutral");
                if (v < 0.0f)
                    return String (static_cast<int> (std::abs (v) * 100)) + "% Dark";
                return String (static_cast<int> (v * 100)) + "% Bright";
            })));

    params.push_back (std::make_unique<AudioParameterFloat> (
        ParameterID { ParamID::REVERB_AMOUNT, 1 },
        "Reverb",
        NormalisableRange<float> (0.0f, 1.0f, 0.001f),
        0.0f,
        AudioParameterFloatAttributes()
            .withStringFromValueFunction ([] (float v, int) {
                return String (static_cast<int> (v * 100)) + "%";
            })));

    params.push_back (std::make_unique<AudioParameterFloat> (
        ParameterID { ParamID::REVERB_SIZE, 1 },
        "Reverb Size",
        NormalisableRange<float> (0.0f, 1.0f, 0.001f),
        0.5f,
        AudioParameterFloatAttributes()
            .withStringFromValueFunction ([] (float v, int) {
                return String (static_cast<int> (v * 100)) + "%";
            })));

    params.push_back (std::make_unique<AudioParameterFloat> (
        ParameterID { ParamID::STEREO_WIDTH, 1 },
        "Brightness",
        NormalisableRange<float> (0.0f, 1.0f, 0.001f),
        0.5f,
        AudioParameterFloatAttributes()
            .withStringFromValueFunction ([] (float v, int) {
                return String (static_cast<int> (v * 100)) + "%";
            })));

    params.push_back (std::make_unique<AudioParameterFloat> (
        ParameterID { ParamID::ENV_ATTACK, 1 },
        "Attack",
        NormalisableRange<float> (0.0f, 5000.0f, 0.1f, 0.4f),
        0.0f,
        AudioParameterFloatAttributes()
            .withLabel ("ms")
            .withStringFromValueFunction ([] (float v, int) {
                return String (v, 1) + " ms";
            })));

    params.push_back (std::make_unique<AudioParameterFloat> (
        ParameterID { ParamID::ENV_RELEASE, 1 },
        "Release",
        NormalisableRange<float> (0.01f, 10000.0f, 0.01f, 0.35f),
        10.0f,
        AudioParameterFloatAttributes()
            .withLabel ("ms")
            .withStringFromValueFunction ([] (float v, int) {
                return String (v, 1) + " ms";
            })));

    params.push_back (std::make_unique<AudioParameterFloat> (
        ParameterID { ParamID::PAN, 1 },
        "Pan",
        NormalisableRange<float> (-1.0f, 1.0f, 0.001f),
        0.0f,
        AudioParameterFloatAttributes()
            .withStringFromValueFunction ([] (float v, int) {
                if (std::abs (v) < 0.01f) return String ("C");
                return String (v, 2);
            })));
}

std::vector<std::unique_ptr<RangedAudioParameter>> buildAllParameters()
{
    std::vector<std::unique_ptr<RangedAudioParameter>> params;
    appendCoreParameters (params);
    AdvancedParameterLayout::appendParameters (params);
    PerformanceParameterLayout::appendParameters (params);
    return params;
}

} // namespace

AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    auto params = buildAllParameters();
    return { params.begin(), params.end() };
}

StringArray getRegisteredParameterIds()
{
    StringArray ids;
    for (const auto& param : buildAllParameters())
        if (param != nullptr)
            ids.add (param->paramID);
    return ids;
}

} // namespace AviatorKeyz
