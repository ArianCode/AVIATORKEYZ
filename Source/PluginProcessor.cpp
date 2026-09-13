#include "PluginProcessor.h"
#if ! AVIATORKEYZ_HEADLESS_TESTS
 #include "PluginEditor.h"
#endif
#include "State/ParameterLayout.h"
#include "State/ApvtsStateHelpers.h"
#include "State/CategorySoundPolicy.h"
#include "State/SampleAnalysis.h"
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

// Private MIDI protocol on the timeline buffer (channel 16 is never produced by
// the arpeggiator for real notes and host notes are consumed while the arp runs):
//   note on/off  ch16, note 36 + slice  -> arp SLICES step
//   CC 119       ch16                   -> MANEUVER lever flip (value >= 64 = REV)
constexpr int kTimelineChannel = 16;
constexpr int kFlipController = 119;

double alignUpToGrid (double beat, double grid) noexcept
{
    if (grid <= 0.0)
        return beat;
    return std::ceil (beat / grid - 1.0e-9) * grid;
}

double flipSnapBeats (int snapIndex) noexcept
{
    switch (snapIndex)
    {
        case 1:  return 1.0;   // 1/4
        case 2:  return 0.5;   // 1/8
        case 3:  return 0.25;  // 1/16
        default: return 0.0;   // off
    }
}
} // namespace

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
    mfxRack.attachParameters (apvts);

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

    lastReverseParam = apvts.getRawParameterValue (ParamID::REVERSE)->load() > 0.5f;
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
    brightnessShaper.prepare (spec);
    smearProcessor.prepare (spec);
    reverbTail.prepare (spec);
    fxChain.prepare (spec);
    performancePipeline.prepare (spec);
    arpeggiator.prepare (sampleRate);
    mfxRack.prepare (spec);
    stretchPlayer.prepare (sampleRate, samplesPerBlock);
    filteredMidi.ensureSize (4096);

    // Timeline buffer capacity: host MIDI + arp events for one block. Reserved
    // here so addEvent() never allocates in processBlock.
    timelineMidi.ensureSize (4096);
    timelineMidi.clear();
    arpWasOn = false;
    flipPending = false;
    liveReverseApplied = false;
    lastReverseParam = apvts.getRawParameterValue (ParamID::REVERSE)->load() > 0.5f;

    // Hosts (FL Studio in particular) run setStateInformation -> releaseResources ->
    // prepareToPlay when reopening a project or starting an offline render.
    // releaseResources() nulls the sampler snapshot; without the re-attach below the
    // engine comes back with no region and startVoice() falls through to its sine
    // fallback — the project reopens with the right preset name but the wrong sound.
    const auto sampleId = presetManager->getCurrentSampleId();
    if (sampleId.isNotEmpty())
    {
        if (! samplerEngine.hasLoadedSample())
        {
            if (sampleId == loadedSampleId)
            {
                // sampleLibrary still owns the published buffer — re-attach the pointer
                // without a full decode/reload.
                samplerEngine.setSampleSnapshot (sampleLibrary.getPublishedSnapshot());
            }

            if (! samplerEngine.hasLoadedSample())
                loadFactorySample (sampleId, presetManager->getCurrentRootNote());
        }
        else if (sampleId != loadedSampleId)
        {
            loadFactorySample (sampleId, presetManager->getCurrentRootNote());
        }
    }

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

// -----------------------------------------------------------------------------
//  Sample loading
// -----------------------------------------------------------------------------
bool AviatorKeyzProcessor::installSampleFromLibrary (const juce::String& sampleId)
{
    // Caller holds sampleLoadLock and has suspended processing; pendingMap is loaded.
    sampleLibrary.publish();
    samplerEngine.setSampleSnapshot (sampleLibrary.getPublishedSnapshot());
    stretchPlayer.allSoundOff();
    {
        const auto* snap = sampleLibrary.getPublishedSnapshot();
        stretchPlayer.setRegion (snap != nullptr && ! snap->regions.empty() ? &snap->regions.front() : nullptr);
    }

    factoryRootNote = sampleLibrary.getPrimaryRootNote();
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
    return true;
}

