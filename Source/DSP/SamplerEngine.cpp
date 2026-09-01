#include "SamplerEngine.h"
#include "ReversePlayer.h"
#include "FastMath.h"
#include "../Debug/AviatorDebug.h"
#include <cmath>

SamplerEngine::SamplerEngine()
{
    for (int n = 0; n < 128; ++n)
        for (int s = 0; s < kMaxStackPerNote; ++s)
            noteVoiceStack[n][s] = -1;
}
SamplerEngine::~SamplerEngine() = default;

void SamplerEngine::prepare (const juce::dsp::ProcessSpec& spec)
{
    if (std::abs (spec.sampleRate - sampleRate) > 1e-9)
        allSoundOff();

    sampleRate = spec.sampleRate;

    for (auto& v : voices)
        v.glideEngine.setSampleRate (sampleRate);
}

void SamplerEngine::releaseResources()
{
    allSoundOff();
    sampleSnapshot = nullptr;
}

void SamplerEngine::setSampleSnapshot (const SampleLibrary::AudioSnapshot* snapshot) noexcept
{
    sampleSnapshot = snapshot;
}

void SamplerEngine::setEnvelopeTimesMs (float aMs, float dMs, float s01, float rMs) noexcept
{
    attackMs = juce::jlimit (0.f, 5000.f, aMs);
    decayMs = juce::jlimit (0.f, 10000.f, dMs);
    sustainLevel = juce::jlimit (0.f, 1.f, s01);
    releaseMs = juce::jlimit (0.01f, 10000.f, rMs);
}

void SamplerEngine::setVelocitySensitivity (float sensitivity01) noexcept
{
    velocitySensitivity = juce::jlimit (0.f, 1.f, sensitivity01);
}

float SamplerEngine::calculateVelocityGain (float velocity, float sensitivity) noexcept
{
    velocity = juce::jlimit (0.f, 1.f, velocity);
    sensitivity = juce::jlimit (0.f, 1.f, sensitivity);
    return juce::jmap (sensitivity, 1.f, velocity);
}

void SamplerEngine::setPolyphony (int numVoices) noexcept
{
    maxVoices = juce::jlimit (1, kMaxVoices, numVoices);
}

void SamplerEngine::setPlayMode (int mode) noexcept
{
    playMode = static_cast<PlayMode> (juce::jlimit (0, 2, mode));
}

void SamplerEngine::setGlideMode (int mode) noexcept
{
    glideMode = static_cast<GlideMode> (juce::jlimit (0, 2, mode));
}

void SamplerEngine::setPhraseParams (bool enabled,
                                     float startNorm,
                                     float lengthNorm,
                                     int pitchSemis,
                                     bool loop,
                                     bool tempoSync,
                                     bool keySync,
                                     double hostBpm) noexcept
{
    phraseEnabled = enabled;
    phraseStartNorm = juce::jlimit (0.f, 1.f, startNorm);
    phraseLengthNorm = juce::jlimit (0.01f, 1.f, lengthNorm);
    phrasePitchSemis = juce::jlimit (-24, 24, pitchSemis);
    phraseLoop = loop;
    phraseTempoSync = tempoSync;
    phraseKeySync = keySync;
    phraseHostBpm = juce::jmax (20.0, hostBpm);
}

void SamplerEngine::setSourceSettings (const SourceSettings& settings, double hostBpm) noexcept
{
    sourceSettings = settings;
    sourceHostBpm = juce::jmax (20.0, hostBpm);

    phraseEnabled = usesPhraseWindow();
    phraseStartNorm = settings.start;
    phraseLengthNorm = juce::jmax (0.01f, settings.end - settings.start);
    phrasePitchSemis = static_cast<int> (settings.tune);
    phraseLoop = settings.loopMode == LoopMode::Loop;
    phraseTempoSync = settings.bpmSync;
    phraseKeySync = settings.keytrack;
    phraseHostBpm = settings.bpmSync ? sourceHostBpm : settings.originalBpm;

    updatePlaybackPolicies();

    for (int i = 0; i < maxVoices; ++i)
        if (voices[i].active)
            updateVoicePlaybackRates (voices[i]);
}

