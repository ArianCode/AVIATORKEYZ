#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "State/FactoryResources.h"

using namespace juce;
using namespace AviatorKeyz;

namespace
{
constexpr float kSqrt2 = 1.41421356f;
}

AviatorKeyzProcessor::AviatorKeyzProcessor()
    : AudioProcessor (BusesProperties()
                          .withOutput ("Output", AudioChannelSet::stereo(), true))
    , apvts (*this, nullptr, "AviatorKeyzState", createParameterLayout())
    , presetManager (std::make_unique<PresetManager> (apvts))
{
    presetManager->onPresetLoaded = [this] (const juce::String&, const juce::String&, const juce::String& sampleId) {
        loadFactorySample (sampleId, factoryRootNote);
    };

    presetManager->loadPreset (AviatorKeyz::Category::LEADS, "Init");
}

AviatorKeyzProcessor::~AviatorKeyzProcessor() = default;

AudioProcessorValueTreeState::ParameterLayout AviatorKeyzProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<RangedAudioParameter>> params;

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

    return { params.begin(), params.end() };
}

void AviatorKeyzProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    constexpr double kSmoothingTime = 0.02;

    inputGainSmoothed.reset (sampleRate, kSmoothingTime);
    outputGainSmoothed.reset (sampleRate, kSmoothingTime);

    const dsp::ProcessSpec spec { sampleRate, (uint32) samplesPerBlock, 2 };

    samplerEngine.prepare (spec);
    toneShaper.prepare (spec);
    smearProcessor.prepare (spec);
    reverbTail.prepare (spec);

    loadFactorySample (presetManager->getCurrentSampleId(), factoryRootNote);
}

const float* AviatorKeyzProcessor::getFactoryWaveformData() const noexcept
{
    int frames = 0;
    return sampleLibrary.getPrimaryWaveformData (frames);
}

void AviatorKeyzProcessor::loadFactorySample (const juce::String& sampleId, int rootNote)
{
    factoryRootNote = juce::jlimit (0, 127, rootNote);
    loadedSampleId = sampleId;
    factoryWaveformFrames = 0;

    samplerEngine.allSoundOff();

    sampleLibrary.clearAll();

    int numBytes = 0;
    if (const void* data = FactoryResources::getEmbeddedWavData (sampleId, numBytes))
    {
        sampleLibrary.loadFromMemory (data,
                                      static_cast<size_t> (numBytes),
                                      sampleId,
                                      factoryRootNote);
    }

    sampleLibrary.publish();
    samplerEngine.setSampleSnapshot (sampleLibrary.getPublishedSnapshot());
    sampleLibrary.getPrimaryWaveformData (factoryWaveformFrames);
}

void AviatorKeyzProcessor::releaseResources()
{
    samplerEngine.releaseResources();
    smearProcessor.reset();
    reverbTail.reset();
    toneShaper.reset();
}

bool AviatorKeyzProcessor::isBusesLayoutSupported (const AudioProcessor::BusesLayout& layouts) const
{
    if (layouts.getMainInputChannelSet() != AudioChannelSet::disabled())
        return false;

    return layouts.getMainOutputChannelSet() == AudioChannelSet::stereo();
}

void AviatorKeyzProcessor::applyStereoWidth (AudioBuffer<float>& buffer, float width) noexcept
{
    if (buffer.getNumChannels() < 2) return;

    auto* L = buffer.getWritePointer (0);
    auto* R = buffer.getWritePointer (1);
    const int n = buffer.getNumSamples();
    const float w = juce::jlimit (0.f, 2.f, width);

    for (int i = 0; i < n; ++i)
    {
        const float m = 0.5f * (L[i] + R[i]);
        const float s = 0.5f * (L[i] - R[i]) * w;
        L[i] = m + s;
        R[i] = m - s;
    }
}