bool AviatorKeyzProcessor::loadFactorySample (const juce::String& sampleId, int rootNote)
{
    {
        const juce::ScopedLock lock (sampleLoadLock);
        // Root is resolved from the sample (smpl > argument); identity is sampleId.
        if (sampleId == loadedSampleId)
        {
            if (! samplerEngine.hasLoadedSample())
                samplerEngine.setSampleSnapshot (sampleLibrary.getPublishedSnapshot());
            {
                const auto* snap = sampleLibrary.getPublishedSnapshot();
                stretchPlayer.setRegion (snap != nullptr && ! snap->regions.empty() ? &snap->regions.front() : nullptr);
            }

            const int authoritativeRoot = sampleLibrary.getPrimaryRootNote();
            factoryRootNote = authoritativeRoot;
            presetManager->setCurrentRootNote (authoritativeRoot);
            AviatorKeyz::syncRootNoteToApvts (apvts, authoritativeRoot);
            juce::ignoreUnused (rootNote);
            return samplerEngine.hasLoadedSample();
        }
    }

    if (sampleId.isEmpty())
    {
        AK_LOG ("Preset contains an empty sampleId");
        return false;
    }

    if (isUserSampleId (sampleId))
    {
        juce::String error;
        const juce::File file (sampleId.fromFirstOccurrenceOf ("user:", false, false));
        if (! loadUserSample (file, error))
        {
            AK_LOG ("User sample restore failed: " + error);
            return false;
        }
        return true;
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
            installSampleFromLibrary (sampleId);
            userSampleInfo = {};
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

    if (onUserSampleChanged)
        onUserSampleChanged();

    return samplerValid;
}

bool AviatorKeyzProcessor::loadUserSample (const juce::File& file, juce::String& errorOut)
{
    errorOut.clear();

    if (! file.existsAsFile())
    {
        errorOut = "File not found: " + file.getFileName();
        return false;
    }

    const auto ext = file.getFileExtension().toLowerCase();
    if (ext == ".mp3" || ext == ".m4a" || ext == ".aac" || ext == ".ogg")
    {
        errorOut = "Compressed audio is not accepted as cargo. Drop a WAV or AIFF.";
        return false;
    }

    // Pre-flight: length cap + analysis on a private reader (no audio-thread impact).
    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> reader (formatManager.createReaderFor (file));
    if (reader == nullptr || reader->sampleRate <= 0.0 || reader->lengthInSamples <= 0)
    {
        errorOut = "Could not read " + file.getFileName() + " (WAV / AIFF only).";
        return false;
    }

    const double seconds = (double) reader->lengthInSamples / reader->sampleRate;
    if (seconds > kMaxUserSampleSeconds + 0.01)
    {
        errorOut = "Cargo too heavy: " + juce::String (seconds, 1) + " s (max "
                   + juce::String ((int) kMaxUserSampleSeconds) + " s).";
        return false;
    }

    SampleAnalysis::Result analysis;
    {
        const int numFrames = (int) reader->lengthInSamples;
        const int numCh = (int) juce::jmax (1u, reader->numChannels);
        juce::AudioBuffer<float> temp (juce::jmin (2, numCh), numFrames);
        temp.clear();
        if (reader->read (&temp, 0, numFrames, 0, true, true))
        {
            juce::HeapBlock<float> mono (static_cast<size_t> (numFrames));
            if (temp.getNumChannels() > 1)
            {
                for (int i = 0; i < numFrames; ++i)
                    mono[i] = 0.5f * (temp.getSample (0, i) + temp.getSample (1, i));
            }
            else
            {
                juce::FloatVectorOperations::copy (mono.getData(), temp.getReadPointer (0), numFrames);
            }
            analysis = SampleAnalysis::analyzeMono (mono.getData(), numFrames, reader->sampleRate);
        }
    }

    const int rootNote = SampleAnalysis::rootNoteForPitchClass (analysis.keyPitchClass);
    const auto sampleId = userSampleIdForFile (file);

    suspendProcessing (true);
    bool loaded = false;
    {
        const juce::ScopedLock lock (sampleLoadLock);

        samplerEngine.allSoundOff();
        sampleLibrary.clearAll();

        if (! sampleLibrary.loadSample (file, rootNote))
        {
            errorOut = sampleLibrary.getLastError();
        }
        else
        {
            installSampleFromLibrary (sampleId);

            userSampleInfo.loaded = true;
            userSampleInfo.name = file.getFileNameWithoutExtension();
            userSampleInfo.path = file.getFullPathName();
            userSampleInfo.seconds = seconds;
            userSampleInfo.fileSampleRate = reader->sampleRate;
            userSampleInfo.detectedBpm = analysis.bpm;
            userSampleInfo.keyPitchClass = analysis.keyPitchClass;
            userSampleInfo.keyMinor = analysis.keyMinor;
            userSampleInfo.rootNote = rootNote;
            loaded = true;
        }
    }
    suspendProcessing (false);

    if (! loaded)
    {
        // The previous sample is gone; fall back to the preset's factory sample.
        const auto previous = presetManager->getCurrentSampleId();
        loadedSampleId.clear();
        if (previous.isNotEmpty() && ! isUserSampleId (previous))
            loadFactorySample (previous, presetManager->getCurrentRootNote());
        return false;
    }

    // Playback policy for the dropped file: long material loops in sync with the
    // host at its own pitch; short hits behave like a chromatic one-shot.
    const bool phraseLike = seconds >= 2.0;
    const auto soundType = phraseLike ? AviatorKeyz::SoundType::Loop : AviatorKeyz::SoundType::OneShot;
    const float bpm = analysis.bpm > 1.f ? analysis.bpm : 120.f;

    presetManager->setPresetIdentity (presetManager->getCurrentCategory(),
                                      file.getFileNameWithoutExtension(),
                                      sampleId, rootNote, soundType, bpm);
    AviatorKeyz::applyPlaybackPolicyToApvts (apvts, presetManager->getCurrentCategory(), soundType);
    AviatorKeyz::syncOriginalBpmToApvts (apvts, bpm);
    AviatorKeyz::syncRootNoteToApvts (apvts, rootNote);

    auto setNorm = [&] (const char* id, float value)
    {
        if (auto* p = apvts.getParameter (id))
            p->setValueNotifyingHost (p->convertTo0to1 (value));
    };
    setNorm (ParamID::SRC_START, 0.f);
    setNorm (ParamID::SRC_END, 1.f);
    setNorm (ParamID::SRC_BPM_SYNC, phraseLike && analysis.bpm > 1.f ? 1.f : 0.f);

    AK_LOG ("loadUserSample: " + file.getFileName() + " " + juce::String (seconds, 2) + "s bpm="
            + juce::String (analysis.bpm, 1) + " key=" + SampleAnalysis::keyName (analysis.keyPitchClass, analysis.keyMinor));

    if (onUserSampleChanged)
        onUserSampleChanged();
    if (presetManager->onPresetLoaded)
        presetManager->onPresetLoaded (presetManager->getCurrentCategory(),
                                       presetManager->getCurrentPresetName(),
                                       sampleId, rootNote);
    return true;
}

void AviatorKeyzProcessor::releaseResources()
{
    isPrepared = false;
    samplerEngine.releaseResources();
    synthEngine.releaseResources();
    filterProcessor.reset();
    textureEngine.reset();
    smearProcessor.reset();
    reverbTail.reset();
    toneShaper.reset();
    brightnessShaper.reset();
    performancePipeline.reset();
    outputLimiter.reset();
    fxChain.reset();
    arpeggiator.reset();
    mfxRack.reset();
    stretchPlayer.reset();
}

bool AviatorKeyzProcessor::isBusesLayoutSupported (const AudioProcessor::BusesLayout& layouts) const
{
    if (layouts.getMainInputChannelSet() != AudioChannelSet::disabled())
        return false;

    return layouts.getMainOutputChannelSet() == AudioChannelSet::stereo();
}

// -----------------------------------------------------------------------------
//  Flight Deck helpers
// -----------------------------------------------------------------------------
Arp::Settings AviatorKeyzProcessor::readArpSettings() const noexcept
{
    namespace P = AviatorKeyz::ParamID;
    auto raw = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };

    Arp::Settings s;
    s.on        = raw (P::ARP_ON) > 0.5f;
    s.mode      = static_cast<Arp::Mode> (juce::jlimit (0, 4, (int) raw (P::ARP_MODE)));
    s.rate      = static_cast<Arp::Rate> (juce::jlimit (0, 3, (int) raw (P::ARP_RATE)));
    s.feel      = static_cast<Arp::Feel> (juce::jlimit (0, 2, (int) raw (P::ARP_FEEL)));
    s.octaves   = juce::jlimit (1, 4, (int) raw (P::ARP_OCTAVES));
    s.gate      = raw (P::ARP_GATE);
    s.swing     = raw (P::ARP_SWING);
    s.humanize  = raw (P::ARP_HUMANIZE);
    s.octSpread = raw (P::ARP_OCT_SPREAD);
    s.hold      = raw (P::ARP_HOLD) > 0.5f;
    s.target    = static_cast<Arp::Target> (juce::jlimit (0, 1, (int) raw (P::ARP_TARGET)));
    return s;
}

