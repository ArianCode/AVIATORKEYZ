#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace juce;
using namespace AviatorKeyz;

// =============================================================================
//  Construction / Destruction
// =============================================================================

AviatorKeyzProcessor::AviatorKeyzProcessor()
    : AudioProcessor (BusesProperties()
                          .withOutput ("Output", AudioChannelSet::stereo(), true))
    , apvts (*this, nullptr, "AviatorKeyzState", createParameterLayout())
{
}

AviatorKeyzProcessor::~AviatorKeyzProcessor() = default;

// =============================================================================
//  Parameter layout
//
//  CompatibilityNote:
//    Parameter IDs in this layout are fixed after 1.0 release.
//    Adding new params at the end is safe — hosts will receive defaults.
//    Never remove, rename, or rescale an existing parameter.
// =============================================================================

AudioProcessorValueTreeState::ParameterLayout
AviatorKeyzProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<RangedAudioParameter>> params;

    // --- Signal path -------------------------------------------------------

    // Input gain: -24 to +12 dB, default 0 dB
    params.push_back (std::make_unique<AudioParameterFloat> (
        ParameterID { ParamID::INPUT_GAIN, 1 },
        "Input Gain",
        NormalisableRange<float> (-24.0f, 12.0f, 0.01f),
        0.0f,
        AudioParameterFloatAttributes()
            .withLabel ("dB")
            .withStringFromValueFunction ([] (float v, int) {
                return String (v, 1) + " dB";
            })
            .withValueFromStringFunction ([] (const String& s) {
                return s.getFloatValue();
            })));

    // Output gain: -24 to +12 dB, default 0 dB
    params.push_back (std::make_unique<AudioParameterFloat> (
        ParameterID { ParamID::OUTPUT_GAIN, 1 },
        "Output Gain",
        NormalisableRange<float> (-24.0f, 12.0f, 0.01f),
        0.0f,
        AudioParameterFloatAttributes()
            .withLabel ("dB")
            .withStringFromValueFunction ([] (float v, int) {
                return String (v, 1) + " dB";
            })
            .withValueFromStringFunction ([] (const String& s) {
                return s.getFloatValue();
            })));

    // --- Creative Engine ---------------------------------------------------

    // Reverse: boolean toggle, default off
    params.push_back (std::make_unique<AudioParameterBool> (
        ParameterID { ParamID::REVERSE, 1 },
        "Reverse",
        false));

    // Glide: 0–500 ms, skewed toward low values, default 0 (off)
    // Skew factor 0.35 means ~80% of knob range covers 0–50 ms
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

    // Smear: 0.0–1.0, default 0.0
    params.push_back (std::make_unique<AudioParameterFloat> (
        ParameterID { ParamID::SMEAR, 1 },
        "Smear",
        NormalisableRange<float> (0.0f, 1.0f, 0.001f),
        0.0f,
        AudioParameterFloatAttributes()
            .withStringFromValueFunction ([] (float v, int) {
                return String (static_cast<int> (v * 100)) + "%";
            })));

    // Tone: -1.0 (dark/warm) to +1.0 (bright/clean), default 0.0
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

    // --- Space / Output section --------------------------------------------

    // Reverb amount: 0–1, default 0
    params.push_back (std::make_unique<AudioParameterFloat> (
        ParameterID { ParamID::REVERB_AMOUNT, 1 },
        "Reverb",
        NormalisableRange<float> (0.0f, 1.0f, 0.001f),
        0.0f,
        AudioParameterFloatAttributes()
            .withStringFromValueFunction ([] (float v, int) {
                return String (static_cast<int> (v * 100)) + "%";
            })));

    // Reverb size: 0–1, default 0.5
    params.push_back (std::make_unique<AudioParameterFloat> (
        ParameterID { ParamID::REVERB_SIZE, 1 },
        "Reverb Size",
        NormalisableRange<float> (0.0f, 1.0f, 0.001f),
        0.5f,
        AudioParameterFloatAttributes()
            .withStringFromValueFunction ([] (float v, int) {
                return String (static_cast<int> (v * 100)) + "%";
            })));

    // Stereo width: 0.0 (mono) to 2.0 (extra wide), default 1.0
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

    return { params.begin(), params.end() };
}

// =============================================================================
//  Lifecycle
// =============================================================================