void SamplerEngine::setPlaybackContext (AviatorKeyz::SoundType soundType,
                                        const juce::String& category) noexcept
{
    currentSoundType = soundType;
    currentCategory = category;
    updatePlaybackPolicies();
}

void SamplerEngine::updatePlaybackPolicies() noexcept
{
    noteGatePolicy = AviatorKeyz::gatePolicyFor (currentCategory,
                                                 currentSoundType,
                                                 sourceSettings.playbackMode,
                                                 sourceSettings.loopMode);
    retriggerPolicy = AviatorKeyz::retriggerPolicyFor (currentCategory,
                                                       currentSoundType,
                                                       sourceSettings.playbackMode);
}

bool SamplerEngine::usesPhraseWindow() const noexcept
{
    return sourceSettings.end > sourceSettings.start + 0.001f;
}

void SamplerEngine::setChopPlaybackState (const ChopPlaybackState& state) noexcept
{
    chopState = state;

    if (! chopState.active)
        return;

    for (int i = 0; i < maxVoices; ++i)
    {
        auto& v = voices[i];
        if (! v.active || v.sampleNumFrames <= 1)
            continue;

        v.phraseStartFrame = juce::jlimit (0, v.sampleNumFrames - 1, chopState.sliceStartFrame);
        v.phraseEndFrame = juce::jmax (v.phraseStartFrame + 1,
                                       juce::jmin (v.sampleNumFrames - 1, chopState.sliceEndFrame));
        v.chopPitchOffsetSemis = chopState.pitchOffsetSemis;
        if (chopState.stepReverse)
            v.reversed = true;
        updateVoicePlaybackRates (v);
    }
}

int SamplerEngine::getPrimarySampleNumFrames() const noexcept
{
    if (sampleSnapshot == nullptr || sampleSnapshot->regions.empty())
        return 0;
    return sampleSnapshot->regions.front().numFrames;
}

float SamplerEngine::midiNoteToHz (float note) noexcept
{
    return AviatorFastMath::midiNoteToHz (note);
}

int SamplerEngine::findFreeOrStealVoice() noexcept
{
    for (int i = 0; i < maxVoices; ++i)
        if (! voices[i].active)
            return i;

    int releaseCandidate = -1;
    float releaseLvl = 2.f;
    for (int i = 0; i < maxVoices; ++i)
    {
        if (voices[i].envStage == EnvStage::release && voices[i].envLevel < releaseLvl)
        {
            releaseLvl = voices[i].envLevel;
            releaseCandidate = i;
        }
    }
    if (releaseCandidate >= 0)
        return releaseCandidate;

    int best = 0;
    float bestLvl = 2.f;
    for (int i = 0; i < maxVoices; ++i)
    {
        if (voices[i].envLevel < bestLvl)
        {
            bestLvl = voices[i].envLevel;
            best = i;
        }
    }
    return best;
}

void SamplerEngine::clearAllNoteStacks() noexcept
{
    for (int n = 0; n < 128; ++n)
    {
        noteVoiceStackCount[n] = 0;
        for (int s = 0; s < kMaxStackPerNote; ++s)
            noteVoiceStack[n][s] = -1;
    }
}

void SamplerEngine::pushNoteVoice (int midiNote, int voiceIndex) noexcept
{
    const int note = juce::jlimit (0, 127, midiNote);
    if (noteVoiceStackCount[note] >= kMaxStackPerNote)
        return;

    noteVoiceStack[note][noteVoiceStackCount[note]++] = voiceIndex;
}

int SamplerEngine::popNoteVoiceFifo (int midiNote) noexcept
{
    const int note = juce::jlimit (0, 127, midiNote);
    if (noteVoiceStackCount[note] <= 0)
        return -1;

    const int voiceIndex = noteVoiceStack[note][0];
    for (int i = 1; i < noteVoiceStackCount[note]; ++i)
        noteVoiceStack[note][i - 1] = noteVoiceStack[note][i];
    --noteVoiceStackCount[note];
    noteVoiceStack[note][noteVoiceStackCount[note]] = -1;
    return voiceIndex;
}