int AviatorKeyzProcessor::flipWindowFrames (int sampleFrames, double hostBpm) const noexcept
{
    const int window = (int) apvts.getRawParameterValue (ParamID::FLIP_WINDOW)->load();
    if (window == 1) // SLICE: one of 16 slices of the source window
    {
        const float start = apvts.getRawParameterValue (ParamID::SRC_START)->load();
        const float end = apvts.getRawParameterValue (ParamID::SRC_END)->load();
        const int windowLen = juce::jmax (1, (int) ((end - start) * (float) (sampleFrames - 1)));
        return juce::jmax (64, windowLen / ParamID::ARP_NUM_SLICES);
    }
    if (window == 2) // BEAT: one host beat, in sample frames of the loaded file
    {
        const double fileRate = samplerEngine.getPrimarySampleRate();
        return juce::jmax (64, (int) (60.0 / juce::jmax (20.0, hostBpm) * fileRate));
    }
    return 0; // PHRASE
}

void AviatorKeyzProcessor::renderVoiceSegment (juce::AudioBuffer<float>& buffer, int start, int numSamples, bool synthLayerOn)
{
    if (numSamples <= 0)
        return;

    float* chans[2] = { buffer.getWritePointer (0) + start,
                        buffer.getWritePointer (1) + start };
    juce::AudioBuffer<float> view (chans, 2, numSamples);

    samplerEngine.process (view);
    if (stretchPlayer.isActive())
        stretchPlayer.render (view);
    if (synthLayerOn || synthEngine.hasActiveVoices())
        synthEngine.process (view);
}

