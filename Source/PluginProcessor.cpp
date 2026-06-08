#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "State/ParameterLayout.h"
#include "State/ApvtsStateHelpers.h"
#include "Debug/AviatorDebug.h"
#include "State/FactoryResources.h"
#include "DSP/ModMatrix.h"
#include "DSP/PerformanceMacroEngine.h"

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
    presetManager->onPresetLoaded = [this] (const juce::String&,
                                            const juce::String&,
                                            const juce::String& sampleId,
                                            int rootNote) {
        loadFactorySample (sampleId, rootNote);
    };

    if (! presetManager->loadPreset (AviatorKeyz::Category::LEADS, "Init"))
    {
        if (! presetManager->loadPreset (AviatorKeyz::Category::LEADS, "BOS_AA_Synth_One_Shot_Shadows_C"))
        {
            const auto leads = presetManager->getPresetsForCategory (AviatorKeyz::Category::LEADS);
            if (! leads.isEmpty())
                presetManager->loadPreset (AviatorKeyz::Category::LEADS, leads[0]);
        }
    }
}

AviatorKeyzProcessor::~AviatorKeyzProcessor() = default;

AudioProcessorValueTreeState::ParameterLayout AviatorKeyzProcessor::createParameterLayout()
{
    return AviatorKeyz::createParameterLayout();
}

void AviatorKeyzProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    constexpr double kSmoothingTime = 0.02;

    inputGainSmoothed.reset (sampleRate, kSmoothingTime);
    outputGainSmoothed.reset (sampleRate, kSmoothingTime);

    const dsp::ProcessSpec spec { sampleRate, (uint32) samplesPerBlock, 2 };

    samplerEngine.prepare (spec);
    synthEngine.prepare (spec);
    filterProcessor.prepare (spec);
    textureEngine.prepare (spec);
    outputLimiter.prepare (spec);
    toneShaper.prepare (spec);
    smearProcessor.prepare (spec);
    reverbTail.prepare (spec);
    fxChain.prepare (spec);
    lfoEngine.prepare (sampleRate);

    synthScratch.setSize (2, samplesPerBlock);
    samplerScratch.setSize (2, samplesPerBlock);

    loadFactorySample (presetManager->getCurrentSampleId(), presetManager->getCurrentRootNote());

    inputGainSmoothed.setCurrentAndTargetValue (
        Decibels::decibelsToGain (apvts.getRawParameterValue (ParamID::INPUT_GAIN)->load()));
    outputGainSmoothed.setCurrentAndTargetValue (
        Decibels::decibelsToGain (apvts.getRawParameterValue (ParamID::OUTPUT_GAIN)->load()));

    isPrepared = true;
}

const float* AviatorKeyzProcessor::getFactoryWaveformData() const noexcept
{
    const juce::ScopedLock lock (sampleLoadLock);
    return factoryWaveformData;
}

void AviatorKeyzProcessor::loadFactorySample (const juce::String& sampleId, int rootNote)
{
    AK_LOG ("loadFactorySample: " + sampleId + " root=" + juce::String (rootNote));

    const bool onMessageThread = juce::MessageManager::existsAndIsCurrentThread();
    if (onMessageThread)
        suspendProcessing (true);

    {
        const juce::ScopedLock lock (sampleLoadLock);

        factoryRootNote = juce::jlimit (0, 127, rootNote);
        loadedSampleId = sampleId;
        factoryWaveformFrames = 0;
        factoryWaveformData = nullptr;

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

        if (const float* src = sampleLibrary.getPrimaryWaveformData (factoryWaveformFrames);
            src != nullptr && factoryWaveformFrames > 0)
        {
            factoryWaveformCopy.malloc (static_cast<size_t> (factoryWaveformFrames));
            juce::FloatVectorOperations::copy (factoryWaveformCopy.getData(), src, factoryWaveformFrames);
            factoryWaveformData = factoryWaveformCopy.getData();
        }
        else
        {
            factoryWaveformCopy.free();
            factoryWaveformData = nullptr;
            factoryWaveformFrames = 0;
        }
    }

    AK_LOG ("loadFactorySample done: frames=" + juce::String (factoryWaveformFrames));

    if (onMessageThread)
        suspendProcessing (false);
}