void SamplerEngine::purgeNoteVoiceFromStack (int midiNote, int voiceIndex) noexcept
{
    const int note = juce::jlimit (0, 127, midiNote);
    for (int i = 0; i < noteVoiceStackCount[note]; ++i)
    {
        if (noteVoiceStack[note][i] == voiceIndex)
        {
            for (int j = i + 1; j < noteVoiceStackCount[note]; ++j)
                noteVoiceStack[note][j - 1] = noteVoiceStack[note][j];
            --noteVoiceStackCount[note];
            noteVoiceStack[note][noteVoiceStackCount[note]] = -1;
            return;
        }
    }
}

void SamplerEngine::chokeSameNoteVoices (int midiNote) noexcept
{
    for (int i = 0; i < maxVoices; ++i)
    {
        auto& v = voices[i];
        if (v.active && v.noteNumber == midiNote)
            chokeVoice (v);
    }
}

void SamplerEngine::resetVoiceState (Voice& v, bool wasActive) noexcept
{
    if (v.noteNumber >= 0 && v.noteNumber < 128)
        purgeNoteVoiceFromStack (v.noteNumber, static_cast<int> (&v - voices));

    if (wasActive)
        --activeVoiceCount;

    v.active = false;
    v.noteNumber = 0;
    v.velocity = 0.f;
    v.phase = 0.f;
    v.readPos = 0.f;
    v.reversed = false;
    v.envStage = EnvStage::idle;
    v.envLevel = 0.f;
    v.envLinearStep = 0.f;
    v.envSegSamplesLeft = 0;
    v.declickGain = 1.f;
    v.declickStep = 0.f;
    v.sampleData = nullptr;
    v.sampleNumFrames = 0;
    v.fileSampleRate = sampleRate;
    v.sampleRootNote = 60;
    v.playbackRates = {};
    v.phraseStartFrame = 0;
    v.phraseEndFrame = 0;
    v.chopPitchOffsetSemis = 0;
    v.voiceInstanceId = 0;
    v.glideEngine.snapToPitch (60.f);
}

void SamplerEngine::chokeVoice (Voice& v) noexcept
{
    if (! v.active)
        return;

    const double chokeS = static_cast<double> (kChokeFadeMs) * 0.001;
    const int n = juce::jmax (1, static_cast<int> (std::round (chokeS * sampleRate)));

    if (v.envLevel <= 0.f)
    {
        resetVoiceState (v, true);
        return;
    }

    v.envStage = EnvStage::release;
    v.envSegSamplesLeft = n;
    v.envLinearStep = -v.envLevel / static_cast<float> (n);
}

void SamplerEngine::chokeActiveVoicesForPhrase() noexcept
{
    for (int i = 0; i < maxVoices; ++i)
        if (voices[i].active)
            chokeVoice (voices[i]);
}