void AviatorKeyzProcessor::prepareToPlay (double sampleRate, int /*samplesPerBlock*/)
{
    constexpr double kSmoothingTime = 0.02; // 20 ms — eliminates zipper noise

    inputGainSmoothed   .reset (sampleRate, kSmoothingTime);
    outputGainSmoothed  .reset (sampleRate, kSmoothingTime);
    toneSmoothed        .reset (sampleRate, kSmoothingTime);
    smearSmoothed       .reset (sampleRate, kSmoothingTime);
    reverbAmountSmoothed.reset (sampleRate, kSmoothingTime);
    stereoWidthSmoothed .reset (sampleRate, kSmoothingTime);

    // M1: samplerEngine->prepare({ sampleRate, (uint32)samplesPerBlock, 2 });
    // M2: toneShaper->prepare(...)  smearProcessor->prepare(...)  reverb.prepare(...)
}

void AviatorKeyzProcessor::releaseResources()
{
    // M1+: call release on DSP modules
}

bool AviatorKeyzProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    // Instrument: no audio inputs, stereo output only
    if (layouts.getMainInputChannelSet() != AudioChannelSet::disabled())
        return false;

    return layouts.getMainOutputChannelSet() == AudioChannelSet::stereo();
}

// =============================================================================
//  Process block
//
//  Audio thread — no allocations, no UI calls, no try/catch, no locks.
// =============================================================================

void AviatorKeyzProcessor::processBlock (AudioBuffer<float>& buffer,
                                          MidiBuffer&         midiMessages)
{
    ScopedNoDenormals noDenormals;

    // Clear output — DSP modules will fill this in M1
    buffer.clear();

    // Pull current param targets (std::atomic loads, safe on audio thread)
    inputGainSmoothed   .setTargetValue (
        Decibels::decibelsToGain (apvts.getRawParameterValue (ParamID::INPUT_GAIN)->load()));
    outputGainSmoothed  .setTargetValue (
        Decibels::decibelsToGain (apvts.getRawParameterValue (ParamID::OUTPUT_GAIN)->load()));
    toneSmoothed        .setTargetValue (apvts.getRawParameterValue (ParamID::TONE)->load());
    smearSmoothed       .setTargetValue (apvts.getRawParameterValue (ParamID::SMEAR)->load());
    reverbAmountSmoothed.setTargetValue (apvts.getRawParameterValue (ParamID::REVERB_AMOUNT)->load());
    stereoWidthSmoothed .setTargetValue (apvts.getRawParameterValue (ParamID::STEREO_WIDTH)->load());

    // M1: samplerEngine->process(buffer, midiMessages, glideEngine, reversePlayer);
    // M2: toneShaper->process(buffer, toneSmoothed);
    //     smearProcessor->process(buffer, smearSmoothed);
    //     reverb.process(buffer, reverbAmountSmoothed);
    //     stereoWidthMatrix(buffer, stereoWidthSmoothed);
    //     outputGain(buffer, outputGainSmoothed);

    // Suppress unused-variable warnings until M1 wires these up
    (void) midiMessages;
}

void AviatorKeyzProcessor::processBlockBypassed (AudioBuffer<float>& buffer,
                                                   MidiBuffer&)
{
    buffer.clear();
}

// =============================================================================
//  Editor
// =============================================================================

AudioProcessorEditor* AviatorKeyzProcessor::createEditor()
{
    return new AviatorKeyzEditor (*this);
}

// =============================================================================
//  State persistence
//
//  CompatibilityNote:
//    State is serialized as XML embedded in a binary blob (standard JUCE idiom).
//    stateVersion property is written so future code can detect and migrate
//    old presets when STATE_SCHEMA_VERSION is bumped.
// =============================================================================

void AviatorKeyzProcessor::getStateInformation (MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("stateVersion",
                        AviatorKeyz::STATE_SCHEMA_VERSION,
                        nullptr);
    if (const auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void AviatorKeyzProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    const auto xml = getXmlFromBinary (data, sizeInBytes);
    if (xml == nullptr) return;

    auto state = ValueTree::fromXml (*xml);
    if (! state.isValid()) return;

    const int savedVersion = state.getProperty ("stateVersion", 1);

    // Migration point — add cases here when STATE_SCHEMA_VERSION increments:
    //   if (savedVersion < 2) migrateTo_v2(state);
    //   if (savedVersion < 3) migrateTo_v3(state);
    juce::ignoreUnused (savedVersion);

    apvts.replaceState (state);
}

// =============================================================================
//  Entry point — required by JUCE
// =============================================================================

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AviatorKeyzProcessor();
}
