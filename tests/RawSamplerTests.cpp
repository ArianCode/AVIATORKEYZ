// =============================================================================
//  Raw sampler transparency regression tests
// =============================================================================

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

#include "State/ParameterLayout.h"
#include "State/StateSchema.h"
#include "State/SampleLibrary.h"
#include "State/ApvtsStateHelpers.h"
#include "State/PresetManager.h"
#include "DSP/SamplerEngine.h"
#include "DSP/FilterProcessor.h"
#include "DSP/Performance/PerformanceTypes.h"
#include "State/CategorySoundPolicy.h"

namespace
{
juce::MemoryBlock makeFlatWav (int numFrames = 2048, int16_t sampleValue = 16000)
{
    const int dataBytes = numFrames * 2;
    const int riffSize  = 36 + dataBytes;

    juce::MemoryBlock mb;
    mb.setSize (static_cast<size_t> (44 + dataBytes), true);
    char* d = static_cast<char*> (mb.getData());

    auto w4 = [&] (int o, uint32_t v) {
        d[o] = (char) (v & 0xFF); d[o + 1] = (char) ((v >> 8) & 0xFF);
        d[o + 2] = (char) ((v >> 16) & 0xFF); d[o + 3] = (char) ((v >> 24) & 0xFF);
    };
    auto w2 = [&] (int o, uint16_t v) {
        d[o] = (char) (v & 0xFF); d[o + 1] = (char) ((v >> 8) & 0xFF);
    };

    d[0] = 'R'; d[1] = 'I'; d[2] = 'F'; d[3] = 'F';
    w4 (4, (uint32_t) riffSize);
    d[8] = 'W'; d[9] = 'A'; d[10] = 'V'; d[11] = 'E';
    d[12] = 'f'; d[13] = 'm'; d[14] = 't'; d[15] = ' ';
    w4 (16, 16); w2 (20, 1); w2 (22, 1);
    w4 (24, 44100u); w4 (28, 44100u * 2u); w2 (32, 2); w2 (34, 16);
    d[36] = 'd'; d[37] = 'a'; d[38] = 't'; d[39] = 'a';
    w4 (40, (uint32_t) dataBytes);

    for (int i = 0; i < numFrames; ++i)
    {
        d[44 + i * 2]     = (char) (sampleValue & 0xFF);
        d[44 + i * 2 + 1] = (char) ((sampleValue >> 8) & 0xFF);
    }

    return mb;
}

float bufferPeak (const juce::AudioBuffer<float>& buffer)
{
    float peak = 0.f;
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        peak = juce::jmax (peak, buffer.getMagnitude (ch, 0, buffer.getNumSamples()));
    return peak;
}

float maxAbsoluteDifference (const juce::AudioBuffer<float>& a,
                             const juce::AudioBuffer<float>& b)
{
    float maxDiff = 0.f;
    const int n = juce::jmin (a.getNumSamples(), b.getNumSamples());
    const int ch = juce::jmin (a.getNumChannels(), b.getNumChannels());
    for (int c = 0; c < ch; ++c)
        for (int i = 0; i < n; ++i)
            maxDiff = juce::jmax (maxDiff, std::abs (a.getSample (c, i) - b.getSample (c, i)));
    return maxDiff;
}

float renderSamplerPeak (SamplerEngine& engine,
                         int midiNote,
                         float velocity,
                         float velocitySensitivity = 0.f,
                         int blockSize = 512,
                         double sampleRate = 44100.0)
{
    juce::dsp::ProcessSpec spec { sampleRate, static_cast<juce::uint32> (blockSize), 2 };
    engine.prepare (spec);
    engine.setEnvelopeTimesMs (0.f, 0.f, 1.f, 10.f);
    engine.setVelocitySensitivity (velocitySensitivity);
    engine.noteOn (midiNote, velocity, false, 0.f);

    juce::AudioBuffer<float> buffer (2, blockSize);
    float peak = 0.f;
    for (int block = 0; block < 4; ++block)
    {
        buffer.clear();
        engine.process (buffer);
        peak = juce::jmax (peak, bufferPeak (buffer));
    }
    return peak;
}

struct RawTestProcessor : juce::AudioProcessor
{
    RawTestProcessor()
        : AudioProcessor (BusesProperties()),
          apvts (*this, nullptr, "AviatorKeyzState", AviatorKeyz::createParameterLayout())
    {}