void AviatorKeyzProcessor::dispatchTimelineMessage (const juce::MidiMessage& msg,
                                                    bool reverse, float glideMs, float sourceBlend, bool synthLayerOn,
                                                    int sampleFrames, double hostBpm, const SourceSettings& src)
{
    if (msg.getChannel() == kTimelineChannel)
    {
        if (msg.isController() && msg.getControllerNumber() == kFlipController)
        {
            const bool rev = msg.getControllerValue() >= 64;
            const int frames = flipWindowFrames (sampleFrames, hostBpm);
            samplerEngine.setLiveReverse (rev, frames);
            liveReverseApplied = rev;
            lastFlipWindowFrames = frames;
            return;
        }

        if (msg.isNoteOn() || msg.isNoteOff())
        {
            // Slice steps play the sample at its own root (no repitch) inside a
            // 1/16 window of the source range.
            const int rootNote = samplerEngine.getPrimarySampleRootNote();
            if (msg.isNoteOn())
            {
                if (sampleFrames > 1)
                {
                    const int slice = juce::jlimit (0, ParamID::ARP_NUM_SLICES - 1,
                                                    msg.getNoteNumber() - Arp::kSliceBaseNote);
                    const int windowStart = (int) (src.start * (float) (sampleFrames - 1));
                    const int windowEnd = juce::jmax (windowStart + 2, (int) (src.end * (float) (sampleFrames - 1)));
                    const int sliceLen = juce::jmax (2, (windowEnd - windowStart) / ParamID::ARP_NUM_SLICES);
                    const int s = windowStart + slice * sliceLen;
                    const int e = juce::jmin (windowEnd, s + sliceLen);
                    samplerEngine.setNextNoteSliceWindow (s, e);
                }
                midiHandler.handleMessage (juce::MidiMessage::noteOn (1, rootNote, msg.getFloatVelocity()),
                                           samplerEngine, synthEngine, reverse, glideMs, sourceBlend, synthLayerOn);
            }
            else
            {
                midiHandler.handleMessage (juce::MidiMessage::noteOff (1, rootNote),
                                           samplerEngine, synthEngine, reverse, glideMs, sourceBlend, synthLayerOn);
            }
            return;
        }
    }

    // Host note events: STRETCH mode routes the sampler part to the stretch voice;
    // SLICE mode turns every key into a slice pad (C1 = slice 0 of 16).
    if (msg.isNoteOnOrOff() && msg.getChannel() != kTimelineChannel)
    {
        if (stretchModeThisBlock)
        {
            if (msg.isNoteOn() && msg.getVelocity() > 0)
                stretchPlayer.noteOn (msg.getNoteNumber(), msg.getFloatVelocity());
            else
                stretchPlayer.noteOff (msg.getNoteNumber());
            // synth layer still follows the keyboard; sampler is skipped (blend 1.0)
            midiHandler.handleMessage (msg, samplerEngine, synthEngine, reverse, glideMs, 1.0f, synthLayerOn);
            return;
        }
        if (src.playbackMode == SamplePlaybackMode::SlicePhrase && msg.isNoteOn() && msg.getVelocity() > 0 && sampleFrames > 1)
        {
            const int slice = Arp::sliceIndexForNote (msg.getNoteNumber());
            const int windowStart = (int) (src.start * (float) (sampleFrames - 1));
            const int windowEnd = juce::jmax (windowStart + 2, (int) (src.end * (float) (sampleFrames - 1)));
            const int sliceLen = juce::jmax (2, (windowEnd - windowStart) / ParamID::ARP_NUM_SLICES);
            const int s = windowStart + slice * sliceLen;
            const int e = juce::jmin (windowEnd, s + sliceLen);
            samplerEngine.setNextNoteSliceWindow (s, e);
        }
    }
    else if (msg.isAllNotesOff() || msg.isAllSoundOff())
    {
        if (msg.isAllSoundOff()) stretchPlayer.allSoundOff(); else stretchPlayer.allNotesOff();
    }

    midiHandler.handleMessage (msg, samplerEngine, synthEngine, reverse, glideMs, sourceBlend, synthLayerOn);
}

