#include <juce_core/juce_core.h>
#include "DSP/SamplerEngine.h"
#include "DSP/SynthEngine.h"
#include "DSP/TextureEngine.h"
#include "State/SampleLibrary.h"
#include <chrono>
#include <vector>

namespace
{
double benchSamplerVoices (int numVoices, int blocks, int blockSize, double sampleRate)
{
    SamplerEngine engine;
    juce::dsp::ProcessSpec spec { sampleRate, static_cast<juce::uint32> (blockSize), 2 };
    engine.prepare (spec);
    engine.setPolyphony (numVoices);

    SampleLibrary lib;
    const float tone[] { 0.f, 0.25f, 0.5f, 0.75f, -0.25f, -0.5f, -0.75f, 0.f };
    lib.loadFromMemory (tone, sizeof (tone), "bench", 60);
    lib.publish();
    engine.setSampleSnapshot (lib.getPublishedSnapshot());

    for (int n = 0; n < numVoices; ++n)
        engine.noteOn (60 + (n % 12), 0.9f, false, 0.f);

    juce::AudioBuffer<float> buf (2, blockSize);
    const auto t0 = std::chrono::steady_clock::now();

    for (int b = 0; b < blocks; ++b)
    {
        buf.clear();
        engine.process (buf);
    }

    const auto us = std::chrono::duration_cast<std::chrono::microseconds> (
        std::chrono::steady_clock::now() - t0).count();
    return static_cast<double> (us) / static_cast<double> (blocks);
}

double benchIdleCallback (int blocks, int blockSize, double sampleRate)
{
    SamplerEngine sampler;
    SynthEngine synth;
    juce::dsp::ProcessSpec spec { sampleRate, static_cast<juce::uint32> (blockSize), 2 };
    sampler.prepare (spec);
    synth.prepare (spec);

    juce::AudioBuffer<float> buf (2, blockSize);
    const auto t0 = std::chrono::steady_clock::now();

    for (int b = 0; b < blocks; ++b)
    {
        buf.clear();
        if (sampler.hasActiveVoices())
            sampler.process (buf);
        if (synth.hasActiveVoices())
            synth.process (buf);
    }

    const auto us = std::chrono::duration_cast<std::chrono::microseconds> (
        std::chrono::steady_clock::now() - t0).count();
    return static_cast<double> (us) / static_cast<double> (blocks);
}
} // namespace

class PerformanceBenchmarkTests final : public juce::UnitTest
{
public:
    PerformanceBenchmarkTests() : juce::UnitTest ("PerformanceBenchmark", "Benchmark") {}

    void runTest() override
    {
        beginTest ("Sampler 16-voice block timing (informational)");
        const double us128 = benchSamplerVoices (16, 500, 128, 48000.0);
        logMessage ("avg us/block @ 48 kHz, 128 samples, 16 voices: " + juce::String (us128, 1));

        beginTest ("Idle engine skip timing (informational)");
        const double idleUs = benchIdleCallback (2000, 128, 48000.0);
        logMessage ("avg us/block idle skip @ 48 kHz, 128 samples: " + juce::String (idleUs, 2));

        expect (us128 > 0.0);
        expect (idleUs >= 0.0);
    }
};

static PerformanceBenchmarkTests performanceBenchmarkTests;
