#include "Arpeggiator.h"
#include <algorithm>
#include <cmath>

namespace
{
constexpr double kMinBpm = 20.0;
constexpr float  kHumanizeMaxMs = 12.f;
constexpr float  kHumanizeMaxVel = 0.3f;

double alignUp (double beat, double grid) noexcept
{
    if (grid <= 0.0)
        return beat;
    const double k = std::ceil (beat / grid - 1.0e-9);
    return k * grid;
}
} // namespace

Arpeggiator::Arpeggiator()
{
    rng.setSeed (0x41524701);
}

void Arpeggiator::prepare (double sr)
{
    sampleRate = sr > 1000.0 ? sr : 44100.0;
    reset();
}

void Arpeggiator::reset()
{
    clearHeld();
    std::fill (std::begin (physical), std::end (physical), false);
    physicalCount = 0;
    orderCounter = 0;
    chordComplete = false;
    seqLen = 0;
    seqIndex = 0;
    stepCounter = 0;
    beatPos = 0.0;
    nextStepBeat = 0.0;
    lastStepBeats = 0.0;
    running = false;
    numPendingOffs = 0;
    sustainDown = false;
    publishUiState();
}

void Arpeggiator::setSettings (const Arp::Settings& s) noexcept
{
    const bool holdWasOn = settings.hold || sustainDown;
    const bool octavesChanged = s.octaves != settings.octaves;
    const bool modeChanged = s.mode != settings.mode;
    settings = s;

    if (holdWasOn && ! (settings.hold || sustainDown))
        purgeUnheldWhenLatchReleased();

    if (octavesChanged || modeChanged)
        rebuildSequence();
}

void Arpeggiator::setSustain (bool down) noexcept
{
    const bool holdWasOn = settings.hold || sustainDown;
    sustainDown = down;
    if (holdWasOn && ! (settings.hold || sustainDown))
        purgeUnheldWhenLatchReleased();
}

// -----------------------------------------------------------------------------
//  held chord
// -----------------------------------------------------------------------------
void Arpeggiator::addHeld (int note, float velocity) noexcept
{
    for (int i = 0; i < numHeld; ++i)
    {
        if (held[i].note == note)
        {
            held[i].velocity = velocity;
            return;
        }
    }
    if (numHeld >= Arp::kMaxHeld)
        return;

    held[numHeld++] = { note, velocity, orderCounter++ };
}

void Arpeggiator::removeHeld (int note) noexcept
{
    for (int i = 0; i < numHeld; ++i)
    {
        if (held[i].note == note)
        {
            for (int j = i + 1; j < numHeld; ++j)
                held[j - 1] = held[j];
            --numHeld;
            return;
        }
    }
}

void Arpeggiator::clearHeld() noexcept
{
    numHeld = 0;
}

void Arpeggiator::purgeUnheldWhenLatchReleased() noexcept
{
    int kept = 0;
    for (int i = 0; i < numHeld; ++i)
        if (physical[held[i].note])
            held[kept++] = held[i];
    numHeld = kept;
    chordComplete = false;
    rebuildSequence();
}

void Arpeggiator::rebuildSequence() noexcept
{
    seqLen = Arp::buildSequence (held, numHeld, settings.mode, settings.octaves,
                                 sequence, Arp::kMaxSequence);
    if (seqLen <= 0)
    {
        seqIndex = 0;
        running = false;
    }
    else if (seqIndex >= seqLen)
    {
        seqIndex = 0;
    }
    publishUiState();
}

void Arpeggiator::restartPattern (double beatAtEvent) noexcept
{
    seqIndex = 0;
    stepCounter = 0;
    running = true;
    const double stepBeats = Arp::stepLengthBeats (settings.rate, settings.feel);
    lastStepBeats = stepBeats;
    // First step fires immediately on the key press; subsequent steps stay on
    // the grid that the press established (free-run) — the host-synced case
    // re-aligns in process().
    nextStepBeat = beatAtEvent;
}

void Arpeggiator::noteOn (int note, float velocity, double beatAtEvent) noexcept
{
    note = juce::jlimit (0, 127, note);
    const bool latch = settings.hold || sustainDown;

    if (! physical[note])
    {
        physical[note] = true;
        ++physicalCount;
    }

    if (latch && chordComplete)
    {
        clearHeld();
        chordComplete = false;
    }

    const bool wasEmpty = numHeld == 0;
    addHeld (note, velocity);
    rebuildSequence();

    if (wasEmpty)
        restartPattern (beatAtEvent);
}