void SamplerEngine::startVoice (Voice& v,
                                 int midiNote,
                                 float velocity,
                                 bool reverse,
                                 float glideTimeMs,
                                 const SampleLibrary::AudioRegion* region,
                                 bool declickFadeIn) noexcept
{
    const bool wasActive = v.active;

    const bool legatoOverlap = ! wasActive && activeVoiceCount > 0;
    const bool legatoReuse = wasActive;
    v.active = true;
    if (! wasActive)
        ++activeVoiceCount;
    v.noteNumber = midiNote;
    v.velocity = calculateVelocityGain (velocity, velocitySensitivity);
    v.reversed = reverse;
    v.phase = 0.f;
    v.chopPitchOffsetSemis = 0;

    // Reusing an audible voice (steal) or replacing a choked mono voice can
    // otherwise start with a hard onset; ramp the new note in over ~1.5 ms.
    const bool needsFade = declickFadeIn || wasActive;
    v.declickGain = needsFade ? 0.f : 1.f;
    v.declickStep = needsFade
                        ? 1.f / juce::jmax (1.f, static_cast<float> (kDeclickFadeMs * 0.001f * sampleRate))
                        : 0.f;

    const bool glideTimeOn = glideTimeMs > 1.f;
    const bool useGlide = glideTimeOn
                          && lastNoteForGlide >= 0
                          && (glideMode != GlideMode::legato || legatoOverlap || legatoReuse);

    if (useGlide)
    {
        v.glideEngine.snapToPitch (static_cast<float> (lastNoteForGlide));
        v.glideEngine.noteOn (midiNote, glideTimeMs);
    }
    else
    {
        v.glideEngine.noteOn (midiNote, 0.f);
    }

    if (region != nullptr && region->data != nullptr && region->numFrames > 1)
    {
        v.sampleData      = region->data;
        v.sampleNumFrames = region->numFrames;
        v.sampleRootNote  = region->rootNote;
        v.fileSampleRate  = region->fileSampleRate > 0.0 ? region->fileSampleRate : sampleRate;

        if (phraseEnabled)
        {
            v.phraseStartFrame = static_cast<int> (phraseStartNorm * static_cast<float> (v.sampleNumFrames - 1));
            v.phraseEndFrame = juce::jmin (v.sampleNumFrames - 1,
                                             v.phraseStartFrame + static_cast<int> (phraseLengthNorm
                                                                                    * static_cast<float> (v.sampleNumFrames - v.phraseStartFrame - 1)));
            v.readPos = reverse ? static_cast<float> (v.phraseEndFrame) : static_cast<float> (v.phraseStartFrame);
        }
        else
        {
            v.phraseStartFrame = 0;
            v.phraseEndFrame = v.sampleNumFrames - 1;
            v.readPos = reverse ? static_cast<float> (v.sampleNumFrames - 1) : 0.f;
        }
    }
    else
    {
        AK_LOG ("SamplerEngine: sine fallback — no sample region for note "
                + juce::String (midiNote)
                + (region == nullptr ? " (no region)" : " (empty region)"));
        v.sampleData      = nullptr;
        v.sampleNumFrames = 0;
        v.sampleRootNote  = 60;
        v.fileSampleRate  = sampleRate;
        v.readPos = 0.f;
        v.phraseStartFrame = 0;
        v.phraseEndFrame = 0;
        v.playbackRates = {};
    }

    updateVoicePlaybackRates (v);

    // Publish a POD diagnostic outside any heap/lock path for off-thread inspection.
    {
        NotePitchDiag diag;
        diag.midiNote = midiNote;
        diag.rootNote = v.sampleRootNote;
        diag.keytrack = sourceSettings.keytrack;
        diag.playbackMode = static_cast<int> (getEffectivePlaybackMode());
        diag.semitoneOffset = v.playbackRates.pitchSemitones;
        diag.pitchRatio = static_cast<float> (v.playbackRates.pitchRatio);
        diag.sourceRateRatio = static_cast<float> (v.playbackRates.sourceRateRatio);
        diag.timeRatio = static_cast<float> (v.playbackRates.timeRatio);
        diag.finalIncrement = voiceReadIncrement (v);
        diag.voiceIndex = static_cast<int> (&v - voices);
        diag.sequence = notePitchDiagSequence.fetch_add (1, std::memory_order_relaxed) + 1;
        lastNotePitchDiag = diag;

        AK_LOG ("NotePitch: midi=" + juce::String (diag.midiNote)
                + " root=" + juce::String (diag.rootNote)
                + " keytrack=" + juce::String (static_cast<int> (diag.keytrack))
                + " mode=" + juce::String (diag.playbackMode)
                + " semis=" + juce::String (diag.semitoneOffset, 2)
                + " ratio=" + juce::String (diag.pitchRatio, 4)
                + " srcRate=" + juce::String (diag.sourceRateRatio, 4)
                + " inc=" + juce::String (diag.finalIncrement, 4)
                + " voice=" + juce::String (diag.voiceIndex));
    }

    lastNoteForGlide = midiNote;

    const double attS = attackMs * 0.001;
    if (attS <= 0.0)
    {
        // Zero attack: enter the post-attack stage with a fully initialised
        // segment. (Previously this left envLinearStep/envSegSamplesLeft
        // stale from the voice's prior life, so the level jumped
        // non-deterministically to sustain.)
        finishAttack (v);
    }
    else
    {
        v.envStage = EnvStage::attack;
        v.envLevel = 0.f;
        const int n = juce::jmax (1, static_cast<int> (std::round (attS * sampleRate)));
        v.envSegSamplesLeft = n;
        v.envLinearStep = 1.f / static_cast<float> (n);
    }
}

