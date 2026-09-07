#include "StretchPlayer.h"

#if defined(__clang__)
 #pragma clang diagnostic push
 #pragma clang diagnostic ignored "-Wshadow"
 #pragma clang diagnostic ignored "-Wconversion"
 #pragma clang diagnostic ignored "-Wsign-conversion"
 #pragma clang diagnostic ignored "-Wfloat-equal"
 #pragma clang diagnostic ignored "-Wunused-parameter"
 #pragma clang diagnostic ignored "-Wzero-as-null-pointer-constant"
 #pragma clang diagnostic ignored "-Wmissing-prototypes"
 #pragma clang diagnostic ignored "-Wswitch-enum"
 #pragma clang diagnostic ignored "-Wimplicit-int-conversion"
 #pragma clang diagnostic ignored "-Wshorten-64-to-32"
 #pragma clang diagnostic ignored "-Wcast-align"
 #pragma clang diagnostic ignored "-Wextra-semi"
 #pragma clang diagnostic ignored "-Wpedantic"
 #pragma clang diagnostic ignored "-Wunused-variable"
#endif
#include "signalsmith-stretch/signalsmith-stretch.h"
#if defined(__clang__)
 #pragma clang diagnostic pop
#endif

#include <cmath>

struct StretchPlayer::Impl
{
    signalsmith::stretch::SignalsmithStretch<float> stretch;
};

namespace
{
constexpr int kMaxTimeRatioInput = 8; // 4x speed * 2x rate conversion headroom
}

StretchPlayer::StretchPlayer() : impl (std::make_unique<Impl>()) {}
StretchPlayer::~StretchPlayer() = default;

void StretchPlayer::prepare (double sr, int maxBlockSize)
{
    sampleRate = sr > 1000.0 ? sr : 44100.0;
    maxBlock = juce::jmax (16, maxBlockSize);

    impl->stretch.presetDefault (1, (float) sampleRate);
    inBuf.assign ((size_t) (maxBlock * kMaxTimeRatioInput + 64), 0.f);
    outBuf.assign ((size_t) maxBlock, 0.f);

    // Warm the stretcher's internal seek buffers so noteOn() never allocates.
    const int preroll = impl->stretch.inputLatency() + impl->stretch.outputLatency() * kMaxTimeRatioInput;
    std::vector<float> zeros ((size_t) preroll, 0.f);
    float* in[1] = { zeros.data() };
    impl->stretch.seek (in, preroll, 1.0);
    impl->stretch.reset();

    prepared = true;
    reset();
}

void StretchPlayer::reset()
{
    if (prepared)
        impl->stretch.reset();
    active = false;
    noteNumber = -1;
    stage = Stage::idle;
    envLevel = 0.f;
    envStep = 0.f;
    envSamplesLeft = 0;
    readPos = 0.0;
    inputAccum = 0.0;
    materialExhausted = false;
    playhead.store (0.f, std::memory_order_relaxed);
}

double StretchPlayer::currentTimeRatio() const noexcept
{
    double ratio = juce::jlimit (0.25, 4.0, (double) params.speed);
    if (params.bpmSync && params.originalBpm > 1.f && params.hostBpm > 1.0)
        ratio *= params.hostBpm / (double) params.originalBpm;
    const double fileRate = (region != nullptr && region->fileSampleRate > 0.0) ? region->fileSampleRate : sampleRate;
    ratio *= fileRate / sampleRate;
    return juce::jlimit (0.05, (double) kMaxTimeRatioInput, ratio);
}

// -----------------------------------------------------------------------------
//  envelope (linear segments, same shape as SamplerEngine)
// -----------------------------------------------------------------------------
void StretchPlayer::finishAttack() noexcept
{
    envLevel = 1.f;
    if (params.decayMs <= 0.f || std::abs (params.sustain - 1.f) < 1.0e-6f)
    {
        envLevel = params.sustain;
        stage = Stage::sustain;
        envStep = 0.f;
        envSamplesLeft = 0;
    }
    else
    {
        stage = Stage::decay;
        const int n = juce::jmax (1, (int) std::round (params.decayMs * 0.001 * sampleRate));
        envSamplesLeft = n;
        envStep = (params.sustain - 1.f) / (float) n;
    }
}

void StretchPlayer::enterRelease() noexcept
{
    if (! active)
        return;
    const int n = juce::jmax (1, (int) std::round (juce::jmax (1.f, params.releaseMs) * 0.001 * sampleRate));
    stage = Stage::release;
    envSamplesLeft = n;
    envStep = -envLevel / (float) n;
}

void StretchPlayer::advanceEnvelope() noexcept
{
    switch (stage)
    {
        case Stage::attack:
            envLevel += envStep;
            if (--envSamplesLeft <= 0 || envLevel >= 1.f)
                finishAttack();
            break;
        case Stage::decay:
            envLevel += envStep;
            if (--envSamplesLeft <= 0)
            {
                envLevel = params.sustain;
                stage = Stage::sustain;
            }
            break;
        case Stage::release:
            envLevel += envStep;
            if (--envSamplesLeft <= 0 || envLevel <= 0.f)
            {
                envLevel = 0.f;
                stage = Stage::idle;
                active = false;
            }
            break;
        case Stage::sustain:
        case Stage::idle:
        default:
            break;
    }
}

