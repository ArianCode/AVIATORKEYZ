#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <atomic>
#include <vector>

// =============================================================================
//  RollingSampler — a live 30-second window of the plugin's own output.
//
//  Audio rolls in at the right and is deleted as it falls off the left: the ring
//  holds exactly kWindowSeconds, so what you can see is exactly what you can
//  still grab. Positions are absolute frame counts since prepare() (never
//  buffer indices), so a selection the user made keeps pointing at the same
//  audio while the window scrolls under it — and `oldestFrame()` says when that
//  audio has aged out for good.
//
//  Audio thread: write() appends a block to the ring and to a decimated min/max
//  envelope. Both are pre-allocated in prepare(); no locks, no allocation.
//
//  Message thread: readEnvelope() feeds the scope's waveform at repaint rate
//  without touching the sample ring (30 s at 48 kHz is 1.4M frames — far too
//  many to scan per frame). snapshotRange() copies real samples out for export.
// =============================================================================

class RollingSampler
{
public:
    /** The live window: 30 s of audio that rolls in at the right and is deleted
        off the left. Fixed — what you can see is exactly what you can still
        grab. ~11 MB at 48 kHz, ~45 MB at 192 kHz, per plugin instance. */
    static constexpr double kWindowSeconds = 30.0;
    static constexpr int    kNumChannels = 2;
    /** Frames per envelope bucket — one column of the drawn waveform. */
    static constexpr int    kBucketFrames = 128;

    struct Bucket
    {
        float min { 0.f };
        float max { 0.f };
    };

    RollingSampler() = default;

    void prepare (double sampleRate, int /*maxBlockSize*/)
    {
        sr = sampleRate > 1000.0 ? sampleRate : 44100.0;

        capacity = juce::jmax (1024, (int) std::ceil (kWindowSeconds * sr));
        ring.setSize (kNumChannels, capacity, false, true, true);

        numBuckets = capacity / kBucketFrames + 2;
        envelope.assign ((size_t) numBuckets, Bucket {});

        prepared = true;
        reset();
    }

    double getWindowSeconds() const noexcept { return kWindowSeconds; }

    void reset() noexcept
    {
        if (! prepared)
            return;
        ring.clear();
        std::fill (envelope.begin(), envelope.end(), Bucket {});
        framesTotal.store (0, std::memory_order_relaxed);
        bucketsTotal.store (0, std::memory_order_relaxed);
        bucketFill = 0;
        bucketMin = 0.f;
        bucketMax = 0.f;
        peak.store (0.f, std::memory_order_relaxed);
    }

    // -------------------------------------------------------------------------
    //  audio thread
    // -------------------------------------------------------------------------
    void write (const juce::AudioBuffer<float>& buffer) noexcept
    {
        if (! prepared || ! armed.load (std::memory_order_relaxed))
            return;

        const int n = buffer.getNumSamples();
        if (n <= 0 || buffer.getNumChannels() < 1)
            return;

        const int64_t startFrame = framesTotal.load (std::memory_order_relaxed);
        int pos = (int) (startFrame % capacity);

        float blockPeak = 0.f;
        int done = 0;
        while (done < n)
        {
            const int chunk = juce::jmin (n - done, capacity - pos);
            for (int ch = 0; ch < kNumChannels; ++ch)
            {
                const int srcCh = juce::jmin (ch, buffer.getNumChannels() - 1);
                ring.copyFrom (ch, pos, buffer, srcCh, done, chunk);
            }
            blockPeak = juce::jmax (blockPeak, ring.getMagnitude (0, pos, chunk));
            pos = (pos + chunk) % capacity;
            done += chunk;
        }

        appendEnvelope (buffer, n);

        // Publish the frame count last: everything behind it is now readable.
        framesTotal.store (startFrame + n, std::memory_order_release);

        const float prev = peak.load (std::memory_order_relaxed);
        peak.store (juce::jmax (blockPeak, prev * 0.85f), std::memory_order_relaxed);
    }

    void setArmed (bool shouldArm) noexcept { armed.store (shouldArm, std::memory_order_relaxed); }
    bool isArmed() const noexcept { return armed.load (std::memory_order_relaxed); }

    // -------------------------------------------------------------------------
    //  any thread — absolute positions
    // -------------------------------------------------------------------------
    /** Total frames ever captured: the right edge of the window ("now"). */
    int64_t nowFrame() const noexcept { return framesTotal.load (std::memory_order_acquire); }

    /** Oldest frame still in the ring. Anything before this has been deleted. */
    int64_t oldestFrame() const noexcept
    {
        return juce::jmax ((int64_t) 0, nowFrame() - capacity);
    }

    double getBufferedSeconds() const noexcept
    {
        return (double) (nowFrame() - oldestFrame()) / sr;
    }

    float  getPeakLevel() const noexcept { return peak.load (std::memory_order_relaxed); }
    double getSampleRate() const noexcept { return sr; }
    int    getCapacityFrames() const noexcept { return capacity; }

