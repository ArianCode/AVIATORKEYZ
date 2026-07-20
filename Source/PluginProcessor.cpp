#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "State/ParameterLayout.h"
#include "State/ApvtsStateHelpers.h"
#include "State/CategorySoundPolicy.h"
#include "Debug/AviatorDebug.h"
#include "Debug/PlaybackProbe.h"
#include "State/FactoryResources.h"
#include "DSP/Performance/PerformanceApvtsReader.h"
#include "DSP/Performance/MacroMapper.h"
#include "DSP/Performance/PerformanceTexturePipeline.h"
#include "DSP/FastMath.h"

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
    presetManager->onPresetLoaded = [this] (const juce::String& category,
                                            const juce::String&,
                                            const juce::String& sampleId,
                                            int rootNote) {
        juce::ignoreUnused (category);
        loadFactorySample (sampleId, rootNote);
    };

    presetManager->onMacroMapsLoaded = [this] (const std::array<MacroControl, 4>& macros) {
        macroControls = macros;
    };

    perfParamCache.init (apvts);

    macroControls = MacroMapper::defaultsForCategory (AviatorKeyz::Category::LEADS);

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
    outputLimiter.prepare (spec);
    toneShaper.prepare (spec);
    brightnessShaper.prepare (spec);
    smearProcessor.prepare (spec);
    reverbTail.prepare (spec);
    fxChain.prepare (spec);
    performancePipeline.prepare (spec);

    const auto sampleId = presetManager->getCurrentSampleId();
    if (sampleId != loadedSampleId)
        loadFactorySample (sampleId, presetManager->getCurrentRootNote());

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

bool AviatorKeyzProcessor::loadFactorySample (const juce::String& sampleId, int rootNote)
{
    {
        const juce::ScopedLock lock (sampleLoadLock);
        // Root is resolved from the sample (smpl > argument); identity is sampleId.
        if (sampleId == loadedSampleId)
        {
            const int authoritativeRoot = sampleLibrary.getPrimaryRootNote();
            factoryRootNote = authoritativeRoot;
            presetManager->setCurrentRootNote (authoritativeRoot);
            AviatorKeyz::syncRootNoteToApvts (apvts, authoritativeRoot);
            juce::ignoreUnused (rootNote);
            return true;
        }
    }

    if (sampleId.isEmpty())
    {
        AK_LOG ("Preset contains an empty sampleId");
        return false;
    }

    if (! AviatorKeyz::isSampleIdCompatibleWithCategory (sampleId, presetManager->getCurrentCategory()))
    {
        AK_LOG ("loadFactorySample rejected — sampleId does not match active category: "
                + sampleId + " tab=" + presetManager->getCurrentCategory());
        return false;
    }

    int numBytes = 0;
    const void* embeddedData = FactoryResources::tryGetEmbeddedWavData (sampleId, numBytes);

    if (embeddedData == nullptr || numBytes <= 0)
    {
        AK_LOG ("Unable to resolve preset sample: " + sampleId);
        return false;
    }

    {
        SampleLibrary probe;
        if (! probe.loadFromMemory (embeddedData,
                                     static_cast<size_t> (numBytes),
                                     sampleId,
                                     juce::jlimit (0, 127, rootNote)))
        {
            AK_LOG ("Failed to decode preset sample: " + sampleId
                    + " — " + probe.getLastError());
            return false;
        }
    }

    AK_LOG ("loadFactorySample: " + sampleId + " root=" + juce::String (rootNote));

    // Stop the audio thread before mutating sample storage or voice state.
    // Must run on every call path (UI preset pick, host setState, prepareToPlay).
    suspendProcessing (true);

    bool loaded = false;

    {
        const juce::ScopedLock lock (sampleLoadLock);

        samplerEngine.allSoundOff();
        sampleLibrary.clearAll();

        if (! sampleLibrary.loadFromMemory (embeddedData,
                                            static_cast<size_t> (numBytes),
                                            sampleId,
                                            juce::jlimit (0, 127, rootNote)))
        {
            AK_LOG ("loadFactorySample aborted: " + sampleLibrary.getLastError());
        }
        else
        {
            sampleLibrary.publish();
            samplerEngine.setSampleSnapshot (sampleLibrary.getPublishedSnapshot());

            // smpl chunk may override the preset XML root — that resolved value is authoritative.
            const int effectiveRoot = sampleLibrary.getPrimaryRootNote();
            factoryRootNote = effectiveRoot;
            loadedSampleId = sampleId;
            factoryWaveformFrames = 0;
            factoryWaveformData = nullptr;

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

            loaded = true;
        }
    }

    AK_LOG ("loadFactorySample done: frames=" + juce::String (factoryWaveformFrames)
            + " loaded=" + juce::String (loaded ? "yes" : "no"));

    suspendProcessing (false);

    if (! loaded)
        return false;

    // One authoritative root after load: sample region → PresetManager → APVTS.
    const int authoritativeRoot = sampleLibrary.getPrimaryRootNote();
    factoryRootNote = authoritativeRoot;
    presetManager->setCurrentRootNote (authoritativeRoot);
    AviatorKeyz::syncRootNoteToApvts (apvts, authoritativeRoot);

    const bool samplerValid = samplerEngine.validateCurrentState();

    const float sourceBlend = apvts.getRawParameterValue (AviatorKeyz::ParamID::SOURCE_BLEND)->load();
    if (sourceBlend >= 0.999f)
        AK_LOG ("Sampler validation warning: source_blend routes fully to synth ("
                + juce::String (sourceBlend) + ")");

    const float inputGainDb = apvts.getRawParameterValue (AviatorKeyz::ParamID::INPUT_GAIN)->load();
    const float inputGain = Decibels::decibelsToGain (inputGainDb);
    if (! std::isfinite (inputGain) || inputGain <= 0.00001f)
        AK_LOG ("Sampler validation warning: input gain is zero or invalid");

    return samplerValid;
}