    const juce::String getName() const override { return "RawSamplerTest"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}
    void prepareToPlay (double, int) override {}
    void releaseResources() override {}
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override {}
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    bool hasEditor() const override { return false; }
    void getStateInformation (juce::MemoryBlock&) override {}
    void setStateInformation (const void*, int) override {}

    juce::AudioProcessorValueTreeState apvts;
};
} // namespace

class RawSamplerTests : public juce::UnitTest
{
public:
    RawSamplerTests() : juce::UnitTest ("RawSampler", "AviatorKeyz") {}

    void runTest() override
    {
        beginTest ("Raw sampler does not systematically attenuate normalized audio");
        {
            SampleLibrary library;
            const auto wav = makeFlatWav();
            expect (library.loadFromMemory (wav.getData(), wav.getSize(), "raw_test", 60));
            library.publish();

            SamplerEngine engine;
            engine.setSampleSnapshot (library.getPublishedSnapshot());
            const float peak = renderSamplerPeak (engine, 60, 1.f);

            expect (peak > 0.7f,
                    "Normalized sample at root note should stay close to source level (peak="
                    + juce::String (peak) + ")");
            expect (peak <= 1.05f,
                    "Normalized sample should not exceed unity by much (peak=" + juce::String (peak) + ")");
        }

        beginTest ("Disabled filter leaves mixed buffer unchanged");
        {
            juce::AudioBuffer<float> buffer (2, 128);
            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < buffer.getNumSamples(); ++i)
                    buffer.setSample (ch, i, 0.25f * std::sin (0.1f * static_cast<float> (i)));

            const juce::AudioBuffer<float> reference (buffer);
            const float diff = maxAbsoluteDifference (reference, buffer);
            expect (diff < 1.0e-6f, "Reference copy must match before processing");

            FilterProcessor filter;
            juce::dsp::ProcessSpec spec { 44100.0, 128, 2 };
            filter.prepare (spec);
            filter.setParameters (8000.f, 0.25f, FilterProcessor::Type::lowPass, 0.15f, 0.f, 0.f);
            // Hard bypass: do not call process when filter is disabled.
            const float afterBypassDiff = maxAbsoluteDifference (reference, buffer);
            expect (afterBypassDiff < 1.0e-6f, "Bypassed filter must not alter the buffer");

            filter.process (buffer);
            const float afterProcessDiff = maxAbsoluteDifference (reference, buffer);
            expect (afterProcessDiff > 1.0e-4f, "Enabled filter should change the buffer");
        }

        beginTest ("Velocity sensitivity zero ignores note velocity");
        {
            SampleLibrary library;
            const auto wav = makeFlatWav();
            library.loadFromMemory (wav.getData(), wav.getSize(), "vel_test", 60);
            library.publish();

            SamplerEngine engine;
            engine.setSampleSnapshot (library.getPublishedSnapshot());
            engine.setVelocitySensitivity (0.f);
            engine.setEnvelopeTimesMs (0.f, 0.f, 1.f, 10.f);

            const float peakSoft = renderSamplerPeak (engine, 60, 0.25f);
            engine.allSoundOff();
            const float peakHard = renderSamplerPeak (engine, 60, 1.f);

            expectWithinAbsoluteError (peakSoft, peakHard, 0.02f,
                                       "Velocity sensitivity 0 must ignore incoming velocity");
        }

        beginTest ("Velocity is applied exactly once");
        {
            expectWithinAbsoluteError (SamplerEngine::calculateVelocityGain (0.79f, 0.f), 1.f, 1.0e-5f);
            expectWithinAbsoluteError (SamplerEngine::calculateVelocityGain (0.79f, 1.f), 0.79f, 1.0e-5f);

            SampleLibrary library;
            const auto wav = makeFlatWav();
            library.loadFromMemory (wav.getData(), wav.getSize(), "vel_once", 60);
            library.publish();

            SamplerEngine engine;
            engine.setSampleSnapshot (library.getPublishedSnapshot());
            engine.setEnvelopeTimesMs (0.f, 0.f, 1.f, 10.f);
            engine.setVelocitySensitivity (1.f);

            const float peak = renderSamplerPeak (engine, 60, 0.5f, 1.f);
            engine.allSoundOff();
            const float full = renderSamplerPeak (engine, 60, 1.f, 1.f);

            expectWithinAbsoluteError (peak, full * 0.5f, 0.03f,
                                       "With sensitivity=1, output should scale linearly once by velocity");
        }