void SamplerEngine::finishAttack (Voice& v) noexcept
{
    v.envLevel = 1.f;

    if (decayMs <= 0.f || std::abs (sustainLevel - 1.f) < 1.0e-6f)
    {
        v.envStage = EnvStage::sustain;
        v.envLinearStep = 0.f;
        v.envSegSamplesLeft = 0;
    }
    else
    {
        v.envStage = EnvStage::decay;
        const double decS = decayMs * 0.001;
        const int n = juce::jmax (1, static_cast<int> (std::round (decS * sampleRate)));
        v.envSegSamplesLeft = n;
        v.envLinearStep = (sustainLevel - 1.f) / static_cast<float> (n);
    }
}

void SamplerEngine::enterRelease (Voice& v) noexcept
{
    if (! v.active) return;

    const double relS = releaseMs * 0.001;
    if (relS <= 0.0 || v.envLevel <= 0.f)
    {
        resetVoiceState (v, true);
        return;
    }

    v.envStage = EnvStage::release;
    const int n = juce::jmax (1, static_cast<int> (std::round (relS * sampleRate)));
    v.envSegSamplesLeft = n;
    v.envLinearStep = -v.envLevel / static_cast<float> (n);
}

void SamplerEngine::advanceGlide (Voice& v) noexcept
{
    if (! v.glideEngine.isGliding())
        return;

    v.glideEngine.tick();
    updateVoicePlaybackRates (v);
}

SamplePlaybackMode SamplerEngine::getEffectivePlaybackMode() const noexcept
{
    if (chopState.active)
        return SamplePlaybackMode::SlicePhrase;

    return sourceSettings.playbackMode;
}

void SamplerEngine::updateVoicePlaybackRates (Voice& v) noexcept
{
    PlaybackRates rates;

    if (v.sampleData == nullptr || v.sampleNumFrames <= 1)
    {
        v.playbackRates = rates;
        return;
    }

    const auto mode = getEffectivePlaybackMode();
    const double fileRate = v.fileSampleRate > 0.0 ? v.fileSampleRate : sampleRate;
    rates.sourceRateRatio = fileRate / sampleRate;

    // Authoritative root is the loaded sample region — never APVTS defaults.
    const int rootNote = v.sampleRootNote;
    const float staticPitchSemis = static_cast<float> (phrasePitchSemis + v.chopPitchOffsetSemis);
    // Keytrack is the sole runtime MIDI-pitch switch. Category policy turns it
    // on for ChromaticResample presets; users can disable it to lock pitch, or
    // enable it on PhraseOriginal / OneShotOriginal sounds.
    const bool tracksMidiPitch = sourceSettings.keytrack;

    if (tracksMidiPitch)
    {
        const float midiOffset = v.glideEngine.getCurrentPitchSemitones() - static_cast<float> (rootNote);
        rates.pitchSemitones = midiOffset + staticPitchSemis;
        rates.pitchRatio = AviatorFastMath::semitoneRatio (rates.pitchSemitones);
    }
    else if (mode == SamplePlaybackMode::OneShotOriginal)
    {
        rates.pitchSemitones = staticPitchSemis;
        if (std::abs (staticPitchSemis) > 1.0e-6f)
            rates.pitchRatio = AviatorFastMath::semitoneRatio (staticPitchSemis);
    }
    else
    {
        // PhraseOriginal, PhraseTimeStretch, SlicePhrase — fixed pitch (varispeed for BPM only).
        rates.pitchRatio = 1.0;
        rates.pitchSemitones = staticPitchSemis;
    }

    double timeRatio = juce::jlimit (0.25, 4.0, static_cast<double> (sourceSettings.speed));

    // Varispeed BPM sync: hostBpm / originalBpm.
    // Host slower than the sample → slower read (sample stays aligned to the grid).
    // Never combine with MIDI pitch tracking.
    // (True pitch-preserving stretch is a future engine.)
    if (! tracksMidiPitch
        && sourceSettings.bpmSync
        && sourceSettings.originalBpm > 1.f
        && sourceHostBpm > 1.0)
    {
        timeRatio *= sourceHostBpm / static_cast<double> (sourceSettings.originalBpm);
    }

    rates.timeRatio = timeRatio;
    v.playbackRates = rates;
}

