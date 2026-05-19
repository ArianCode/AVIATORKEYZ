#include "SamplerEngine.h"
#include "ReversePlayer.h"
#include <cmath>

SamplerEngine::SamplerEngine()  = default;
SamplerEngine::~SamplerEngine() = default;

void SamplerEngine::prepare (const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;
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

void SamplerEngine::setEnvelopeTimesMs (float aMs, float rMs) noexcept
{
    attackMs = juce::jlimit (0.5f, 5000.f, aMs);
    releaseMs = juce::jlimit (5.f, 10000.f, rMs);
}

float SamplerEngine::midiNoteToHz (float note) noexcept
{
    return 440.f * std::pow (2.f, (note - 69.f) / 12.f);
}

int SamplerEngine::findFreeOrStealVoice() noexcept
{
    for (int i = 0; i < kMaxVoices; ++i)
        if (! voices[i].active)
            return i;

    int best = 0;
    float bestLvl = 2.f;
    for (int i = 0; i < kMaxVoices; ++i)
    {
        if (voices[i].envLevel < bestLvl)
        {
            bestLvl = voices[i].envLevel;
            best = i;
        }
    }
    return best;
}

void SamplerEngine::startVoice (Voice& v,
                                 int midiNote,
                                 float velocity,
                                 bool reverse,
                                 float glideTimeMs,
                                 const SampleLibrary::AudioRegion* region) noexcept
{
    v.active = true;
    v.noteNumber = midiNote;
    v.velocity = juce::jlimit (0.f, 1.f, velocity);
    v.reversed = reverse;
    v.phase = 0.f;
    v.targetPitch = static_cast<float> (midiNote);

    if (region != nullptr && region->data != nullptr && region->numFrames > 1)
    {
        v.sampleData      = region->data;
        v.sampleNumFrames = region->numFrames;
        v.sampleRootNote  = region->rootNote;
    }
    else
    {
        v.sampleData      = nullptr;
        v.sampleNumFrames = 0;
        v.sampleRootNote  = 60;
    }

    if (glideTimeMs > 1.f && lastNoteForGlide >= 0)
    {
        v.currentPitch = static_cast<float> (lastNoteForGlide);
        v.gliding = std::abs (v.targetPitch - v.currentPitch) > 0.001f;
        const double glideSamples = (glideTimeMs * 0.001) * sampleRate;
        v.glideIncPerSample = v.gliding
                                  ? (v.targetPitch - v.currentPitch) / static_cast<float> (glideSamples)
                                  : 0.f;
    }
    else
    {
        v.currentPitch = v.targetPitch;
        v.gliding = false;
        v.glideIncPerSample = 0.f;
    }

    lastNoteForGlide = midiNote;

    if (v.sampleData != nullptr && v.sampleNumFrames > 1)
        v.readPos = reverse ? static_cast<float> (v.sampleNumFrames - 1) : 0.f;
    else
        v.readPos = 0.f;

    const double attS = attackMs * 0.001;
    if (attS <= 0.0)
    {
        v.envStage = EnvStage::sustain;
        v.envLevel = 1.f;
        v.envLinearStep = 0.f;
        v.envSegSamplesLeft = 0;
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

void SamplerEngine::enterRelease (Voice& v) noexcept
{
    if (! v.active) return;

    const double relS = releaseMs * 0.001;
    if (relS <= 0.0 || v.envLevel <= 0.f)
    {
        v.active = false;
        v.envStage = EnvStage::idle;
        v.envLevel = 0.f;
        return;
    }

    v.envStage = EnvStage::release;
    const int n = juce::jmax (1, static_cast<int> (std::round (relS * sampleRate)));
    v.envSegSamplesLeft = n;
    v.envLinearStep = -v.envLevel / static_cast<float> (n);
}

void SamplerEngine::advanceGlide (Voice& v) noexcept
{
    if (! v.gliding) return;

    v.currentPitch += v.glideIncPerSample;
    if ((v.glideIncPerSample > 0.f && v.currentPitch >= v.targetPitch) ||
        (v.glideIncPerSample < 0.f && v.currentPitch <= v.targetPitch))
    {
        v.currentPitch = v.targetPitch;
        v.gliding = false;
    }
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
            {
                v.envLevel = 1.f;
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
                v.active = false;
                v.envStage = EnvStage::idle;
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
        const float pitchRatio =
            std::pow (2.f, (v.currentPitch - static_cast<float> (v.sampleRootNote)) / 12.f);
        const float inc = ReversePlayer::getReadIncrement (pitchRatio, v.reversed);

        int i0 = static_cast<int> (std::floor (v.readPos));
        const float frac = v.readPos - static_cast<float> (i0);

        if (! v.reversed)
        {
            i0 = juce::jlimit (0, v.sampleNumFrames - 2, i0);
            const float s0 = v.sampleData[i0];
            const float s1 = v.sampleData[i0 + 1];
            osc = s0 + frac * (s1 - s0);
            v.readPos += inc;
            while (v.readPos >= static_cast<float> (v.sampleNumFrames - 1))
                v.readPos -= static_cast<float> (v.sampleNumFrames - 1);
        }
        else
        {
            i0 = juce::jlimit (1, v.sampleNumFrames - 1, i0);
            const float s0 = v.sampleData[i0];
            const float s1 = v.sampleData[i0 - 1];
            osc = s0 + (1.f - frac) * (s1 - s0);
            v.readPos += inc;
            while (v.readPos <= 0.f)
                v.readPos += static_cast<float> (v.sampleNumFrames - 1);
        }
    }
    else
    {
        const float hz = midiNoteToHz (v.currentPitch);
        const float delta = juce::MathConstants<float>::twoPi * hz / static_cast<float> (sampleRate);
        osc = std::sin (v.phase);
        v.phase += delta;
        if (v.phase > juce::MathConstants<float>::twoPi) v.phase -= juce::MathConstants<float>::twoPi;
    }

    const float out = osc * vel * env;
    advanceEnvelope (v);
    return out;
}

void SamplerEngine::noteOn (int midiNote, float velocity, bool reverse, float glideTimeMs) noexcept
{
    const SampleLibrary::AudioRegion* region = nullptr;
    if (sampleSnapshot != nullptr)
        region = SampleLibrary::findRegionForNote (*sampleSnapshot, midiNote, velocity);

    const int i = findFreeOrStealVoice();
    startVoice (voices[i], midiNote, velocity, reverse, glideTimeMs, region);
}

void SamplerEngine::noteOff (int midiNote) noexcept
{
    for (auto& v : voices)
    {
        if (v.active && v.noteNumber == midiNote)
            enterRelease (v);
    }
}

void SamplerEngine::allNotesOff() noexcept
{
    for (auto& v : voices)
    {
        if (v.active) enterRelease (v);
    }
}

void SamplerEngine::allSoundOff() noexcept
{
    for (auto& v : voices)
    {
        v.active = false;
        v.envStage = EnvStage::idle;
        v.envLevel = 0.f;
        v.sampleData = nullptr;
        v.sampleNumFrames = 0;
    }
    lastNoteForGlide = -1;
}

void SamplerEngine::process (juce::AudioBuffer<float>& buffer)
{
    const int numCh = buffer.getNumChannels();
    const int n = buffer.getNumSamples();
    if (numCh < 1 || n <= 0) return;

    float* L = buffer.getWritePointer (0);
    float* R = numCh > 1 ? buffer.getWritePointer (1) : L;

    for (int s = 0; s < n; ++s)
    {
        float sum = 0.f;
        for (auto& v : voices)
        {
            if (v.active)
                sum += renderVoiceSample (v);
        }
        L[s] += sum;
        R[s] += sum;
    }
}