void AviatorKeyzProcessor::releaseResources()
{
    isPrepared = false;
    samplerEngine.releaseResources();
    synthEngine.releaseResources();
    smearProcessor.reset();
    reverbTail.reset();
    toneShaper.reset();
    brightnessShaper.reset();
    performancePipeline.reset();
    outputLimiter.reset();
    fxChain.reset();
    fxChain.reset();
}

bool AviatorKeyzProcessor::isBusesLayoutSupported (const AudioProcessor::BusesLayout& layouts) const
{
    if (layouts.getMainInputChannelSet() != AudioChannelSet::disabled())
        return false;

    return layouts.getMainOutputChannelSet() == AudioChannelSet::stereo();
}

void AviatorKeyzProcessor::processBlock (AudioBuffer<float>& buffer,
                                          MidiBuffer&         midiMessages)
{
    ScopedNoDenormals noDenormals;

    buffer.clear();

    if (! isPrepared)
        return;

    const int n = buffer.getNumSamples();

    namespace P = AviatorKeyz::ParamID;

    const double hostBpm = [this] {
        if (auto* playHead = getPlayHead())
            if (auto pos = playHead->getPosition())
                if (auto bpm = pos->getBpm())
                    return *bpm;
        return 120.0;
    }();
    lastKnownHostBpm.store (hostBpm, std::memory_order_relaxed);

    EngineState baseState = PerformanceApvtsReader::readBaseState (perfParamCache);
    EngineState engineState = MacroMapper::applyMacros (baseState, macroControls, perfParamCache);

    inputGainSmoothed.setTargetValue (
        Decibels::decibelsToGain (apvts.getRawParameterValue (P::INPUT_GAIN)->load()
                                  + engineState.outputLevelOffset * 6.f));
    outputGainSmoothed.setTargetValue (
        Decibels::decibelsToGain (apvts.getRawParameterValue (P::OUTPUT_GAIN)->load()
                                  + engineState.outputLevelOffset * 6.f));

    const bool reverse = engineState.source.reverse
                         || apvts.getRawParameterValue (P::REVERSE)->load() > 0.5f;
    const float glideMs = apvts.getRawParameterValue (P::GLIDE_TIME)->load();
    const float attackMs = apvts.getRawParameterValue (P::ENV_ATTACK)->load();
    const float decayMs = apvts.getRawParameterValue (P::ENV_AMP_DECAY)->load();
    const float sustain01 = apvts.getRawParameterValue (P::ENV_AMP_SUSTAIN)->load();
    const float releaseMs = apvts.getRawParameterValue (P::ENV_RELEASE)->load();

    const int polyphony = static_cast<int> (apvts.getRawParameterValue (P::VOICE_POLYPHONY)->load());
    const int playMode = static_cast<int> (apvts.getRawParameterValue (P::VOICE_PLAY_MODE)->load());
    const int glideMode = static_cast<int> (apvts.getRawParameterValue (P::VOICE_GLIDE_MODE)->load());

    samplerEngine.setEnvelopeTimesMs (attackMs, decayMs, sustain01, releaseMs);
    samplerEngine.setVelocitySensitivity (apvts.getRawParameterValue (P::VELOCITY_SENSITIVITY)->load());
    samplerEngine.setPolyphony (polyphony);
    samplerEngine.setPlayMode (playMode);
    samplerEngine.setGlideMode (glideMode);
    samplerEngine.setSourceSettings (engineState.source, hostBpm);
    samplerEngine.setPlaybackContext (presetManager->getCurrentSoundType(),
                                      presetManager->getCurrentCategory());

    const int sampleFrames = samplerEngine.getPrimarySampleNumFrames();
    const auto chopPlayback = performancePipeline.advanceChopPlayback (engineState, sampleFrames, hostBpm);
    samplerEngine.setChopPlaybackState (chopPlayback);

    midiHandler.process (midiMessages, samplerEngine, synthEngine, reverse, glideMs, 0.f);

    // Render voices directly into the host buffer (cleared above) — no
    // scratch copy, no audio-thread buffer resizing.
    samplerEngine.process (buffer);

#if AVIATORKEYZ_DEBUG
    PlaybackProbe::updateSamplerVoices (samplerEngine.getNumActiveVoices(),
                                        samplerEngine.getRetriggerPolicy() == RetriggerPolicy::PhraseChoke
                                            ? samplerEngine.getNumActiveVoices() : 0);
    PlaybackProbe::updatePeak (PlaybackProbe::samplerPeak, PlaybackProbe::bufferPeak (buffer));
#endif

    for (int i = 0; i < n; ++i)
    {
        const float gIn = inputGainSmoothed.getNextValue();
        buffer.getWritePointer (0)[i] *= gIn;
        buffer.getWritePointer (1)[i] *= gIn;
    }

    performancePipeline.process (buffer, engineState, hostBpm);

#if AVIATORKEYZ_DEBUG
    PlaybackProbe::updatePeak (PlaybackProbe::texturePeak, PlaybackProbe::bufferPeak (buffer));
#endif

    auto perfFx = performancePipeline.updatePerformanceFx (engineState);
    performancePipeline.applyPerformanceFx (buffer, perfFx);

    const float tone = juce::jlimit (-1.f, 1.f,
        apvts.getRawParameterValue (P::TONE)->load() + engineState.toneOffset);
    const float smear = juce::jlimit (0.f, 1.f,
        apvts.getRawParameterValue (P::SMEAR)->load() + engineState.smearOffset);
    const float revAmt = juce::jlimit (0.f, 1.f,
        apvts.getRawParameterValue (P::REVERB_AMOUNT)->load() + engineState.reverbMixOffset);
    const float revSize = juce::jlimit (0.f, 1.f, apvts.getRawParameterValue (P::REVERB_SIZE)->load());
    const float brightness = juce::jlimit (0.f, 1.f,
        apvts.getRawParameterValue (P::STEREO_WIDTH)->load() + engineState.widthOffset);
    const float pan = juce::jlimit (-1.f, 1.f, apvts.getRawParameterValue (P::PAN)->load());
    const bool reverbOn = apvts.getRawParameterValue (P::FX_REVERB_ON)->load() > 0.5f
                          && revAmt >= 0.001f;
    const float reverbDamp = apvts.getRawParameterValue (P::FX_REVERB_DAMP)->load();

    const float brightnessTone = (brightness - 0.5f) * 2.f;
    if (std::abs (brightnessTone) >= 0.001f)
    {
        brightnessShaper.process (buffer, brightnessTone);
#if JUCE_DEBUG
        printBufferLevel ("05 brightness", buffer);
#endif
    }

    if (std::abs (tone) >= 0.001f)
    {
        toneShaper.process (buffer, tone);
#if JUCE_DEBUG
        printBufferLevel ("05 drive", buffer);
#endif
    }

    if (smear >= 0.001f)
    {
        smearProcessor.process (buffer, smear);
#if JUCE_DEBUG
        printBufferLevel ("05 filter macro", buffer);
#endif
    }

    if (reverbOn)
    {
        reverbTail.process (buffer, revAmt, revSize, true, reverbDamp);
#if JUCE_DEBUG
        printBufferLevel ("07 reverb", buffer);
#endif
    }

    const bool delayOn   = apvts.getRawParameterValue (P::FX_DELAY_ON)->load() > 0.5f;
    const float delayTime = apvts.getRawParameterValue (P::FX_DELAY_TIME)->load();
    const float delayFb  = apvts.getRawParameterValue (P::FX_DELAY_FEEDBACK)->load();
    const float delayMix = juce::jlimit (0.f, 1.f,
        apvts.getRawParameterValue (P::FX_DELAY_MIX)->load() + engineState.delayMixOffset);
    const bool delaySync = apvts.getRawParameterValue (P::FX_DELAY_SYNC)->load() > 0.5f;

    const bool chorusOn  = apvts.getRawParameterValue (P::FX_CHORUS_ON)->load() > 0.5f;
    const float chorusRate  = apvts.getRawParameterValue (P::FX_CHORUS_RATE)->load();
    const float chorusDepth = apvts.getRawParameterValue (P::FX_CHORUS_DEPTH)->load();
    const float chorusMix   = apvts.getRawParameterValue (P::FX_CHORUS_MIX)->load();

    const bool lofiOn    = apvts.getRawParameterValue (P::FX_LOFI_ON)->load() > 0.5f;
    const float lofiAmt  = apvts.getRawParameterValue (P::FX_LOFI_AMOUNT)->load();

    const bool distOn    = apvts.getRawParameterValue (P::FX_DIST_ON)->load() > 0.5f;
    const float distDrv  = juce::jlimit (0.f, 1.f,
        apvts.getRawParameterValue (P::FX_DIST_DRIVE)->load() + engineState.driveOffset);

    const bool fxActive = delayOn || chorusOn || lofiOn || distOn;

    if (fxActive)
    {
        fxChain.process (buffer,
                         delayOn, delayTime, delayFb, delayMix, delaySync,
                         chorusOn, chorusRate, chorusDepth, chorusMix,
                         lofiOn, lofiAmt,
                         distOn, distDrv,
                         hostBpm);
#if JUCE_DEBUG
        printBufferLevel ("07 FX", buffer);
#endif
    }

#if AVIATORKEYZ_DEBUG
    PlaybackProbe::updatePeak (PlaybackProbe::fxPeak, PlaybackProbe::bufferPeak (buffer));
#endif

    const float p = juce::jlimit (-1.f, 1.f, pan);
    float panGainL = 0.f;
    float panGainR = 0.f;
    AviatorFastMath::constantPowerPan (p, panGainL, panGainR);
    panGainL *= kSqrt2;
    panGainR *= kSqrt2;

    outputLimiter.process (buffer, apvts.getRawParameterValue (P::OUTPUT_LIMITER)->load() > 0.5f);

    auto* L = buffer.getWritePointer (0);
    auto* R = buffer.getWritePointer (1);

    for (int i = 0; i < n; ++i)
    {
        float l = L[i];
        float r = R[i];

        l *= panGainL;
        r *= panGainR;

        const float gOut = outputGainSmoothed.getNextValue();
        L[i] = l * gOut;
        R[i] = r * gOut;
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
    state.setProperty (AviatorKeyz::PresetKey::SOUND_TYPE,
                       AviatorKeyz::soundTypeToString (presetManager->getCurrentSoundType()), nullptr);
    state.setProperty (AviatorKeyz::PresetKey::ORIGINAL_BPM, presetManager->getCurrentOriginalBpm(), nullptr);
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
    const auto soundType = AviatorKeyz::soundTypeFromString (
        state.getProperty (AviatorKeyz::PresetKey::SOUND_TYPE,
                           AviatorKeyz::soundTypeToString (presetManager->getCurrentSoundType())).toString());
    const float originalBpm = static_cast<float> (state.getProperty (
        AviatorKeyz::PresetKey::ORIGINAL_BPM, presetManager->getCurrentOriginalBpm()));

    presetManager->setPresetIdentity (category, name, sampleId, rootNote, soundType, originalBpm);
    AviatorKeyz::applyPlaybackPolicyToApvts (apvts, category, soundType);
    AviatorKeyz::syncOriginalBpmToApvts (apvts, originalBpm > 1.f ? originalBpm : 120.f);
    AviatorKeyz::syncRootNoteToApvts (apvts, rootNote);

    loadFactorySample (sampleId, rootNote);

    // Notify UI listeners (editor/cockpit) so selection matches restored identity.
    if (presetManager->onPresetLoaded)
        presetManager->onPresetLoaded (category, name, sampleId, presetManager->getCurrentRootNote());
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AviatorKeyzProcessor();
}