void AviatorKeyzProcessor::applyPan (AudioBuffer<float>& buffer, float pan) noexcept
{
    if (buffer.getNumChannels() < 2) return;

    auto* L = buffer.getWritePointer (0);
    auto* R = buffer.getWritePointer (1);
    const int n = buffer.getNumSamples();
    const float p = juce::jlimit (-1.f, 1.f, pan);
    const float ang = (p + 1.f) * (MathConstants<float>::halfPi * 0.5f);
    const float gL = std::cos (ang) * kSqrt2;
    const float gR = std::sin (ang) * kSqrt2;

    for (int i = 0; i < n; ++i)
    {
        L[i] *= gL;
        R[i] *= gR;
    }
}

void AviatorKeyzProcessor::processBlock (AudioBuffer<float>& buffer,
                                          MidiBuffer&         midiMessages)
{
    ScopedNoDenormals noDenormals;

    buffer.clear();

    inputGainSmoothed.setTargetValue (
        Decibels::decibelsToGain (apvts.getRawParameterValue (ParamID::INPUT_GAIN)->load()));
    outputGainSmoothed.setTargetValue (
        Decibels::decibelsToGain (apvts.getRawParameterValue (ParamID::OUTPUT_GAIN)->load()));

    const bool reverse = apvts.getRawParameterValue (ParamID::REVERSE)->load() > 0.5f;
    const float glideMs = apvts.getRawParameterValue (ParamID::GLIDE_TIME)->load();
    const float attackMs = apvts.getRawParameterValue (ParamID::ENV_ATTACK)->load();
    const float releaseMs = apvts.getRawParameterValue (ParamID::ENV_RELEASE)->load();

    samplerEngine.setEnvelopeTimesMs (attackMs, releaseMs);
    midiHandler.process (midiMessages, samplerEngine, reverse, glideMs);
    samplerEngine.process (buffer);

    auto* L = buffer.getWritePointer (0);
    auto* R = buffer.getWritePointer (1);
    const int n = buffer.getNumSamples();

    for (int i = 0; i < n; ++i)
    {
        const float gIn = inputGainSmoothed.getNextValue();
        L[i] *= gIn;
        R[i] *= gIn;
    }

    const float tone = apvts.getRawParameterValue (ParamID::TONE)->load();
    const float smear = apvts.getRawParameterValue (ParamID::SMEAR)->load();
    const float revAmt = apvts.getRawParameterValue (ParamID::REVERB_AMOUNT)->load();
    const float revSize = apvts.getRawParameterValue (ParamID::REVERB_SIZE)->load();
    const float width = apvts.getRawParameterValue (ParamID::STEREO_WIDTH)->load();
    const float pan = apvts.getRawParameterValue (ParamID::PAN)->load();

    toneShaper.process (buffer, tone);
    smearProcessor.process (buffer, smear);
    reverbTail.process (buffer, revAmt, revSize);
    applyStereoWidth (buffer, width);
    applyPan (buffer, pan);

    for (int i = 0; i < n; ++i)
    {
        const float gOut = outputGainSmoothed.getNextValue();
        L[i] *= gOut;
        R[i] *= gOut;
    }
}

void AviatorKeyzProcessor::processBlockBypassed (AudioBuffer<float>& buffer,
                                                  MidiBuffer& midiMessages)
{
    buffer.clear();

    const bool reverse = apvts.getRawParameterValue (ParamID::REVERSE)->load() > 0.5f;
    const float glideMs = apvts.getRawParameterValue (ParamID::GLIDE_TIME)->load();
    juce::ignoreUnused (reverse, glideMs);

    midiHandler.processBypassed (midiMessages);
}

AudioProcessorEditor* AviatorKeyzProcessor::createEditor()
{
    return new AviatorKeyzEditor (*this);
}

void AviatorKeyzProcessor::getStateInformation (MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("stateVersion", AviatorKeyz::STATE_SCHEMA_VERSION, nullptr);
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
    juce::ignoreUnused (savedVersion);

    apvts.replaceState (state);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AviatorKeyzProcessor();
}
