#include "SynthEngine.h"
#include <cmath>

void SynthEngine::prepare (const juce::dsp::ProcessSpec& spec)
{
    if (std::abs (spec.sampleRate - sampleRate) > 1e-9)
        allSoundOff();

    sampleRate = spec.sampleRate;
}

void SynthEngine::releaseResources()
{
    allSoundOff();
}

void SynthEngine::setPolyphony (int numVoices) noexcept
{
    maxVoices = juce::jlimit (1, kMaxVoices, numVoices);
}

void SynthEngine::setPlayMode (PlayMode mode) noexcept
{
    playMode = mode;
}

void SynthEngine::setGlideMode (GlideMode mode) noexcept
{
    glideMode = mode;
}

void SynthEngine::setOscParams (int oscIndex,
                                int type,
                                float tuneSemis,
                                float fineCents,
                                float shape01,
                                float level01,
                                float pan) noexcept
{
    auto& o = oscIndex == 0 ? osc1 : osc2;
    o.type = juce::jlimit (0, 7, type);
    o.tune = tuneSemis;
    o.fine = fineCents;
    o.shape = juce::jlimit (0.f, 1.f, shape01);
    o.level = juce::jlimit (0.f, 1.f, level01);
    o.pan = juce::jlimit (-1.f, 1.f, pan);
}

void SynthEngine::setAmpEnvelopeMs (float attackMs, float decayMs, float sustain01, float releaseMs) noexcept
{
    ampAttackMs = juce::jlimit (0.5f, 5000.f, attackMs);
    ampDecayMs = juce::jlimit (1.f, 10000.f, decayMs);
    ampSustain = juce::jlimit (0.f, 1.f, sustain01);
    ampReleaseMs = juce::jlimit (5.f, 10000.f, releaseMs);
}

void SynthEngine::setFilterEnvelopeMs (float attackMs, float decayMs, float sustain01, float releaseMs) noexcept
{
    fltAttackMs = juce::jlimit (0.5f, 10000.f, attackMs);
    fltDecayMs = juce::jlimit (1.f, 10000.f, decayMs);
    fltSustain = juce::jlimit (0.f, 1.f, sustain01);
    fltReleaseMs = juce::jlimit (5.f, 30000.f, releaseMs);
}

float SynthEngine::midiNoteToHz (float note) noexcept
{
    return 440.f * std::pow (2.f, (note - 69.f) / 12.f);
}

float SynthEngine::constantPowerPanL (float pan) noexcept
{
    const float ang = (pan + 1.f) * (juce::MathConstants<float>::halfPi * 0.25f);
    return std::cos (ang);
}

float SynthEngine::constantPowerPanR (float pan) noexcept
{
    const float ang = (pan + 1.f) * (juce::MathConstants<float>::halfPi * 0.25f);
    return std::sin (ang);
}

float SynthEngine::oscSample (int type, float shape01, float phase01, float& noiseSeed) noexcept
{
    switch (type)
    {
        case 1: // square
        {
            const float pw = juce::jmap (shape01, 0.1f, 0.9f);
            return phase01 < pw ? 1.f : -1.f;
        }
        case 2: // triangle
            return phase01 < 0.5f ? phase01 * 4.f - 1.f : 3.f - phase01 * 4.f;
        case 3: // sine
            return std::sin (phase01 * juce::MathConstants<float>::twoPi);
        case 4: // noise
            noiseSeed = noiseSeed * 1664525.f + 1013904223.f;
            return (noiseSeed / static_cast<float> (0x7fffffff)) - 1.f;
        case 5: // wavetable placeholder — band-limited saw blend
        case 0: // saw
        default:
        {
            const float saw = phase01 * 2.f - 1.f;
            const float sine = std::sin (phase01 * juce::MathConstants<float>::twoPi);
            return juce::jmap (shape01, saw, sine);
        }
        case 6: // FM — modulated sine
        {
            const float mod = std::sin (phase01 * juce::MathConstants<float>::twoPi * (2.f + shape01 * 6.f));
            return std::sin (phase01 * juce::MathConstants<float>::twoPi + mod * shape01 * 3.f);
        }
        case 7: // chord — detuned stack
        {
            const float s1 = std::sin (phase01 * juce::MathConstants<float>::twoPi);
            const float s2 = std::sin (phase01 * juce::MathConstants<float>::twoPi * 1.002f);
            const float s3 = std::sin (phase01 * juce::MathConstants<float>::twoPi * 0.998f);
            return (s1 + s2 + s3) / 3.f;
        }
    }
}

int SynthEngine::findFreeOrStealVoice() noexcept
{
    for (int i = 0; i < maxVoices; ++i)
        if (! voices[i].active)
            return i;

    int best = 0;
    float bestLvl = 2.f;
    for (int i = 0; i < maxVoices; ++i)
    {
        if (voices[i].ampLevel < bestLvl)
        {
            bestLvl = voices[i].ampLevel;
            best = i;
        }
    }
    return best;
}

