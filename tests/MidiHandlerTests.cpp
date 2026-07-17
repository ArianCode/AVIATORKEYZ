// =============================================================================
//  MidiHandler unit tests
// =============================================================================

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include "MIDI/MidiHandler.h"
#include "DSP/SamplerEngine.h"
#include "DSP/SynthEngine.h"
#include "DSP/Performance/PerformanceTypes.h"
#include "State/SampleLibrary.h"

namespace
{
struct TestSampleSnapshot
{
    SampleLibrary::AudioSnapshot snapshot;
    SampleLibrary::AudioRegion   region;

    void setMono (const float* data, int numFrames) noexcept
    {
        region.data = data;
        region.numFrames = numFrames;
        region.rootNote = 60;
        snapshot.regions.clear();
        snapshot.regions.push_back (region);
    }
};

float sineBuf[1024];

void initSine()
{
    static bool done = false;
    if (done) return;
    done = true;
    for (int i = 0; i < 1024; ++i)
        sineBuf[i] = std::sin (juce::MathConstants<float>::twoPi * 440.f * i / 44100.f);
}

void setupGatedSampler (SamplerEngine& engine, TestSampleSnapshot& snap)
{
    juce::dsp::ProcessSpec spec { 44100.0, 256, 2 };
    engine.prepare (spec);
    snap.setMono (sineBuf, 1024);
    engine.setSampleSnapshot (&snap.snapshot);
    engine.setEnvelopeTimesMs (0.f, 0.f, 1.f, 20.f);

    SourceSettings settings;
    settings.playbackMode = SamplePlaybackMode::ChromaticResample;
    settings.loopMode = LoopMode::Gate;
    engine.setSourceSettings (settings, 120.0);
    engine.setPlaybackContext (AviatorKeyz::SoundType::OneShot, AviatorKeyz::Category::LEADS);
}
} // namespace

class MidiHandlerTests : public juce::UnitTest
{
public:
    MidiHandlerTests() : juce::UnitTest ("MidiHandler", "AviatorKeyz") {}

    void runTest() override
    {
        initSine();

        beginTest ("Note-on velocity zero triggers note-off when sustain is up");
        {
            MidiHandler handler;
            SamplerEngine sampler;
            SynthEngine synth;
            TestSampleSnapshot snap;
            setupGatedSampler (sampler, snap);

            juce::MidiBuffer midi;
            midi.addEvent (juce::MidiMessage::noteOn (1, 60, 1.f), 0);
            handler.process (midi, sampler, synth, false, 0.f, 0.f);
            expect (sampler.getNumActiveVoices() > 0);

            midi.clear();
            midi.addEvent (juce::MidiMessage::noteOn (1, 60, 0.f), 0);
            handler.process (midi, sampler, synth, false, 0.f, 0.f);

            juce::AudioBuffer<float> buf (2, 256);
            for (int i = 0; i < 64; ++i)
            {
                buf.clear();
                sampler.process (buf);
            }
            expect (sampler.getNumActiveVoices() == 0);
            sampler.allSoundOff();
        }

        beginTest ("Sustain pedal defers note-off until pedal release");
        {
            MidiHandler handler;
            SamplerEngine sampler;
            SynthEngine synth;
            TestSampleSnapshot snap;
            setupGatedSampler (sampler, snap);

            juce::MidiBuffer midi;
            midi.addEvent (juce::MidiMessage::noteOn (1, 60, 1.f), 0);
            handler.process (midi, sampler, synth, false, 0.f, 0.f);

            midi.clear();
            midi.addEvent (juce::MidiMessage::controllerEvent (1, 64, 127), 0);
            midi.addEvent (juce::MidiMessage::noteOff (1, 60), 0);
            handler.process (midi, sampler, synth, false, 0.f, 0.f);
            expect (sampler.getNumActiveVoices() > 0);

            midi.clear();
            midi.addEvent (juce::MidiMessage::controllerEvent (1, 64, 0), 0);
            handler.process (midi, sampler, synth, false, 0.f, 0.f);

            juce::AudioBuffer<float> buf (2, 256);
            for (int i = 0; i < 64; ++i)
            {
                buf.clear();
                sampler.process (buf);
            }
            sampler.allSoundOff();
        }

        beginTest ("All sound off kills voices immediately");
        {
            MidiHandler handler;
            SamplerEngine sampler;
            SynthEngine synth;
            TestSampleSnapshot snap;
            setupGatedSampler (sampler, snap);

            juce::MidiBuffer midi;
            midi.addEvent (juce::MidiMessage::noteOn (1, 60, 1.f), 0);
            handler.process (midi, sampler, synth, false, 0.f, 0.f);

            midi.clear();
            midi.addEvent (juce::MidiMessage::allSoundOff (1), 0);
            handler.process (midi, sampler, synth, false, 0.f, 0.f);
            expectEquals (sampler.getNumActiveVoices(), 0);
        }
    }
};

static MidiHandlerTests midiHandlerTests;