    // -------------------------------------------------------------------------
    //  message thread
    // -------------------------------------------------------------------------
    /** Fills `out` with one min/max pair per column across [fromFrame, toFrame).
        Columns whose audio has aged out (or was never written) come back flat.
        Reads only the decimated envelope, so this is cheap enough per repaint. */
    void readEnvelope (Bucket* out, int numColumns, int64_t fromFrame, int64_t toFrame) const noexcept
    {
        if (out == nullptr || numColumns <= 0)
            return;
        for (int i = 0; i < numColumns; ++i)
            out[i] = Bucket {};

        if (! prepared || toFrame <= fromFrame)
            return;

        const int64_t completed = bucketsTotal.load (std::memory_order_acquire);
        const int64_t oldestBucket = juce::jmax ((int64_t) 0, completed - (numBuckets - 2));
        const int64_t span = toFrame - fromFrame;

        for (int i = 0; i < numColumns; ++i)
        {
            // Frame range this column covers, mapped to whole buckets.
            const int64_t f0 = fromFrame + span * i / numColumns;
            const int64_t f1 = fromFrame + span * (i + 1) / numColumns;
            int64_t b0 = f0 / kBucketFrames;
            int64_t b1 = juce::jmax (b0 + 1, f1 / kBucketFrames);

            b0 = juce::jmax (b0, oldestBucket);
            b1 = juce::jmin (b1, completed);
            if (b1 <= b0)
                continue;

            float lo = 0.f, hi = 0.f;
            for (int64_t b = b0; b < b1; ++b)
            {
                const auto& e = envelope[(size_t) (b % numBuckets)];
                lo = juce::jmin (lo, e.min);
                hi = juce::jmax (hi, e.max);
            }
            out[i] = { lo, hi };
        }
    }

    /** Copies samples for the absolute range [fromFrame, toFrame) into `dest`,
        clamped to what is still held. Returns false when the range has aged out
        entirely or is too short to be useful. */
    bool snapshotRange (juce::AudioBuffer<float>& dest, int64_t fromFrame, int64_t toFrame) const
    {
        if (! prepared)
            return false;

        const int64_t now = nowFrame();
        const int64_t oldest = juce::jmax ((int64_t) 0, now - capacity);
        const int64_t a = juce::jmax (fromFrame, oldest);
        const int64_t b = juce::jmin (toFrame, now);
        const int64_t want = b - a;
        if (want < 64)
            return false;

        dest.setSize (kNumChannels, (int) want, false, true, true);
        const int start = (int) (a % capacity);
        const int first = juce::jmin ((int) want, capacity - start);
        for (int ch = 0; ch < kNumChannels; ++ch)
        {
            dest.copyFrom (ch, 0, ring, ch, start, first);
            if ((int) want > first)
                dest.copyFrom (ch, first, ring, ch, 0, (int) want - first);
        }
        return true;
    }

    /** Convenience: the most recent `seconds`. */
    bool snapshot (juce::AudioBuffer<float>& dest, double seconds) const
    {
        const int64_t now = nowFrame();
        const int64_t want = (int64_t) std::llround (juce::jlimit (0.0, kWindowSeconds, seconds) * sr);
        return snapshotRange (dest, now - want, now);
    }

private:
    /** Decimated min/max, advanced in lockstep with the sample ring. */
    void appendEnvelope (const juce::AudioBuffer<float>& buffer, int n) noexcept
    {
        const float* L = buffer.getReadPointer (0);
        const float* R = buffer.getNumChannels() > 1 ? buffer.getReadPointer (1) : L;

        for (int i = 0; i < n; ++i)
        {
            const float lo = juce::jmin (L[i], R[i]);
            const float hi = juce::jmax (L[i], R[i]);
            bucketMin = juce::jmin (bucketMin, lo);
            bucketMax = juce::jmax (bucketMax, hi);

            if (++bucketFill >= kBucketFrames)
            {
                const int64_t index = bucketsTotal.load (std::memory_order_relaxed);
                envelope[(size_t) (index % numBuckets)] = { bucketMin, bucketMax };
                bucketsTotal.store (index + 1, std::memory_order_release);
                bucketFill = 0;
                bucketMin = 0.f;
                bucketMax = 0.f;
            }
        }
    }

    juce::AudioBuffer<float> ring;
    std::vector<Bucket>      envelope;

    double sr { 44100.0 };
    bool   prepared { false };
    int    capacity { 0 };
    int    numBuckets { 0 };

    std::atomic<int64_t> framesTotal { 0 };   // audio writes, UI reads
    std::atomic<int64_t> bucketsTotal { 0 };  // completed envelope buckets
    int   bucketFill { 0 };                   // audio thread only
    float bucketMin { 0.f };
    float bucketMax { 0.f };

    std::atomic<float> peak { 0.f };
    std::atomic<bool>  armed { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RollingSampler)
};