// -----------------------------------------------------------------------------
//  processBlock
// -----------------------------------------------------------------------------
void AviatorKeyzProcessor::processBlock (AudioBuffer<float>& buffer,
                                          MidiBuffer&         midiMessages)
{
    ScopedNoDenormals noDenormals;

    buffer.clear();

    if (! isPrepared)
        return;

    const int n = buffer.getNumSamples();
    if (n <= 0)
        return;

    namespace P = AviatorKeyz::ParamID;

    // --- transport -----------------------------------------------------------
    double hostBpm = 120.0;
    double hostPpq = -1.0;
    bool hostPlaying = false;
    if (auto* playHead = getPlayHead())
    {
        if (auto pos = playHead->getPosition())
        {
            if (auto bpm = pos->getBpm())
                hostBpm = *bpm;
            if (auto ppq = pos->getPpqPosition())
                hostPpq = *ppq;
            hostPlaying = pos->getIsPlaying();
        }
    }
    hostBpm = juce::jmax (20.0, hostBpm);
    lastKnownHostBpm.store (hostBpm, std::memory_order_relaxed);

    const double beatsPerSample = hostBpm / 60.0 / getSampleRate();
    if (hostPlaying && hostPpq >= 0.0)
        transportBeat = hostPpq;
    const double blockStartBeat = transportBeat;

    EngineState baseState = PerformanceApvtsReader::readBaseState (perfParamCache);
    EngineState engineState = MacroMapper::applyMacros (baseState, macroControls, perfParamCache);

    inputGainSmoothed.setTargetValue (
        Decibels::decibelsToGain (apvts.getRawParameterValue (P::INPUT_GAIN)->load()
                                  + engineState.outputLevelOffset * 6.f));
    outputGainSmoothed.setTargetValue (
        Decibels::decibelsToGain (apvts.getRawParameterValue (P::OUTPUT_GAIN)->load()
                                  + engineState.outputLevelOffset * 6.f));

    const bool reverseParam = apvts.getRawParameterValue (P::REVERSE)->load() > 0.5f;
    const bool reverse = engineState.source.reverse || reverseParam;
    const float glideMs = apvts.getRawParameterValue (P::GLIDE_TIME)->load();
    // Envelope OFF = flat: instant attack, full sustain, short release.
    const bool envOn = apvts.getRawParameterValue (P::ENV_ENABLED)->load() > 0.5f;
    const float attackMs = envOn ? apvts.getRawParameterValue (P::ENV_ATTACK)->load() : 0.f;
    // env_amp_decay is stored in seconds (0–10 s); the engines take milliseconds.
    const float decayMs = envOn ? apvts.getRawParameterValue (P::ENV_AMP_DECAY)->load() * 1000.f : 0.f;
    const float sustain01 = envOn ? apvts.getRawParameterValue (P::ENV_AMP_SUSTAIN)->load() : 1.f;
    const float releaseMs = envOn ? apvts.getRawParameterValue (P::ENV_RELEASE)->load() : 10.f;

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

    // --- LAYER MIX: synth oscillator layer -------------------------------------
    const float osc1Level = apvts.getRawParameterValue (P::OSC1_LEVEL)->load();
    const float osc2Level = apvts.getRawParameterValue (P::OSC2_LEVEL)->load();
    const float sourceBlend = apvts.getRawParameterValue (P::SOURCE_BLEND)->load();
    const bool synthLayerOn = osc1Level > 0.001f || osc2Level > 0.001f;

    synthEngine.setOscParams (0,
                              (int) apvts.getRawParameterValue (P::OSC1_TYPE)->load(),
                              apvts.getRawParameterValue (P::OSC1_TUNE)->load(),
                              apvts.getRawParameterValue (P::OSC1_FINE)->load(),
                              apvts.getRawParameterValue (P::OSC1_SHAPE)->load(),
                              osc1Level,
                              apvts.getRawParameterValue (P::OSC1_PAN)->load());
    synthEngine.setOscParams (1,
                              (int) apvts.getRawParameterValue (P::OSC2_TYPE)->load(),
                              apvts.getRawParameterValue (P::OSC2_TUNE)->load(),
                              apvts.getRawParameterValue (P::OSC2_FINE)->load(),
                              apvts.getRawParameterValue (P::OSC2_SHAPE)->load(),
                              osc2Level,
                              apvts.getRawParameterValue (P::OSC2_PAN)->load());
    synthEngine.setAmpEnvelopeMs (attackMs, decayMs, sustain01, releaseMs);
    synthEngine.setFilterEnvelopeMs (apvts.getRawParameterValue (P::ENV_FLT_ATTACK)->load() * 1000.f,
                                     apvts.getRawParameterValue (P::ENV_FLT_DECAY)->load() * 1000.f,
                                     apvts.getRawParameterValue (P::ENV_FLT_SUSTAIN)->load(),
                                     apvts.getRawParameterValue (P::ENV_FLT_RELEASE)->load() * 1000.f);
    synthEngine.setPolyphony (polyphony);
    synthEngine.setPlayMode (static_cast<SynthEngine::PlayMode> (juce::jlimit (0, 2, playMode)));
    synthEngine.setGlideMode (static_cast<SynthEngine::GlideMode> (juce::jlimit (0, 2, glideMode)));

    const int sampleFrames = samplerEngine.getPrimarySampleNumFrames();
    const auto chopPlayback = performancePipeline.advanceChopPlayback (engineState, sampleFrames, hostBpm);
    samplerEngine.setChopPlaybackState (chopPlayback);

    // STRETCH mode: the phrase voice runs through the pitch-preserving stretcher.
    const bool stretchMode = engineState.source.playbackMode == SamplePlaybackMode::PhraseTimeStretch;
    {
        StretchPlayer::Params sp;
        sp.speed = engineState.source.speed;
        sp.bpmSync = engineState.source.bpmSync;
        sp.originalBpm = engineState.source.originalBpm;
        sp.hostBpm = hostBpm;
        sp.tuneSemis = engineState.source.tune;
        sp.keytrack = engineState.source.keytrack;
        sp.start = engineState.source.start;
        sp.end = engineState.source.end;
        sp.loopMode = engineState.source.loopMode;
        sp.reverse = reverse;
        sp.attackMs = attackMs; sp.decayMs = decayMs; sp.sustain = sustain01; sp.releaseMs = releaseMs;
        sp.velocitySensitivity = apvts.getRawParameterValue (P::VELOCITY_SENSITIVITY)->load();
        stretchPlayer.setParams (sp);
        if (! stretchMode && stretchPlayer.isActive())
            stretchPlayer.allNotesOff();
        stretchActiveForUi.store (stretchMode, std::memory_order_relaxed);
    }

    // MIDI flip trigger: a dedicated note throws the lever and is consumed.
    filteredMidi.clear();
    {
        const bool trigOn = apvts.getRawParameterValue (P::FLIP_TRIGGER_ON)->load() > 0.5f;
        const int trigNote = (int) apvts.getRawParameterValue (P::FLIP_TRIGGER_NOTE)->load();
        const bool momentary = apvts.getRawParameterValue (P::FLIP_MODE)->load() > 0.5f;
        auto* reverseParam = apvts.getParameter (P::REVERSE);
        for (const auto meta : midiMessages)
        {
            const auto msg = meta.getMessage();
            if (trigOn && reverseParam != nullptr && msg.isNoteOnOrOff() && msg.getNoteNumber() == trigNote)
            {
                const bool on = msg.isNoteOn() && msg.getVelocity() > 0;
                if (momentary)
                    reverseParam->setValueNotifyingHost (on ? 1.f : 0.f);
                else if (on)
                    reverseParam->setValueNotifyingHost (reverseParam->getValue() > 0.5f ? 0.f : 1.f);
                continue;
            }
            filteredMidi.addEvent (msg, meta.samplePosition);
        }
    }

    // --- MIDI timeline: host events, arpeggiator, flip lever ---------------------
    timelineMidi.clear();

    const auto arpSettings = readArpSettings();
    arpeggiator.setSettings (arpSettings);

    auto addArpEvents = [this] (int count, Arp::Target target)
    {
        for (int i = 0; i < count; ++i)
        {
            const auto& e = arpEvents[i];
            if (target == Arp::Target::slices)
            {
                const int sliceNote = Arp::kSliceBaseNote + e.sliceIndex;
                timelineMidi.addEvent (e.noteOn ? juce::MidiMessage::noteOn (kTimelineChannel, sliceNote, e.velocity)
                                                : juce::MidiMessage::noteOff (kTimelineChannel, sliceNote),
                                       e.samplePos);
            }
            else
            {
                timelineMidi.addEvent (e.noteOn ? juce::MidiMessage::noteOn (1, e.note, e.velocity)
                                                : juce::MidiMessage::noteOff (1, e.note),
                                       e.samplePos);
            }
        }
    };

    if (arpSettings.on)
    {
        // Sustain acts as latch for the arp; note events feed the chord.
        for (const auto meta : filteredMidi)
        {
            const auto msg = meta.getMessage();
            if (msg.isSustainPedalOn())       arpeggiator.setSustain (true);
            else if (msg.isSustainPedalOff()) arpeggiator.setSustain (false);
            else if (! msg.isNoteOnOrOff())
                timelineMidi.addEvent (msg, meta.samplePosition);
        }

        const int count = arpeggiator.process (filteredMidi, n, hostBpm, hostPpq, hostPlaying, arpEvents);
        addArpEvents (count, arpSettings.target);
    }
    else
    {
        if (arpWasOn)
        {
            const int count = arpeggiator.flushAllNotesOff (arpEvents);
            // Target may have changed since the notes started: close both routes.
            addArpEvents (count, Arp::Target::notes);
            addArpEvents (count, Arp::Target::slices);
        }
        for (const auto meta : filteredMidi)
            timelineMidi.addEvent (meta.getMessage(), meta.samplePosition);
    }
    arpWasOn = arpSettings.on;

    // Flip lever: a REVERSE edge is applied at the next snap boundary.
    if (reverseParam != lastReverseParam)
    {
        lastReverseParam = reverseParam;
        flipPending = true;
        flipPendingValue = reverseParam;
    }
    if (flipPending)
    {
        const double snap = flipSnapBeats ((int) apvts.getRawParameterValue (P::FLIP_SNAP)->load());
        int pos = 0;
        if (snap > 0.0)
        {
            const double boundary = alignUpToGrid (blockStartBeat, snap);
            const double offset = (boundary - blockStartBeat) / beatsPerSample;
            pos = offset < (double) n ? (int) offset : -1;
        }
        if (pos >= 0)
        {
            timelineMidi.addEvent (juce::MidiMessage::controllerEvent (kTimelineChannel, kFlipController,
                                                                       flipPendingValue ? 127 : 0), pos);
            flipPending = false;
        }
    }
    else if (liveReverseApplied)
    {
        // Keep the window for new notes current if FLIP WINDOW / tempo changed.
        const int frames = flipWindowFrames (sampleFrames, hostBpm);
        if (frames != lastFlipWindowFrames)
        {
            samplerEngine.setLiveReverse (true, frames);
            lastFlipWindowFrames = frames;
        }
    }
    flipPendingForUi.store (flipPending, std::memory_order_relaxed);

    // --- dispatch events + render voices in sample-accurate segments -----------
    stretchModeThisBlock = stretchMode;
    int cursor = 0;
    for (const auto meta : timelineMidi)
    {
        const int pos = juce::jlimit (cursor, n, meta.samplePosition);
        if (pos > cursor)
        {
            renderVoiceSegment (buffer, cursor, pos - cursor, synthLayerOn);
            cursor = pos;
        }
        dispatchTimelineMessage (meta.getMessage(), reverse, glideMs, sourceBlend, synthLayerOn,
                                 sampleFrames, hostBpm, engineState.source);
    }
    renderVoiceSegment (buffer, cursor, n - cursor, synthLayerOn);
    midiMessages.clear();

    transportBeat = blockStartBeat + n * beatsPerSample;
    transportBeatForUi.store (transportBeat, std::memory_order_relaxed);
    activeVoices.store (samplerEngine.getNumActiveVoices(), std::memory_order_relaxed);

#if AVIATORKEYZ_DEBUG
    PlaybackProbe::updateSamplerVoices (samplerEngine.getNumActiveVoices(),
                                        samplerEngine.getRetriggerPolicy() == RetriggerPolicy::PhraseChoke
                                            ? samplerEngine.getNumActiveVoices() : 0);
    PlaybackProbe::updatePeak (PlaybackProbe::samplerPeak, PlaybackProbe::bufferPeak (buffer));
#endif

    // --- FILTER (main-page HP/LP panel) -------------------------------------
    if (apvts.getRawParameterValue (P::FILTER_ENABLED)->load() > 0.5f)
    {
        filterProcessor.setParameters (apvts.getRawParameterValue (P::FILTER_CUTOFF)->load(),
                                       apvts.getRawParameterValue (P::FILTER_RESONANCE)->load(),
                                       static_cast<FilterProcessor::Type> (
                                           juce::jlimit (0, 3, (int) apvts.getRawParameterValue (P::FILTER_TYPE)->load())),
                                       apvts.getRawParameterValue (P::FILTER_DRIVE)->load(),
                                       apvts.getRawParameterValue (P::ENV_FLT_AMOUNT)->load(),
                                       synthEngine.getFilterEnvLevel());
        filterProcessor.process (buffer);
    }

    for (int i = 0; i < n; ++i)
    {
        const float gIn = inputGainSmoothed.getNextValue();
        buffer.getWritePointer (0)[i] *= gIn;
        buffer.getWritePointer (1)[i] *= gIn;
    }

    // --- LAYER MIX: TEX granular layer (parallel, wet mix = tex_amount) -----
    textureEngine.process (buffer,
                           apvts.getRawParameterValue (P::TEX_ENABLED)->load() > 0.5f,
                           apvts.getRawParameterValue (P::TEX_AMOUNT)->load(),
                           apvts.getRawParameterValue (P::TEX_FREEZE)->load() > 0.5f,
                           apvts.getRawParameterValue (P::TEX_GRAIN_RATE)->load(),
                           apvts.getRawParameterValue (P::TEX_GRAIN_SIZE)->load(),
                           (int) apvts.getRawParameterValue (P::TEX_GRAIN_PITCH)->load(),
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

    // --- MFX rack (slot A -> slot B). The old ATMOSPHERE grain layer lives in
    // the rack now as the Grain Cloud effect; its macro destinations are passed
    // through as offsets so preset macros keep working.
    MfxRack::TextureMacroOffsets texOffsets;
    texOffsets.mix         = engineState.texture.mix - baseState.texture.mix;
    texOffsets.grainSize   = engineState.texture.grainSize - baseState.texture.grainSize;
    texOffsets.density     = engineState.texture.density - baseState.texture.density;
    texOffsets.position    = engineState.texture.position - baseState.texture.position;
    texOffsets.pitchSpread = engineState.texture.pitchSpread - baseState.texture.pitchSpread;
    texOffsets.smear       = engineState.texture.smear - baseState.texture.smear;
    texOffsets.width       = engineState.texture.width - baseState.texture.width;
    engineState.texture.enabled = false; // legacy blend stage bypassed

    performancePipeline.process (buffer, engineState, hostBpm);

    {
        MfxRack::ModSources mods;
        mods.modWheel   = midiHandler.getModWheel();
        mods.velocity   = midiHandler.getLastVelocity();
        mods.aftertouch = midiHandler.getAftertouch();
        mods.envelope   = samplerEngine.getLastVoiceEnvLevel();
        mods.macro1     = apvts.getRawParameterValue (P::PERF_MACRO_1)->load();
        mods.notePitch  = (float) midiHandler.getLastNote() / 127.f;
        const float lfoRate = apvts.getRawParameterValue (P::LFO1_RATE)->load();
        mods.lfo1 = 0.5f + 0.5f * std::sin (lfo1Phase * juce::MathConstants<float>::twoPi);
        lfo1Phase += (float) (lfoRate * n / getSampleRate());
        lfo1Phase -= std::floor (lfo1Phase);

        Mfx::Clock clock;
        clock.bpm = hostBpm;
        clock.beatPos = blockStartBeat;
        clock.sampleRate = getSampleRate();
        mfxRack.process (buffer, clock, mods, texOffsets);
    }

    textureLevel.store (juce::jmax (buffer.getMagnitude (0, 0, n), buffer.getMagnitude (1, 0, n)),
                        std::memory_order_relaxed);

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
    const auto reverbMode = AviationReverb::algorithmFromIndex (
        (int) apvts.getRawParameterValue (P::FX_REVERB_MODE)->load());
    const auto reverbColor = AviationReverb::colorFromIndex (
        (int) apvts.getRawParameterValue (P::FX_REVERB_COLOR)->load());

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

    // Always called: off/amount changes glide to dry instead of cutting the tail.
    reverbTail.process (buffer, revAmt, revSize, reverbOn, reverbDamp, reverbMode, reverbColor);
#if JUCE_DEBUG
    if (reverbTail.isActive())
        printBufferLevel ("07 reverb", buffer);
#endif

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

// -----------------------------------------------------------------------------
//  MFX rack helpers (message thread)
// -----------------------------------------------------------------------------
AviatorKeyzProcessor::MfxSnapshot AviatorKeyzProcessor::captureMfx (int slot) const
{
    MfxSnapshot snap;
    auto raw = [this] (const juce::String& id) { auto* p = apvts.getRawParameterValue (id); return p != nullptr ? p->load() : 0.f; };
    snap.effect = (int) raw (Mfx::effectId (slot));
    for (int i = 0; i < Mfx::kParamsPerSlot; ++i)
        snap.values[(size_t) i] = raw (Mfx::paramId (slot, i));
    snap.send = raw (Mfx::sendId (slot));
    snap.level = raw (Mfx::levelId (slot));
    return snap;
}

void AviatorKeyzProcessor::applyMfx (int slot, const MfxSnapshot& snap)
{
    auto set = [this] (const juce::String& id, float value)
    {
        if (auto* p = apvts.getParameter (id))
        {
            p->beginChangeGesture();
            p->setValueNotifyingHost (p->convertTo0to1 (value));
            p->endChangeGesture();
        }
    };
    set (Mfx::effectId (slot), (float) snap.effect);
    for (int i = 0; i < Mfx::kParamsPerSlot; ++i)
        set (Mfx::paramId (slot, i), snap.values[(size_t) i]);
    set (Mfx::sendId (slot), snap.send);
    set (Mfx::levelId (slot), snap.level);
}

void AviatorKeyzProcessor::pushMfxHistory (int slot)
{
    auto& h = mfxHistory[(size_t) juce::jlimit (0, Mfx::kNumSlots - 1, slot)];
    h.push_back (captureMfx (slot));
    if (h.size() > kMfxHistoryDepth)
        h.erase (h.begin());
}

bool AviatorKeyzProcessor::undoMfx (int slot)
{
    auto& h = mfxHistory[(size_t) juce::jlimit (0, Mfx::kNumSlots - 1, slot)];
    if (h.empty())
        return false;
    const auto snap = h.back();
    h.pop_back();
    applyMfx (slot, snap);
    return true;
}

void AviatorKeyzProcessor::setMfxEffect (int slot, Mfx::Effect effect, int presetIndex)
{
    pushMfxHistory (slot);
    auto snap = captureMfx (slot);
    snap.effect = (int) effect;
    snap.values = presetIndex >= 0 ? Mfx::presetNormalised (effect, presetIndex)
                                   : Mfx::defaultsNormalised (effect);
    applyMfx (slot, snap);
}

void AviatorKeyzProcessor::processBlockBypassed (AudioBuffer<float>& buffer,
                                                  MidiBuffer& midiMessages)
{
    buffer.clear();

    const bool reverseNow = apvts.getRawParameterValue (ParamID::REVERSE)->load() > 0.5f;
    const float glideMs = apvts.getRawParameterValue (ParamID::GLIDE_TIME)->load();
    juce::ignoreUnused (reverseNow, glideMs);

    midiHandler.processBypassed (midiMessages);
}

AudioProcessorEditor* AviatorKeyzProcessor::createEditor()
{
   #if AVIATORKEYZ_HEADLESS_TESTS
    // The unit-test target links the processor without the GUI layer.
    return nullptr;
   #else
    return new AviatorKeyzEditor (*this);
   #endif
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
    if (userSampleInfo.loaded)
        state.setProperty ("userSamplePath", userSampleInfo.path, nullptr);
    state.setProperty ("mfxLocks1", (int) mfxLocks[0], nullptr);
    state.setProperty ("mfxLocks2", (int) mfxLocks[1], nullptr);
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
    mfxLocks[0] = (uint32_t) (int) state.getProperty ("mfxLocks1", 0);
    mfxLocks[1] = (uint32_t) (int) state.getProperty ("mfxLocks2", 0);

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

#if ! AVIATORKEYZ_HEADLESS_TESTS
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AviatorKeyzProcessor();
}
#endif