float SamplerEngine::voiceReadIncrement (const Voice& v) const noexcept
{
    const auto& r = v.playbackRates;

    // pitchRatio is 1.0 when MIDI tracking / static tune are inactive.
    // Always fold it so PhraseOriginal + keytrack ON actually repitches.
    return static_cast<float> (r.sourceRateRatio * r.timeRatio * r.pitchRatio);
}

void SamplerEngine::advanceEnvelope (Voice& v) noexcept
{
    switch (v.envStage)
    {
        case EnvStage::idle:
            break;
        case EnvStage::attack:
            v.envLevel += v.envLinearStep;
            if (--v.envSegSamplesLeft <= 0 || v.envLevel >= 1.f)
                finishAttack (v);
            break;
        case EnvStage::decay:
            v.envLevel += v.envLinearStep;
            if (--v.envSegSamplesLeft <= 0)
            {
                v.envLevel = sustainLevel;
                v.envStage = EnvStage::sustain;
            }
            break;
        case EnvStage::sustain:
            break;
        case EnvStage::release:
            v.envLevel += v.envLinearStep;
            if (--v.envSegSamplesLeft <= 0 || v.envLevel <= 0.f)
            {
                v.envLevel = 0.f;
                resetVoiceState (v, true);
            }
            break;
    }
}