void Arpeggiator::noteOff (int note) noexcept
{
    note = juce::jlimit (0, 127, note);
    if (physical[note])
    {
        physical[note] = false;
        --physicalCount;
    }

    if (settings.hold || sustainDown)
    {
        if (physicalCount <= 0)
            chordComplete = true;
        return;
    }

    removeHeld (note);
    rebuildSequence();
}

void Arpeggiator::publishUiState() noexcept
{
    uint64_t lo = 0, hi = 0;
    for (int i = 0; i < numHeld; ++i)
    {
        const int n = held[i].note;
        if (n < 64) lo |= (uint64_t) 1 << n;
        else        hi |= (uint64_t) 1 << (n - 64);
    }
    uiState.heldMaskLo.store (lo, std::memory_order_relaxed);
    uiState.heldMaskHi.store (hi, std::memory_order_relaxed);
    uiState.sequenceLength.store (seqLen, std::memory_order_relaxed);
    uiState.running.store (running && seqLen > 0, std::memory_order_relaxed);
    if (seqLen <= 0)
        uiState.currentIndex.store (-1, std::memory_order_relaxed);
}

// -----------------------------------------------------------------------------
//  scheduling
// -----------------------------------------------------------------------------
void Arpeggiator::addPendingOff (int note, int sliceIndex, int samplesUntilOff) noexcept
{
    if (numPendingOffs >= kMaxPendingOffs)
        return;
    pendingOffs[numPendingOffs++] = { note, sliceIndex, juce::jmax (1, samplesUntilOff) };
}

void Arpeggiator::emitStep (int samplePos, double stepBeats, double beatsPerSample,
                            Event* out, int& count) noexcept
{
    if (seqLen <= 0 || count >= kMaxEvents)
        return;

    int idx = seqIndex;
    if (settings.mode == Arp::Mode::random)
        idx = rng.nextInt (juce::jmax (1, seqLen));
    idx = juce::jlimit (0, seqLen - 1, idx);

    Arp::Step step = sequence[idx];

    // octave spread: chance of a ±1 octave jump on this step
    if (settings.octSpread > 0.001f && rng.nextFloat() < settings.octSpread)
    {
        const int jump = rng.nextBool() ? 12 : -12;
        const int candidate = step.note + jump;
        if (candidate >= 0 && candidate <= 127)
        {
            step.note = candidate;
            step.octave += jump / 12;
        }
    }

    // humanize: timing + velocity jitter
    int pos = samplePos;
    float vel = step.velocity;
    if (settings.humanize > 0.001f)
    {
        const float jitterMs = (rng.nextFloat() * 2.f - 1.f) * settings.humanize * kHumanizeMaxMs;
        pos += (int) std::lround (jitterMs * 0.001 * sampleRate);
        vel *= 1.f + (rng.nextFloat() * 2.f - 1.f) * settings.humanize * kHumanizeMaxVel;
    }
    pos = juce::jmax (samplePos, pos); // never earlier than the grid (can't rewind a block)
    vel = juce::jlimit (0.05f, 1.f, vel);

    const int slice = Arp::sliceIndexForNote (step.baseNote, step.octave);

    // If this note still has a pending off (gate == 100%), close it first.
    for (int i = 0; i < numPendingOffs; ++i)
    {
        if (pendingOffs[i].note == step.note && count < kMaxEvents)
        {
            out[count++] = { pos, false, step.note, 0.f, pendingOffs[i].sliceIndex };
            for (int j = i + 1; j < numPendingOffs; ++j)
                pendingOffs[j - 1] = pendingOffs[j];
            --numPendingOffs;
            break;
        }
    }

    if (count >= kMaxEvents)
        return;
    out[count++] = { pos, true, step.note, vel, slice };

    const double gateBeats = juce::jlimit (0.05, 1.0, (double) settings.gate) * stepBeats;
    const int gateSamples = juce::jmax (1, (int) std::lround (gateBeats / beatsPerSample));
    addPendingOff (step.note, slice, pos + gateSamples);

    uiState.currentIndex.store (idx, std::memory_order_relaxed);
    seqIndex = (seqIndex + 1) % juce::jmax (1, seqLen);
    ++stepCounter;
}

int Arpeggiator::flushAllNotesOff (Event* out) noexcept
{
    int count = 0;
    for (int i = 0; i < numPendingOffs && count < kMaxEvents; ++i)
        out[count++] = { 0, false, pendingOffs[i].note, 0.f, pendingOffs[i].sliceIndex };
    numPendingOffs = 0;
    clearHeld();
    std::fill (std::begin (physical), std::end (physical), false);
    physicalCount = 0;
    chordComplete = false;
    seqLen = 0;
    seqIndex = 0;
    running = false;
    publishUiState();
    return count;
}

