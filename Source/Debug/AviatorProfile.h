#pragma once

#include <cstdint>

#if defined (AVIATORKEYZ_PROFILE) && AVIATORKEYZ_PROFILE

#include <atomic>
#include <chrono>

/** Lightweight audio-thread profiling — compile with -DAVIATORKEYZ_PROFILE=1. */
namespace AviatorProfile
{
struct ScopedBlockUs
{
    std::chrono::steady_clock::time_point start;
    std::atomic<uint64_t>* dest;

    explicit ScopedBlockUs (std::atomic<uint64_t>* d) noexcept
        : start (std::chrono::steady_clock::now()), dest (d) {}

    ~ScopedBlockUs()
    {
        const auto elapsed = std::chrono::steady_clock::now() - start;
        const auto us = static_cast<uint64_t> (
            std::chrono::duration_cast<std::chrono::microseconds> (elapsed).count());
        dest->store (us, std::memory_order_relaxed);
    }
};

struct PeakTracker
{
    std::atomic<uint64_t> lastUs { 0 };
    std::atomic<uint64_t> peakUs { 0 };

    void update (uint64_t us) noexcept
    {
        lastUs.store (us, std::memory_order_relaxed);
        uint64_t prev = peakUs.load (std::memory_order_relaxed);
        while (us > prev && ! peakUs.compare_exchange_weak (prev, us, std::memory_order_relaxed)) {}
    }
};

inline void recordBlock (PeakTracker& tracker, uint64_t us) noexcept
{
    tracker.update (us);
}

} // namespace AviatorProfile

  #define AK_PROFILE_BLOCK_US(dest) \
    AviatorProfile::ScopedBlockUs _akProfileScope_##__LINE__ (dest)

#else

  #define AK_PROFILE_BLOCK_US(dest)

#endif
