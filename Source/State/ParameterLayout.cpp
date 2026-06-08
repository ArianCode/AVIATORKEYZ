#include "ParameterLayout.h"
#include "StateSchema.h"
#include "../GUI/Advanced/AdvancedParameterLayout.h"

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
        "Smear",
        NormalisableRange<float> (0.0f, 1.0f, 0.001f),
        0.0f,
        AudioParameterFloatAttributes()
            .withStringFromValueFunction ([] (float v, int) {
                return String (static_cast<int> (v * 100)) + "%";
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
        "Width",
        NormalisableRange<float> (0.0f, 2.0f, 0.001f),
        1.0f,
        AudioParameterFloatAttributes()
            .withStringFromValueFunction ([] (float v, int) {
                if (v < 0.01f) return String ("Mono");
                if (std::abs (v - 1.0f) < 0.01f) return String ("Stereo");
                return String (static_cast<int> (v * 100)) + "%";
            })));

    params.push_back (std::make_unique<AudioParameterFloat> (
        ParameterID { ParamID::ENV_ATTACK, 1 },
        "Attack",
        NormalisableRange<float> (0.5f, 5000.0f, 0.1f, 0.4f),
        5.0f,
        AudioParameterFloatAttributes()
            .withLabel ("ms")
            .withStringFromValueFunction ([] (float v, int) {
                return String (v, 1) + " ms";
            })));

    params.push_back (std::make_unique<AudioParameterFloat> (
        ParameterID { ParamID::ENV_RELEASE, 1 },
        "Release",
        NormalisableRange<float> (5.0f, 10000.0f, 0.1f, 0.35f),
        150.0f,
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