float SamplerEngine::renderVoiceSample (Voice& v) noexcept
{
    advanceGlide (v);

    const float env = v.envLevel;
    const float vel = v.velocity;
    float osc = 0.f;

    if (v.sampleData != nullptr && v.sampleNumFrames > 1)
    {
        const float inc = ReversePlayer::getReadIncrement (voiceReadIncrement (v), v.reversed);

        const int phraseStart = phraseEnabled ? v.phraseStartFrame : 0;
        const int phraseEnd = phraseEnabled ? v.phraseEndFrame : v.sampleNumFrames - 1;

        int i0 = static_cast<int> (std::floor (v.readPos));
        const float frac = v.readPos - static_cast<float> (i0);

        if (! v.reversed)
        {
            i0 = juce::jlimit (phraseStart, juce::jmax (phraseStart, phraseEnd - 1), i0);
            AK_ASSERT (i0 >= phraseStart && i0 + 1 <= phraseEnd);
            const float y0 = v.sampleData[juce::jmax (phraseStart, i0 - 1)];
            const float y1 = v.sampleData[i0];
            const float y2 = v.sampleData[juce::jmin (phraseEnd, i0 + 1)];
            const float y3 = v.sampleData[juce::jmin (phraseEnd, i0 + 2)];
            osc = AviatorFastMath::hermite4 (y0, y1, y2, y3, frac);
            v.readPos += inc;
            if (v.readPos >= static_cast<float> (phraseEnd))
            {
                if (phraseLoop && (phraseEnabled || sourceSettings.loopMode == LoopMode::Loop))
                    v.readPos = static_cast<float> (phraseStart);
                else
                    enterRelease (v);
            }
        }
        else
        {
            i0 = juce::jlimit (phraseStart + 1, phraseEnd, i0);
            AK_ASSERT (i0 >= phraseStart + 1 && i0 <= phraseEnd);
            const float y0 = v.sampleData[juce::jmin (phraseEnd, i0 + 1)];
            const float y1 = v.sampleData[i0];
            const float y2 = v.sampleData[juce::jmax (phraseStart, i0 - 1)];
            const float y3 = v.sampleData[juce::jmax (phraseStart, i0 - 2)];
            osc = AviatorFastMath::hermite4 (y0, y1, y2, y3, 1.f - frac);
            v.readPos += inc;
            if (v.readPos <= static_cast<float> (phraseStart))
            {
                if (phraseLoop && (phraseEnabled || sourceSettings.loopMode == LoopMode::Loop))
                    v.readPos = static_cast<float> (phraseEnd);
                else
                    enterRelease (v);
            }
        }
    }
    else
    {
        const float hz = midiNoteToHz (v.glideEngine.getCurrentPitchSemitones());
        const float delta = juce::MathConstants<float>::twoPi * hz / static_cast<float> (sampleRate);
        osc = AviatorFastMath::fastSin (v.phase);
        v.phase += delta;
        if (v.phase > juce::MathConstants<float>::twoPi) v.phase -= juce::MathConstants<float>::twoPi;
    }

    const float chopGain = chopState.active ? chopState.gateGain : 1.f;
    float out = osc * vel * env * chopGain;

    if (v.declickGain < 1.f)
    {
        out *= v.declickGain;
        v.declickGain = juce::jmin (1.f, v.declickGain + v.declickStep);
    }

    advanceEnvelope (v);
    return out;
}

void SamplerEngine::noteOn (int midiNote, float velocity, bool reverse, float glideTimeMs) noexcept
{
    const SampleLibrary::AudioRegion* region = nullptr;
    if (sampleSnapshot != nullptr)
        region = SampleLibrary::findRegionForNote (*sampleSnapshot, midiNote, velocity);

    if (playMode == PlayMode::mono || playMode == PlayMode::legato)
    {
        if (playMode == PlayMode::legato
            && monoVoiceIndex >= 0 && monoVoiceIndex < kMaxVoices
            && voices[monoVoiceIndex].active
            && voices[monoVoiceIndex].envStage != EnvStage::release)
        {
            // True legato: the sample and envelope keep running; only the
            // pitch moves (glide when set, snap otherwise). No restart means
            // no discontinuity at all.
            auto& v = voices[monoVoiceIndex];
            purgeNoteVoiceFromStack (v.noteNumber, monoVoiceIndex);
            v.noteNumber = midiNote;
            v.glideEngine.noteOn (midiNote, glideTimeMs);
            updateVoicePlaybackRates (v);
            lastNoteForGlide = midiNote;
            pushNoteVoice (midiNote, monoVoiceIndex);
            return;
        }

        // Mono retrigger: fade the previous note out over kChokeFadeMs
        // instead of the old instant allSoundOff() (a hard click), and fade
        // the new note in on a fresh voice — a brief crossfade. Registering
        // the voice in the note stack also makes note-off work in mono mode
        // (previously ignored → hung notes).
        const bool hadActive = activeVoiceCount > 0;
        if (hadActive)
            chokeActiveVoicesForPhrase();

        const int i = findFreeOrStealVoice();
        monoVoiceIndex = i;
        startVoice (voices[i], midiNote, velocity, reverse, glideTimeMs, region, hadActive);
        voices[i].voiceInstanceId = ++nextVoiceInstanceId;
        pushNoteVoice (midiNote, i);
        return;
    }

    if (retriggerPolicy == RetriggerPolicy::PhraseChoke)
    {
        chokeActiveVoicesForPhrase();
        clearAllNoteStacks();
    }
    else if (noteGatePolicy == NoteGatePolicy::Gated)
        chokeSameNoteVoices (midiNote);

    const int i = findFreeOrStealVoice();
    startVoice (voices[i], midiNote, velocity, reverse, glideTimeMs, region);
    voices[i].voiceInstanceId = ++nextVoiceInstanceId;
    pushNoteVoice (midiNote, i);
}