// -----------------------------------------------------------------------------
void StretchPlayer::noteOn (int midiNote, float velocity) noexcept
{
    if (! prepared || region == nullptr || region->data == nullptr || region->numFrames <= 1)
        return;

    noteNumber = midiNote;
    velocityGain = juce::jmap (juce::jlimit (0.f, 1.f, params.velocitySensitivity), 1.f, juce::jlimit (0.f, 1.f, velocity));

    const int frames = region->numFrames;
    const int startFrame = juce::jlimit (0, frames - 2, (int) (params.start * (float) (frames - 1)));
    const int endFrame = juce::jlimit (startFrame + 1, frames - 1, (int) (params.end * (float) (frames - 1)));
    readPos = params.reverse ? (double) endFrame : (double) startFrame;
    inputAccum = 0.0;
    materialExhausted = false;
    active = true;

    // Prime the stretcher with material from the start so the onset is immediate.
    impl->stretch.reset();
    const double ratio = currentTimeRatio();
    const int preroll = juce::jmin ((int) inBuf.size(),
                                    impl->stretch.inputLatency() + (int) std::ceil (impl->stretch.outputLatency() * ratio));
    const int got = fillInput (inBuf.data(), preroll);
    if (got < preroll)
        std::fill (inBuf.begin() + got, inBuf.begin() + preroll, 0.f);
    float* in[1] = { inBuf.data() };
    impl->stretch.seek (in, preroll, ratio);

    const double attS = params.attackMs * 0.001;
    if (attS <= 0.0)
    {
        finishAttack();
    }
    else
    {
        stage = Stage::attack;
        envLevel = 0.f;
        const int n = juce::jmax (1, (int) std::round (attS * sampleRate));
        envSamplesLeft = n;
        envStep = 1.f / (float) n;
    }
}

void StretchPlayer::noteOff (int midiNote) noexcept
{
    if (active && midiNote == noteNumber)
        enterRelease();
}

void StretchPlayer::allNotesOff() noexcept
{
    enterRelease();
}

void StretchPlayer::allSoundOff() noexcept
{
    active = false;
    stage = Stage::idle;
    envLevel = 0.f;
    if (prepared)
        impl->stretch.reset();
}

int StretchPlayer::fillInput (float* dst, int numSamples) noexcept
{
    if (region == nullptr || region->data == nullptr || region->numFrames <= 1)
        return 0;

    const float* data = region->data;
    const int frames = region->numFrames;
    const int startFrame = juce::jlimit (0, frames - 2, (int) (params.start * (float) (frames - 1)));
    const int endFrame = juce::jlimit (startFrame + 1, frames - 1, (int) (params.end * (float) (frames - 1)));
    const bool loops = params.loopMode == LoopMode::Loop;
    const double dir = params.reverse ? -1.0 : 1.0;

    int written = 0;
    for (; written < numSamples; ++written)
    {
        if (materialExhausted)
            break;

        int idx = (int) readPos;
        if (idx < startFrame || idx > endFrame)
        {
            if (loops)
            {
                readPos = params.reverse ? (double) endFrame : (double) startFrame;
                idx = (int) readPos;
            }
            else
            {
                materialExhausted = true;
                break;
            }
        }
        dst[written] = data[idx];
        readPos += dir;
    }
    for (int i = written; i < numSamples; ++i)
        dst[i] = 0.f;

    playhead.store (juce::jlimit (0.f, 1.f, (float) (readPos / (double) juce::jmax (1, frames - 1))),
                    std::memory_order_relaxed);
    return written;
}

void StretchPlayer::render (juce::AudioBuffer<float>& buffer) noexcept
{
    if (! prepared || ! active || buffer.getNumChannels() < 1)
        return;

    const int n = juce::jmin (buffer.getNumSamples(), maxBlock);
    if (n <= 0)
        return;

    const double ratio = currentTimeRatio();
    inputAccum += ratio * n;
    int nIn = (int) inputAccum;
    inputAccum -= nIn;
    nIn = juce::jlimit (0, (int) inBuf.size(), nIn);

    const int got = fillInput (inBuf.data(), nIn);
    if (got < nIn && stage != Stage::release && materialExhausted)
        enterRelease(); // played through the window: release naturally

    const float transpose = params.tuneSemis
                            + (params.keytrack ? (float) (noteNumber - region->rootNote) : 0.f);
    impl->stretch.setTransposeSemitones (transpose, 0.f);

    float* in[1] = { inBuf.data() };
    float* out[1] = { outBuf.data() };
    impl->stretch.process (in, nIn, out, n);

    float* L = buffer.getWritePointer (0);
    float* R = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : L;
    for (int i = 0; i < n; ++i)
    {
        const float s = outBuf[(size_t) i] * envLevel * velocityGain;
        L[i] += s;
        if (R != L)
            R[i] += s;
        advanceEnvelope();
        if (! active)
        {
            // silence the rest of the block
            for (int j = i + 1; j < n; ++j) { juce::ignoreUnused (j); }
            break;
        }
    }
}