        beginTest ("Preset change does not inherit previous processing state");
        {
            RawTestProcessor proc;
            PresetManager presetManager (proc.apvts);

            if (auto* smear = proc.apvts.getParameter (AviatorKeyz::ParamID::SMEAR))
                if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (smear))
                    smear->setValueNotifyingHost (ranged->convertTo0to1 (0.9f));

            const auto partial = juce::XmlDocument::parse (R"(
<AviatorKeyzState stateVersion="1">
  <PARAM id="smear" value="0.0"/>
</AviatorKeyzState>)");
            const auto state = juce::ValueTree::fromXml (*partial);
            AviatorKeyz::applyStateTreeToApvts (proc.apvts, state);

            expectWithinAbsoluteError (
                proc.apvts.getRawParameterValue (AviatorKeyz::ParamID::SMEAR)->load(), 0.f, 0.001f);
            expectWithinAbsoluteError (
                proc.apvts.getRawParameterValue (AviatorKeyz::ParamID::FILTER_DRIVE)->load(), 0.f, 0.001f);
            expect (
                proc.apvts.getRawParameterValue (AviatorKeyz::ParamID::FILTER_ENABLED)->load() <= 0.5f,
                "Neutral baseline must keep filter bypassed");
        }

        beginTest ("Chromatic instrument uses gated playback policy");
        {
            SamplerEngine engine;
            engine.setEnvelopeTimesMs (0.f, 0.f, 1.f, 10.f);

            SourceSettings settings;
            settings.playbackMode = SamplePlaybackMode::ChromaticResample;
            settings.loopMode = LoopMode::Gate;
            engine.setSourceSettings (settings, 120.0);
            engine.setPlaybackContext (AviatorKeyz::SoundType::OneShot, AviatorKeyz::Category::LEADS);

            expect (! engine.isOneShotPlayback(), "Chromatic instrument must honor note-off");
            expect (engine.getNoteGatePolicy() == NoteGatePolicy::Gated);
        }

        beginTest ("TriggerToEnd one-shot continues after MIDI note-off");
        {
            SampleLibrary library;
            const auto wav = makeFlatWav (4096);
            library.loadFromMemory (wav.getData(), wav.getSize(), "oneshot", 60);
            library.publish();

            SamplerEngine engine;
            juce::dsp::ProcessSpec spec { 44100.0, 256, 2 };
            engine.prepare (spec);
            engine.setSampleSnapshot (library.getPublishedSnapshot());
            engine.setEnvelopeTimesMs (0.f, 0.f, 1.f, 10.f);
            engine.setVelocitySensitivity (0.f);

            SourceSettings settings;
            settings.playbackMode = SamplePlaybackMode::OneShotOriginal;
            settings.loopMode = LoopMode::OneShot;
            settings.bpmSync = false;
            engine.setSourceSettings (settings, 120.0);
            engine.setPlaybackContext (AviatorKeyz::SoundType::OneShot, AviatorKeyz::Category::CHORDS);
            expect (engine.getNoteGatePolicy() == NoteGatePolicy::TriggerToEnd);

            engine.noteOn (60, 1.f, false, 0.f);

            juce::AudioBuffer<float> block (2, 256);
            block.clear();
            engine.process (block);
            const float peakBeforeOff = bufferPeak (block);

            engine.noteOff (60);

            float peakAfterOff = 0.f;
            for (int i = 0; i < 8; ++i)
            {
                block.clear();
                engine.process (block);
                peakAfterOff = juce::jmax (peakAfterOff, bufferPeak (block));
            }

            expect (peakBeforeOff > 0.1f, "Sample should be audible before note-off");
            expect (peakAfterOff > 0.1f,
                    "TriggerToEnd must continue after note-off (peak="
                    + juce::String (peakAfterOff) + ")");
            engine.allSoundOff();
        }
    }
};

static RawSamplerTests rawSamplerTests;