void AviatorKeyzProcessor::releaseResources()
{
    isPrepared = false;
    samplerEngine.releaseResources();
    synthEngine.releaseResources();
    smearProcessor.reset();
    reverbTail.reset();
    toneShaper.reset();
    textureEngine.reset();
    filterProcessor.reset();
    outputLimiter.reset();
    fxChain.reset();
    lfoEngine.reset();
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

    if (! isPrepared)
        return;

    namespace P = AviatorKeyz::ParamID;

    // --- LFO engine ---
    const auto lfoShape = [] (int idx) {
        return static_cast<LfoEngine::Shape> (juce::jlimit (0, 5, idx));
    };

    const double hostBpm = [this] {
        if (auto* playHead = getPlayHead())
        {
            if (auto pos = playHead->getPosition())
                if (auto bpm = pos->getBpm())
                    return *bpm;
        }
        return 120.0;
    }();

    struct LfoParams { const char* rate, *depth, *shape, *sync, *phase; };
    const LfoParams lfoIds[] {
        { P::LFO1_RATE, P::LFO1_DEPTH, P::LFO1_SHAPE, P::LFO1_SYNC, P::LFO1_PHASE },
        { P::LFO2_RATE, P::LFO2_DEPTH, P::LFO2_SHAPE, P::LFO2_SYNC, P::LFO2_PHASE },
        { P::LFO3_RATE, P::LFO3_DEPTH, P::LFO3_SHAPE, P::LFO3_SYNC, P::LFO3_PHASE },
    };

    for (int i = 0; i < LfoEngine::kNumLfos; ++i)
    {
        const auto& ids = lfoIds[i];
        lfoEngine.setRateHz (i, apvts.getRawParameterValue (ids.rate)->load());
        lfoEngine.setDepth (i, apvts.getRawParameterValue (ids.depth)->load());
        lfoEngine.setShape (i, lfoShape (static_cast<int> (apvts.getRawParameterValue (ids.shape)->load())));
        lfoEngine.setSyncToHost (i, apvts.getRawParameterValue (ids.sync)->load() > 0.5f, hostBpm);
        lfoEngine.setPhaseOffset (i, apvts.getRawParameterValue (ids.phase)->load());
    }

    lfoEngine.advance (buffer.getNumSamples());
    modMatrix.updateFromApvts (apvts, lfoEngine);
    const auto& mod = modMatrix.getOffsets();

    const auto macros = PerformanceMacroEngine::compute (
        apvts.getRawParameterValue (P::PERF_MACRO_1)->load(),
        apvts.getRawParameterValue (P::PERF_MACRO_2)->load(),
        apvts.getRawParameterValue (P::PERF_MACRO_3)->load(),
        apvts.getRawParameterValue (P::PERF_MACRO_4)->load());

    inputGainSmoothed.setTargetValue (
        Decibels::decibelsToGain (apvts.getRawParameterValue (P::INPUT_GAIN)->load()
                                  + mod.inputGainDb));
    outputGainSmoothed.setTargetValue (
        Decibels::decibelsToGain (apvts.getRawParameterValue (P::OUTPUT_GAIN)->load()));

    const bool reverse = apvts.getRawParameterValue (P::REVERSE)->load() > 0.5f;
    const float glideMs = apvts.getRawParameterValue (P::GLIDE_TIME)->load();
    const float attackMs = apvts.getRawParameterValue (P::ENV_ATTACK)->load();
    const float decayMs = apvts.getRawParameterValue (P::ENV_AMP_DECAY)->load();
    const float sustain01 = apvts.getRawParameterValue (P::ENV_AMP_SUSTAIN)->load();
    const float releaseMs = apvts.getRawParameterValue (P::ENV_RELEASE)->load();
    const float sourceBlend = apvts.getRawParameterValue (P::SOURCE_BLEND)->load();

    const int polyphony = static_cast<int> (apvts.getRawParameterValue (P::VOICE_POLYPHONY)->load());
    const int playMode = static_cast<int> (apvts.getRawParameterValue (P::VOICE_PLAY_MODE)->load());
    const int glideMode = static_cast<int> (apvts.getRawParameterValue (P::VOICE_GLIDE_MODE)->load());

    samplerEngine.setEnvelopeTimesMs (attackMs, decayMs, sustain01, releaseMs);
    samplerEngine.setPolyphony (polyphony);
    samplerEngine.setPlayMode (playMode);
    samplerEngine.setGlideMode (glideMode);
    samplerEngine.setPhraseParams (
        apvts.getRawParameterValue (P::PHRASE_ENABLED)->load() > 0.5f,
        apvts.getRawParameterValue (P::PHRASE_START)->load(),
        apvts.getRawParameterValue (P::PHRASE_LENGTH)->load(),
        static_cast<int> (apvts.getRawParameterValue (P::PHRASE_PITCH)->load()),
        apvts.getRawParameterValue (P::PHRASE_LOOP)->load() > 0.5f,
        apvts.getRawParameterValue (P::PHRASE_TEMPO_SYNC)->load() > 0.5f,
        apvts.getRawParameterValue (P::PHRASE_KEY_SYNC)->load() > 0.5f,
        hostBpm);

    synthEngine.setPolyphony (polyphony);
    synthEngine.setPlayMode (static_cast<SynthEngine::PlayMode> (playMode));
    synthEngine.setGlideMode (static_cast<SynthEngine::GlideMode> (glideMode));
    synthEngine.setAmpEnvelopeMs (attackMs, decayMs, sustain01, releaseMs);
    synthEngine.setFilterEnvelopeMs (
        apvts.getRawParameterValue (P::ENV_FLT_ATTACK)->load(),
        apvts.getRawParameterValue (P::ENV_FLT_DECAY)->load(),
        apvts.getRawParameterValue (P::ENV_FLT_SUSTAIN)->load(),
        apvts.getRawParameterValue (P::ENV_FLT_RELEASE)->load());

    synthEngine.setOscParams (0,
                              static_cast<int> (apvts.getRawParameterValue (P::OSC1_TYPE)->load()),
                              apvts.getRawParameterValue (P::OSC1_TUNE)->load(),
                              apvts.getRawParameterValue (P::OSC1_FINE)->load(),
                              apvts.getRawParameterValue (P::OSC1_SHAPE)->load(),
                              juce::jlimit (0.f, 1.f, apvts.getRawParameterValue (P::OSC1_LEVEL)->load()
                                            + mod.osc1Level + macros.osc1Level),
                              apvts.getRawParameterValue (P::OSC1_PAN)->load());
    synthEngine.setOscParams (1,
                              static_cast<int> (apvts.getRawParameterValue (P::OSC2_TYPE)->load()),
                              apvts.getRawParameterValue (P::OSC2_TUNE)->load(),
                              apvts.getRawParameterValue (P::OSC2_FINE)->load(),
                              apvts.getRawParameterValue (P::OSC2_SHAPE)->load(),
                              juce::jlimit (0.f, 1.f, apvts.getRawParameterValue (P::OSC2_LEVEL)->load()
                                            + mod.osc2Level + macros.osc2Level),
                              apvts.getRawParameterValue (P::OSC2_PAN)->load());

    midiHandler.process (midiMessages, samplerEngine, synthEngine, reverse, glideMs, sourceBlend);

    const int n = buffer.getNumSamples();
    samplerScratch.setSize (2, n, false, false, true);
    synthScratch.setSize (2, n, false, false, true);
    samplerScratch.clear();
    synthScratch.clear();

    if (sourceBlend < 0.999f)
        samplerEngine.process (samplerScratch);
    if (sourceBlend > 0.001f)
        synthEngine.process (synthScratch);

    auto* L = buffer.getWritePointer (0);
    auto* R = buffer.getWritePointer (1);
    const float sampleGain = 1.f - sourceBlend;
    const float synthGain = sourceBlend;
    const float* sL = samplerScratch.getReadPointer (0);
    const float* sR = samplerScratch.getReadPointer (1);
    const float* yL = synthScratch.getReadPointer (0);
    const float* yR = synthScratch.getReadPointer (1);

    for (int i = 0; i < n; ++i)
    {
        L[i] = sL[i] * sampleGain + yL[i] * synthGain;
        R[i] = sR[i] * sampleGain + yR[i] * synthGain;
    }

    for (int i = 0; i < n; ++i)
    {
        const float gIn = inputGainSmoothed.getNextValue();
        L[i] *= gIn;
        R[i] *= gIn;
    }

    const float cutoffNorm = juce::jlimit (0.f, 1.f,
        juce::jmap (apvts.getRawParameterValue (P::FILTER_CUTOFF)->load(), 20.f, 20000.f, 0.f, 1.f)
        + mod.filterCutoff + macros.filterCutoff);
    const float cutoffHz = juce::jmap (cutoffNorm, 20.f, 20000.f);
    const float filterReso = juce::jlimit (0.f, 1.f,
        apvts.getRawParameterValue (P::FILTER_RESONANCE)->load() + mod.filterReso);
    const auto filterType = static_cast<FilterProcessor::Type> (
        static_cast<int> (apvts.getRawParameterValue (P::FILTER_TYPE)->load()));
    filterProcessor.setParameters (cutoffHz,
                                   filterReso,
                                   filterType,
                                   apvts.getRawParameterValue (P::FILTER_DRIVE)->load(),
                                   apvts.getRawParameterValue (P::ENV_FLT_AMOUNT)->load(),
                                   synthEngine.getFilterEnvLevel());
    filterProcessor.process (buffer);

    const float texAmount = juce::jlimit (0.f, 1.f,
        apvts.getRawParameterValue (P::TEX_AMOUNT)->load() + mod.textureAmount + macros.textureAmount);
    const bool texEnabled = apvts.getRawParameterValue (P::TEX_ENABLED)->load() > 0.5f
                            || texAmount >= 0.001f;
    textureEngine.process (buffer,
                           texEnabled,
                           texAmount,
                           apvts.getRawParameterValue (P::TEX_FREEZE)->load() > 0.5f,
                           juce::jlimit (0.f, 1.f, apvts.getRawParameterValue (P::TEX_GRAIN_RATE)->load() + mod.grainRate + macros.grainRate),
                           juce::jlimit (0.f, 1.f, apvts.getRawParameterValue (P::TEX_GRAIN_SIZE)->load() + mod.grainSize),
                           static_cast<int> (apvts.getRawParameterValue (P::TEX_GRAIN_PITCH)->load()),
                           apvts.getRawParameterValue (P::TEX_GRAIN_DENSITY)->load(),
                           apvts.getRawParameterValue (P::TEX_GRAIN_SPREAD)->load(),
                           apvts.getRawParameterValue (P::TEX_GRAIN_PAN)->load(),
                           apvts.getRawParameterValue (P::TEX_MOTION)->load(),
                           apvts.getRawParameterValue (P::TEX_DRIFT)->load(),
                           apvts.getRawParameterValue (P::TEX_AIR)->load(),
                           apvts.getRawParameterValue (P::TEX_REVERSE)->load() > 0.5f,
                           apvts.getRawParameterValue (P::TEX_WIDTH)->load(),
                           apvts.getRawParameterValue (P::TEX_GRAIN_SCAN)->load(),
                           hostBpm);

    const float tone = juce::jlimit (-1.f, 1.f, apvts.getRawParameterValue (P::TONE)->load() + mod.tone + macros.tone);
    const float smear = juce::jlimit (0.f, 1.f, apvts.getRawParameterValue (P::SMEAR)->load() + mod.smear + macros.smear);
    const float revAmt = juce::jlimit (0.f, 1.f, apvts.getRawParameterValue (P::REVERB_AMOUNT)->load() + mod.reverbAmount + macros.reverbAmount);
    const float revSize = juce::jlimit (0.f, 1.f, apvts.getRawParameterValue (P::REVERB_SIZE)->load() + mod.reverbSize);
    const float width = juce::jlimit (0.f, 2.f, apvts.getRawParameterValue (P::STEREO_WIDTH)->load() + mod.stereoWidth);
    const float pan = juce::jlimit (-1.f, 1.f, apvts.getRawParameterValue (P::PAN)->load() + mod.pan);
    const bool reverbOn = apvts.getRawParameterValue (P::FX_REVERB_ON)->load() > 0.5f
                          || revAmt >= 0.001f;
    const float reverbDamp = apvts.getRawParameterValue (P::FX_REVERB_DAMP)->load();

    toneShaper.process (buffer, tone);
    smearProcessor.process (buffer, smear);
    reverbTail.process (buffer, revAmt, revSize, reverbOn, reverbDamp);

    const bool delayOn   = apvts.getRawParameterValue (P::FX_DELAY_ON)->load() > 0.5f;
    const float delayTime = apvts.getRawParameterValue (P::FX_DELAY_TIME)->load();
    const float delayFb  = apvts.getRawParameterValue (P::FX_DELAY_FEEDBACK)->load();
    const float delayMix = juce::jlimit (0.f, 1.f, apvts.getRawParameterValue (P::FX_DELAY_MIX)->load() + mod.delayMix);
    const bool delaySync = apvts.getRawParameterValue (P::FX_DELAY_SYNC)->load() > 0.5f;

    const bool chorusOn  = apvts.getRawParameterValue (P::FX_CHORUS_ON)->load() > 0.5f;
    const float chorusRate  = apvts.getRawParameterValue (P::FX_CHORUS_RATE)->load();
    const float chorusDepth = apvts.getRawParameterValue (P::FX_CHORUS_DEPTH)->load();
    const float chorusMix   = juce::jlimit (0.f, 1.f, apvts.getRawParameterValue (P::FX_CHORUS_MIX)->load() + mod.chorusMix);

    const bool lofiOn    = apvts.getRawParameterValue (P::FX_LOFI_ON)->load() > 0.5f;
    const float lofiAmt  = juce::jlimit (0.f, 1.f, apvts.getRawParameterValue (P::FX_LOFI_AMOUNT)->load() + mod.lofiAmount);

    const bool distOn    = apvts.getRawParameterValue (P::FX_DIST_ON)->load() > 0.5f;
    const float distDrv  = juce::jlimit (0.f, 1.f, apvts.getRawParameterValue (P::FX_DIST_DRIVE)->load() + mod.distDrive);

    fxChain.process (buffer,
                     delayOn, delayTime, delayFb, delayMix, delaySync,
                     chorusOn, chorusRate, chorusDepth, chorusMix,
                     lofiOn, lofiAmt,
                     distOn, distDrv,
                     hostBpm);

    applyStereoWidth (buffer, width);
    applyPan (buffer, pan);

    outputLimiter.process (buffer, apvts.getRawParameterValue (P::OUTPUT_LIMITER)->load() > 0.5f);

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
    state.setProperty ("presetCategory", presetManager->getCurrentCategory(), nullptr);
    state.setProperty ("presetName", presetManager->getCurrentPresetName(), nullptr);
    state.setProperty (AviatorKeyz::PresetKey::SAMPLE_ID, presetManager->getCurrentSampleId(), nullptr);
    state.setProperty (AviatorKeyz::PresetKey::ROOT_NOTE, presetManager->getCurrentRootNote(), nullptr);
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

    AviatorKeyz::applyStateTreeToApvts (apvts, state);

    const auto category = state.getProperty ("presetCategory", presetManager->getCurrentCategory()).toString();
    const auto name     = state.getProperty ("presetName", presetManager->getCurrentPresetName()).toString();
    const auto sampleId = state.getProperty (AviatorKeyz::PresetKey::SAMPLE_ID,
                                             presetManager->getCurrentSampleId()).toString();
    const int rootNote  = static_cast<int> (state.getProperty (AviatorKeyz::PresetKey::ROOT_NOTE,
                                                                 presetManager->getCurrentRootNote()));

    presetManager->setPresetIdentity (category, name, sampleId, rootNote);
    loadFactorySample (sampleId, rootNote);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AviatorKeyzProcessor();
}