void SamplerEngine::noteOff (int midiNote) noexcept
{
    if (noteGatePolicy == NoteGatePolicy::TriggerToEnd)
        return;

    while (true)
    {
        const int voiceIndex = popNoteVoiceFifo (midiNote);
        if (voiceIndex < 0)
            break;

        auto& v = voices[voiceIndex];
        if (v.active)
        {
            enterRelease (v);
            break;
        }
    }
}

void SamplerEngine::allNotesOff() noexcept
{
    if (noteGatePolicy == NoteGatePolicy::TriggerToEnd)
        return;

    for (int i = 0; i < maxVoices; ++i)
    {
        if (voices[i].active)
            enterRelease (voices[i]);
    }
    clearAllNoteStacks();
}

int SamplerEngine::getNumActiveVoices() const noexcept
{
    int active = 0;

    for (int i = 0; i < maxVoices; ++i)
        if (voices[i].active)
            ++active;

    return active;
}

bool SamplerEngine::validateCurrentState() const noexcept
{
    bool valid = true;

    const bool hasSample = sampleSnapshot != nullptr
                           && ! sampleSnapshot->regions.empty()
                           && sampleSnapshot->regions.front().data != nullptr
                           && sampleSnapshot->regions.front().numFrames > 1;

    if (! hasSample)
    {
        AK_LOG ("Sampler validation failed: no sound is loaded");
        valid = false;
    }

    if (maxVoices <= 0)
    {
        AK_LOG ("Sampler validation failed: no sampler voices");
        valid = false;
    }

    if (! std::isfinite (attackMs) || ! std::isfinite (decayMs)
        || ! std::isfinite (sustainLevel) || ! std::isfinite (releaseMs))
    {
        AK_LOG ("Sampler validation failed: envelope parameters are invalid");
        valid = false;
    }

    if (sustainLevel < 0.f || sustainLevel > 1.f)
    {
        AK_LOG ("Sampler validation failed: sustain level out of range");
        valid = false;
    }

    if (sustainLevel <= 0.f)
    {
        AK_LOG ("Sampler validation failed: sustain level is zero");
        valid = false;
    }

    return valid;
}

bool SamplerEngine::isOneShotPlayback() const noexcept
{
    return noteGatePolicy == NoteGatePolicy::TriggerToEnd;
}

void SamplerEngine::allSoundOff() noexcept
{
    for (int i = 0; i < kMaxVoices; ++i)
        resetVoiceState (voices[i], voices[i].active);

    activeVoiceCount = 0;
    lastNoteForGlide = -1;
    monoVoiceIndex = -1;
    clearAllNoteStacks();
}

float SamplerEngine::getActiveVoiceReadIncrementForTest() const noexcept
{
    for (int i = 0; i < maxVoices; ++i)
    {
        if (voices[i].active)
            return voiceReadIncrement (voices[i]);
    }
    return 0.f;
}

SamplerEngine::NotePitchDiag SamplerEngine::getLastNotePitchDiag() const noexcept
{
    return lastNotePitchDiag;
}

void SamplerEngine::process (juce::AudioBuffer<float>& buffer)
{
    if (activeVoiceCount <= 0)
        return;

    const int numCh = buffer.getNumChannels();
    const int n = buffer.getNumSamples();
    if (numCh < 1 || n <= 0) return;

    float* L = buffer.getWritePointer (0);
    float* R = numCh > 1 ? buffer.getWritePointer (1) : L;

    for (int s = 0; s < n; ++s)
    {
        float sum = 0.f;
        for (int i = 0; i < maxVoices; ++i)
        {
            if (voices[i].active)
                sum += renderVoiceSample (voices[i]);
        }
        L[s] += sum;
        R[s] += sum;
    }
}
