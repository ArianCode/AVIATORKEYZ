#include "GlideEngine.h"
#include <cmath>

GlideEngine::GlideEngine()  = default;
GlideEngine::~GlideEngine() = default;

void GlideEngine::setSampleRate (double sr)
{
    sampleRate = sr;
}

void GlideEngine::snapToPitch (float semitones) noexcept
{
    currentPitch   = semitones;
    glideActive    = false;
    glideIncrement = 0.0f;
}

void GlideEngine::noteOn (int midiNote, float glideTimeMs)
{
    targetPitch = static_cast<float> (midiNote);

    if (glideTimeMs < 1.0f)
    {
        // Glide off — snap immediately
        currentPitch  = targetPitch;
        glideActive   = false;
        glideIncrement = 0.0f;
        return;
    }

    const double samplesForGlide = (glideTimeMs / 1000.0) * sampleRate;
    glideIncrement = (targetPitch - currentPitch) / static_cast<float> (samplesForGlide);
    glideActive    = std::abs (targetPitch - currentPitch) > 0.001f;
}

float GlideEngine::getCurrentPitchSemitones() const noexcept
{
    return currentPitch;
}

void GlideEngine::tick() noexcept
{
    if (! glideActive) return;

    currentPitch += glideIncrement;

    // Arrived at target?
    if ((glideIncrement > 0.0f && currentPitch >= targetPitch) ||
        (glideIncrement < 0.0f && currentPitch <= targetPitch))
    {
        currentPitch  = targetPitch;
        glideActive   = false;
        glideIncrement = 0.0f;
    }
}