int Arpeggiator::process (const juce::MidiBuffer& in,
                          int numSamples,
                          double bpm,
                          double hostBeat,
                          bool hostPlaying,
                          Event* out) noexcept
{
    int count = 0;
    if (out == nullptr || numSamples <= 0)
        return 0;

    bpm = juce::jmax (kMinBpm, bpm);
    const double beatsPerSample = bpm / 60.0 / sampleRate;
    const double stepBeats = Arp::stepLengthBeats (settings.rate, settings.feel);

    // Transport sync: adopt the host beat and re-align the step grid to it.
    const bool synced = hostPlaying && hostBeat >= 0.0;
    if (synced)
    {
        const bool jumped = std::abs (hostBeat - beatPos) > beatsPerSample * 4.0;
        beatPos = hostBeat;
        if (jumped || std::abs (stepBeats - lastStepBeats) > 1.0e-9)
            nextStepBeat = alignUp (beatPos, stepBeats);
    }
    else if (std::abs (stepBeats - lastStepBeats) > 1.0e-9 && running)
    {
        nextStepBeat = alignUp (beatPos, stepBeats);
    }
    lastStepBeats = stepBeats;

    const double blockStartBeat = beatPos;
    int cursor = 0;

    auto runClockUntil = [&] (int endSample)
    {
        if (! running || seqLen <= 0)
        {
            // idle: keep the grid moving so the next chord starts on time
            const double endBeat = blockStartBeat + endSample * beatsPerSample;
            if (nextStepBeat < endBeat)
                nextStepBeat = alignUp (endBeat, stepBeats);
            return;
        }

        while (count < kMaxEvents)
        {
            double stepTime = nextStepBeat;
            if (settings.swing > 0.001f && (stepCounter % 2) == 1)
                stepTime += stepBeats * settings.swing * 0.5;

            int stepSample = (int) std::floor ((stepTime - blockStartBeat) / beatsPerSample + 1.0e-9);
            if (stepSample >= endSample)
                break;
            if (stepSample < cursor)
                stepSample = cursor;

            emitStep (stepSample, stepBeats, beatsPerSample, out, count);
            nextStepBeat += stepBeats;
        }
    };

    for (const auto meta : in)
    {
        const auto msg = meta.getMessage();
        const int pos = juce::jlimit (0, numSamples, meta.samplePosition);

        if (! (msg.isNoteOnOrOff() || msg.isAllNotesOff() || msg.isAllSoundOff()))
            continue;

        runClockUntil (pos);
        cursor = pos;
        const double beatAtEvent = blockStartBeat + pos * beatsPerSample;

        if (msg.isNoteOn() && msg.getVelocity() > 0)
        {
            const bool wasRunning = running;
            noteOn (msg.getNoteNumber(), msg.getFloatVelocity(), beatAtEvent);
            if (! wasRunning && running && synced)
                nextStepBeat = alignUp (beatAtEvent, stepBeats); // host grid wins when playing
        }
        else if (msg.isNoteOff() || (msg.isNoteOn() && msg.getVelocity() == 0))
        {
            noteOff (msg.getNoteNumber());
        }
        else // all notes / all sound off
        {
            for (int i = 0; i < numPendingOffs && count < kMaxEvents; ++i)
                out[count++] = { pos, false, pendingOffs[i].note, 0.f, pendingOffs[i].sliceIndex };
            numPendingOffs = 0;
            clearHeld();
            std::fill (std::begin (physical), std::end (physical), false);
            physicalCount = 0;
            chordComplete = false;
            rebuildSequence();
        }
    }

    runClockUntil (numSamples);

    // note-offs that fall inside this block
    int kept = 0;
    for (int i = 0; i < numPendingOffs; ++i)
    {
        auto& p = pendingOffs[i];
        if (p.samplesLeft < numSamples)
        {
            if (count < kMaxEvents)
                out[count++] = { juce::jmax (0, p.samplesLeft), false, p.note, 0.f, p.sliceIndex };
        }
        else
        {
            p.samplesLeft -= numSamples;
            pendingOffs[kept++] = p;
        }
    }
    numPendingOffs = kept;

    beatPos = blockStartBeat + numSamples * beatsPerSample;

    if (count > 1)
    {
        std::sort (out, out + count, [] (const Event& a, const Event& b)
        {
            if (a.samplePos != b.samplePos)
                return a.samplePos < b.samplePos;
            return (! a.noteOn) && b.noteOn; // offs first
        });
    }

    publishUiState();
    return count;
}