int SynthEngine::findMonoVoice() noexcept
{
    if (monoVoiceIndex >= 0 && voices[monoVoiceIndex].active)
        return monoVoiceIndex;

    return findFreeOrStealVoice();
}

void SynthEngine::startVoice (Voice& v, int midiNote, float velocity, float glideTimeMs) noexcept
{
    v.active = true;
    v.noteNumber = midiNote;
    v.velocity = juce::jlimit (0.f, 1.f, velocity);
    v.targetPitch = static_cast<float> (midiNote);

    const bool useGlide = glideTimeMs > 1.f
                          && ((glideMode == GlideMode::always)
                              || (glideMode == GlideMode::legato && lastNote >= 0));

    if (useGlide && lastNote >= 0)
    {
        v.currentPitch = static_cast<float> (lastNote);
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

    lastNote = midiNote;

    const double attS = ampAttackMs * 0.001;
    if (attS <= 0.0)
    {
        v.ampStage = EnvStage::decay;
        v.ampLevel = 1.f;
    }
    else
    {
        v.ampStage = EnvStage::attack;
        v.ampLevel = 0.f;
        const int n = juce::jmax (1, static_cast<int> (std::round (attS * sampleRate)));
        v.ampSegLeft = n;
        v.ampStep = 1.f / static_cast<float> (n);
    }

    v.ampSustain = ampSustain;

    filterEnvStage = EnvStage::attack;
    filterEnvLevel = 0.f;
    const double fltAttS = fltAttackMs * 0.001;
    if (fltAttS <= 0.0)
    {
        filterEnvStage = EnvStage::decay;
        filterEnvLevel = 1.f;
    }
    else
    {
        const int n = juce::jmax (1, static_cast<int> (std::round (fltAttS * sampleRate)));
        filterEnvSegLeft = n;
        filterEnvStep = 1.f / static_cast<float> (n);
    }
}

void SynthEngine::enterRelease (Voice& v) noexcept
{
    if (! v.active)
        return;

    const double relS = ampReleaseMs * 0.001;
    if (relS <= 0.0 || v.ampLevel <= 0.f)
    {
        v.active = false;
        v.ampStage = EnvStage::idle;
        v.ampLevel = 0.f;
        return;
    }

    v.ampStage = EnvStage::release;
    const int n = juce::jmax (1, static_cast<int> (std::round (relS * sampleRate)));
    v.ampSegLeft = n;
    v.ampStep = -v.ampLevel / static_cast<float> (n);

    filterEnvStage = EnvStage::release;
    const double fltRelS = fltReleaseMs * 0.001;
    const int fn = juce::jmax (1, static_cast<int> (std::round (fltRelS * sampleRate)));
    filterEnvSegLeft = fn;
    filterEnvStep = -filterEnvLevel / static_cast<float> (fn);
}

void SynthEngine::advanceAmpEnv (Voice& v) noexcept
{
    switch (v.ampStage)
    {
        case EnvStage::attack:
            v.ampLevel += v.ampStep;
            if (--v.ampSegLeft <= 0 || v.ampLevel >= 1.f)
            {
                v.ampLevel = 1.f;
                v.ampStage = EnvStage::decay;
                const double decS = ampDecayMs * 0.001;
                const int n = juce::jmax (1, static_cast<int> (std::round (decS * sampleRate)));
                v.ampSegLeft = n;
                v.ampStep = (v.ampSustain - 1.f) / static_cast<float> (n);
            }
            break;
        case EnvStage::decay:
            v.ampLevel += v.ampStep;
            if (--v.ampSegLeft <= 0)
            {
                v.ampLevel = v.ampSustain;
                v.ampStage = EnvStage::sustain;
            }
            break;
        case EnvStage::sustain:
            break;
        case EnvStage::release:
            v.ampLevel += v.ampStep;
            if (--v.ampSegLeft <= 0 || v.ampLevel <= 0.f)
            {
                v.ampLevel = 0.f;
                v.active = false;
                v.ampStage = EnvStage::idle;
            }
            break;
        default:
            break;
    }
}

void SynthEngine::advanceFilterEnv (EnvStage) noexcept
{
    switch (filterEnvStage)
    {
        case EnvStage::attack:
            filterEnvLevel += filterEnvStep;
            if (--filterEnvSegLeft <= 0 || filterEnvLevel >= 1.f)
            {
                filterEnvLevel = 1.f;
                filterEnvStage = EnvStage::decay;
                const double decS = fltDecayMs * 0.001;
                const int n = juce::jmax (1, static_cast<int> (std::round (decS * sampleRate)));
                filterEnvSegLeft = n;
                filterEnvStep = (fltSustain - 1.f) / static_cast<float> (n);
            }
            break;
        case EnvStage::decay:
            filterEnvLevel += filterEnvStep;
            if (--filterEnvSegLeft <= 0)
            {
                filterEnvLevel = fltSustain;
                filterEnvStage = EnvStage::sustain;
            }
            break;
        case EnvStage::sustain:
            break;
        case EnvStage::release:
            filterEnvLevel += filterEnvStep;
            if (--filterEnvSegLeft <= 0 || filterEnvLevel <= 0.f)
            {
                filterEnvLevel = 0.f;
                filterEnvStage = EnvStage::idle;
            }
            break;
        default:
            break;
    }
}

void SynthEngine::advanceGlide (Voice& v) noexcept
{
    if (! v.gliding)
        return;

    v.currentPitch += v.glideIncPerSample;
    if ((v.glideIncPerSample > 0.f && v.currentPitch >= v.targetPitch) ||
        (v.glideIncPerSample < 0.f && v.currentPitch <= v.targetPitch))
    {
        v.currentPitch = v.targetPitch;
        v.gliding = false;
    }
}

float SynthEngine::renderVoice (Voice& v) noexcept
{
    advanceGlide (v);

    const float pitch1 = v.currentPitch + osc1.tune + osc1.fine / 100.f;
    const float pitch2 = v.currentPitch + osc2.tune + osc2.fine / 100.f;
    const float hz1 = midiNoteToHz (pitch1);
    const float hz2 = midiNoteToHz (pitch2);

    const float inc1 = hz1 / static_cast<float> (sampleRate);
    const float inc2 = hz2 / static_cast<float> (sampleRate);

    v.osc1Phase += inc1;
    v.osc2Phase += inc2;
    if (v.osc1Phase >= 1.f) v.osc1Phase -= 1.f;
    if (v.osc2Phase >= 1.f) v.osc2Phase -= 1.f;

    const float s1 = oscSample (osc1.type, osc1.shape, v.osc1Phase, v.noiseSeed) * osc1.level;
    const float s2 = oscSample (osc2.type, osc2.shape, v.osc2Phase, v.noiseSeed) * osc2.level;

    const float mono = (s1 + s2) * 0.5f * v.velocity * v.ampLevel;
    advanceAmpEnv (v);
    return mono;
}

void SynthEngine::noteOn (int midiNote, float velocity, float glideTimeMs) noexcept
{
    if (playMode == PlayMode::mono || playMode == PlayMode::legato)
    {
        const int i = findMonoVoice();
        monoVoiceIndex = i;
        if (voices[i].active && playMode == PlayMode::legato)
            startVoice (voices[i], midiNote, velocity, glideTimeMs);
        else
        {
            allSoundOff();
            monoVoiceIndex = i;
            startVoice (voices[i], midiNote, velocity, glideTimeMs);
        }
        return;
    }

    const int i = findFreeOrStealVoice();
    startVoice (voices[i], midiNote, velocity, glideTimeMs);
}

void SynthEngine::noteOff (int midiNote) noexcept
{
    for (auto& v : voices)
    {
        if (v.active && v.noteNumber == midiNote)
            enterRelease (v);
    }
}

void SynthEngine::allNotesOff() noexcept
{
    for (auto& v : voices)
    {
        if (v.active)
            enterRelease (v);
    }
}

void SynthEngine::allSoundOff() noexcept
{
    for (auto& v : voices)
    {
        v.active = false;
        v.ampStage = EnvStage::idle;
        v.ampLevel = 0.f;
    }
    filterEnvStage = EnvStage::idle;
    filterEnvLevel = 0.f;
    lastNote = -1;
    monoVoiceIndex = -1;
}

void SynthEngine::process (juce::AudioBuffer<float>& buffer)
{
    const int numCh = buffer.getNumChannels();
    const int n = buffer.getNumSamples();
    if (numCh < 1 || n <= 0)
        return;

    float* L = buffer.getWritePointer (0);
    float* R = numCh > 1 ? buffer.getWritePointer (1) : L;

    for (int s = 0; s < n; ++s)
    {
        float sumL = 0.f;
        float sumR = 0.f;

        for (auto& v : voices)
        {
            if (! v.active)
                continue;

            const float mono = renderVoice (v);
            sumL += mono * constantPowerPanL (osc1.pan);
            sumR += mono * constantPowerPanR (osc1.pan);
            sumL += mono * constantPowerPanL (osc2.pan) * 0.5f;
            sumR += mono * constantPowerPanR (osc2.pan) * 0.5f;
        }

        advanceFilterEnv (filterEnvStage);

        L[s] += sumL;
        R[s] += sumR;
    }
}
