// =============================================================================
//  Voice lifecycle, playback policy, and phrase behavior tests
// =============================================================================

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include "DSP/SamplerEngine.h"
#include "DSP/Performance/PerformanceTypes.h"
#include "State/CategorySoundPolicy.h"
#include "State/SampleLibrary.h"

namespace TestSamples
{
static constexpr int kFrames = 4096;
static float sine4096[kFrames] = {};

static void init()
{
    static bool done = false;
    if (done) return;
    done = true;
    for (int i = 0; i < kFrames; ++i)
        sine4096[i] = std::sin (juce::MathConstants<float>::twoPi
                                * 440.0f * static_cast<float> (i) / 44100.0f);
}
} // namespace TestSamples

namespace
{
struct TestSampleSnapshot
{
    SampleLibrary::AudioSnapshot snapshot;
    SampleLibrary::AudioRegion   region;

    void setMono (const float* data, int numFrames, int rootNote = 60) noexcept
    {
        region.data = data;
        region.numFrames = numFrames;
        region.rootNote = rootNote;
        region.noteMin = 0;
        region.noteMax = 127;
        region.fileSampleRate = 44100.0;
        snapshot.regions.clear();
        snapshot.regions.push_back (region);
    }
};

float bufferPeak (const juce::AudioBuffer<float>& buffer)
{
    float peak = 0.f;
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        peak = std::max (peak, buffer.getMagnitude (ch, 0, buffer.getNumSamples()));
    return peak;
}

float estimateZeroCrossingHz (const juce::AudioBuffer<float>& buffer, int numSamples, double sampleRate)
{
    int crossings = 0;
    const float* data = buffer.getReadPointer (0);
    for (int i = 1; i < numSamples; ++i)
    {
        if ((data[i - 1] >= 0.f && data[i] < 0.f)
            || (data[i - 1] < 0.f && data[i] >= 0.f))
            ++crossings;
    }
    return static_cast<float> (crossings) * 0.5f * static_cast<float> (sampleRate)
           / static_cast<float> (numSamples);
}

int renderUntilSilent (SamplerEngine& engine, int maxSamples = 44100 * 8)
{
    juce::AudioBuffer<float> buf (2, 256);
    int rendered = 0;
    while (engine.hasActiveVoices() && rendered < maxSamples)
    {
        buf.clear();
        engine.process (buf);
        rendered += buf.getNumSamples();
    }
    return rendered;
}

void configurePhraseEngine (SamplerEngine& engine, TestSampleSnapshot& snap) noexcept
{
    juce::dsp::ProcessSpec spec { 44100.0, 256, 2 };
    engine.prepare (spec);
    snap.setMono (TestSamples::sine4096, TestSamples::kFrames, 60);
    engine.setSampleSnapshot (&snap.snapshot);
    engine.setEnvelopeTimesMs (0.f, 0.f, 1.f, 20.f);

    SourceSettings settings;
    settings.playbackMode = SamplePlaybackMode::PhraseOriginal;
    settings.loopMode = LoopMode::Gate;
    settings.bpmSync = false;
    settings.keytrack = false;
    settings.start = 0.f;
    settings.end = 1.f;
    engine.setSourceSettings (settings, 120.0);
    engine.setPlaybackContext (AviatorKeyz::SoundType::Phrase, AviatorKeyz::Category::ARPS);
}

void configureInstrumentEngine (SamplerEngine& engine, TestSampleSnapshot& snap) noexcept
{
    juce::dsp::ProcessSpec spec { 44100.0, 256, 2 };
    engine.prepare (spec);
    snap.setMono (TestSamples::sine4096, TestSamples::kFrames, 60);
    engine.setSampleSnapshot (&snap.snapshot);
    engine.setEnvelopeTimesMs (0.f, 0.f, 1.f, 20.f);

    SourceSettings settings;
    settings.playbackMode = SamplePlaybackMode::ChromaticResample;
    settings.loopMode = LoopMode::Gate;
    settings.bpmSync = false;
    engine.setSourceSettings (settings, 120.0);
    engine.setPlaybackContext (AviatorKeyz::SoundType::OneShot, AviatorKeyz::Category::LEADS);
}
} // namespace

class PlaybackPolicyTests : public juce::UnitTest
{
public:
    PlaybackPolicyTests() : juce::UnitTest ("PlaybackPolicy", "AviatorKeyz") {}

    void runTest() override
    {
        beginTest ("gatePolicyFor chromatic instrument is Gated");
        {
            expect (AviatorKeyz::gatePolicyFor ("Leads",
                                                AviatorKeyz::SoundType::OneShot,
                                                SamplePlaybackMode::ChromaticResample,
                                                LoopMode::Gate)
                        == NoteGatePolicy::Gated);
        }

        beginTest ("gatePolicyFor true one-shot is TriggerToEnd");
        {
            expect (AviatorKeyz::gatePolicyFor ("Phrases",
                                                AviatorKeyz::SoundType::OneShot,
                                                SamplePlaybackMode::OneShotOriginal,
                                                LoopMode::OneShot)
                        == NoteGatePolicy::TriggerToEnd);
        }

        beginTest ("retriggerPolicyFor phrase is PhraseChoke");
        {
            expect (AviatorKeyz::retriggerPolicyFor ("Arps",
                                                     AviatorKeyz::SoundType::Phrase,
                                                     SamplePlaybackMode::PhraseOriginal)
                        == RetriggerPolicy::PhraseChoke);
        }

        beginTest ("loopModeFor assigns Gate to phrases");
        {
            expect (AviatorKeyz::loopModeFor ("Vocals", AviatorKeyz::SoundType::Phrase) == LoopMode::Gate);
            expect (AviatorKeyz::loopModeFor ("Strings", AviatorKeyz::SoundType::OneShot) == LoopMode::Gate);
            expect (AviatorKeyz::loopModeFor ("Phrases", AviatorKeyz::SoundType::OneShot) == LoopMode::OneShot);
        }
    }
};

class VoiceLifecycleTests : public juce::UnitTest
{
public:
    VoiceLifecycleTests() : juce::UnitTest ("VoiceLifecycle", "AviatorKeyz") {}

    void runTest() override
    {
        TestSamples::init();

        beginTest ("Gated note-off: release finishes and active voices return to zero");
        {
            SamplerEngine engine;
            TestSampleSnapshot snap;
            configureInstrumentEngine (engine, snap);

            engine.noteOn (60, 1.f, false, 0.f);
            juce::AudioBuffer<float> buf (2, 256);
            buf.clear();
            engine.process (buf);
            expect (engine.getNumActiveVoices() > 0);

            engine.noteOff (60);
            renderUntilSilent (engine);
            expectEquals (engine.getNumActiveVoices(), 0);
        }

        beginTest ("Overlapping same pitch: FIFO note-off pairs with correct voice");
        {
            SamplerEngine engine;
            TestSampleSnapshot snap;
            configureInstrumentEngine (engine, snap);

            engine.noteOn (60, 1.f, false, 0.f); // voice A
            engine.noteOn (60, 1.f, false, 0.f); // voice B (A choked)

            juce::AudioBuffer<float> buf (2, 256);
            buf.clear();
            engine.process (buf);
            const float peakBoth = bufferPeak (buf);
            expect (peakBoth > 0.05f);
            expect (engine.getNumActiveVoices() >= 1);

            engine.noteOff (60); // should release A only
            buf.clear();
            engine.process (buf);
            const float peakAfterFirstOff = bufferPeak (buf);
            expect (peakAfterFirstOff > 0.02f, "Voice B must remain after first note-off");

            engine.noteOff (60); // release B
            renderUntilSilent (engine);
            expectEquals (engine.getNumActiveVoices(), 0);
        }

        beginTest ("PhraseOriginal tempo: host BPM scales read increment inversely");
        {
            auto readIncAtHostBpm = [&] (double hostBpm) -> float
            {
                SamplerEngine engine;
                TestSampleSnapshot snap;
                configurePhraseEngine (engine, snap);

                SourceSettings settings;
                settings.playbackMode = SamplePlaybackMode::PhraseOriginal;
                settings.loopMode = LoopMode::Gate;
                settings.bpmSync = true;
                settings.keytrack = false;
                settings.originalBpm = 120.f;
                settings.start = 0.f;
                settings.end = 1.f;
                engine.setSourceSettings (settings, hostBpm);
                engine.setPlaybackContext (AviatorKeyz::SoundType::Phrase, AviatorKeyz::Category::ARPS);

                engine.noteOn (60, 1.f, false, 0.f);
                return engine.getActiveVoiceReadIncrementForTest();
            };

            const float slowInc = readIncAtHostBpm (60.0);
            const float midInc = readIncAtHostBpm (120.0);
            const float fastInc = readIncAtHostBpm (240.0);

            expect (slowInc > 1.0e-6f && midInc > 1.0e-6f && fastInc > 1.0e-6f,
                    "Phrase voice must use sample playback with BPM sync enabled");

            const float slowVsMid = slowInc / midInc;
            const float midVsFast = midInc / fastInc;
            expect (slowVsMid > 0.45f && slowVsMid < 0.55f,
                    "Host 60 BPM read increment should be ~half of host 120 BPM");
            expect (midVsFast > 0.45f && midVsFast < 0.55f,
                    "Host 120 BPM read increment should be ~half of host 240 BPM");
        }

        beginTest ("Repeated same note: active voice count stays bounded");
        {
            SamplerEngine engine;
            TestSampleSnapshot snap;
            configureInstrumentEngine (engine, snap);

            int maxActive = 0;
            juce::AudioBuffer<float> buf (2, 256);
            for (int n = 0; n < 8; ++n)
            {
                engine.noteOn (60, 1.f, false, 0.f);
                engine.noteOff (60);
                buf.clear();
                engine.process (buf);
                maxActive = juce::jmax (maxActive, engine.getNumActiveVoices());
            }
            expect (maxActive <= 2, "Repeated note must not accumulate unbounded voices");
            engine.allSoundOff();
        }

        beginTest ("Instrument chord: polyphonic voices release independently");
        {
            SamplerEngine engine;
            TestSampleSnapshot snap;
            configureInstrumentEngine (engine, snap);

            engine.noteOn (60, 1.f, false, 0.f);
            engine.noteOn (64, 1.f, false, 0.f);
            engine.noteOn (67, 1.f, false, 0.f);
            expectEquals (engine.getNumActiveVoices(), 3);

            engine.noteOff (64);
            juce::AudioBuffer<float> buf (2, 256);
            for (int i = 0; i < 64; ++i)
            {
                buf.clear();
                engine.process (buf);
            }
            expect (engine.getNumActiveVoices() >= 2);
            engine.allSoundOff();
        }

        beginTest ("Phrase retrigger: previous phrase is choked on new note");
        {
            SamplerEngine engine;
            TestSampleSnapshot snap;
            configurePhraseEngine (engine, snap);

            engine.noteOn (60, 1.f, false, 0.f);
            juce::AudioBuffer<float> buf (2, 256);
            buf.clear();
            engine.process (buf);
            const float firstPeak = bufferPeak (buf);

            engine.noteOff (60);
            engine.noteOn (62, 1.f, false, 0.f);
            buf.clear();
            engine.process (buf);
            expect (engine.getNumActiveVoices() <= 2);
            expect (bufferPeak (buf) > 0.f);
            expect (firstPeak > 0.f);
            engine.allSoundOff();
        }

        beginTest ("TriggerToEnd: note-off ignored until sample completes");
        {
            SamplerEngine engine;
            TestSampleSnapshot snap;
            juce::dsp::ProcessSpec spec { 44100.0, 256, 2 };
            engine.prepare (spec);
            snap.setMono (TestSamples::sine4096, TestSamples::kFrames, 60);
            engine.setSampleSnapshot (&snap.snapshot);
            engine.setEnvelopeTimesMs (0.f, 0.f, 1.f, 10.f);

            SourceSettings settings;
            settings.playbackMode = SamplePlaybackMode::OneShotOriginal;
            settings.loopMode = LoopMode::OneShot;
            settings.bpmSync = false;
            settings.keytrack = false;
            engine.setSourceSettings (settings, 120.0);
            engine.setPlaybackContext (AviatorKeyz::SoundType::OneShot, AviatorKeyz::Category::PHRASES);
            expect (engine.getNoteGatePolicy() == NoteGatePolicy::TriggerToEnd);

            engine.noteOn (60, 1.f, false, 0.f);
            juce::AudioBuffer<float> block (2, 256);
            block.clear();
            engine.process (block);
            const float peakBefore = bufferPeak (block);

            engine.noteOff (60);
            float peakAfter = 0.f;
            for (int i = 0; i < 4; ++i)
            {
                block.clear();
                engine.process (block);
                peakAfter = juce::jmax (peakAfter, bufferPeak (block));
            }

            expect (peakBefore > 0.1f);
            expect (peakAfter > 0.1f, "TriggerToEnd must continue after note-off");
            engine.allSoundOff();
        }

        beginTest ("PhraseOriginal: C2/C3/C4 duration approximately equal");
        {
            auto measureDuration = [&] (int midiNote) -> int
            {
                SamplerEngine engine;
                TestSampleSnapshot snap;
                configurePhraseEngine (engine, snap);
                engine.noteOn (midiNote, 1.f, false, 0.f);
                return renderUntilSilent (engine);
            };

            const int d36 = measureDuration (36);
            const int d48 = measureDuration (48);
            const int d60 = measureDuration (60);
            const float ratioMax = static_cast<float> (juce::jmax (d36, d48, d60))
                                   / static_cast<float> (juce::jmin (d36, d48, d60));
            expect (ratioMax < 1.15f,
                    "Phrase duration must not depend on MIDI note (ratio="
                    + juce::String (ratioMax, 3) + ")");
        }

        beginTest ("PhraseOriginal: C2/C3/C4 pitch approximately equal");
        {
            auto measurePitch = [&] (int midiNote) -> float
            {
                SamplerEngine engine;
                TestSampleSnapshot snap;
                configurePhraseEngine (engine, snap);
                engine.noteOn (midiNote, 1.f, false, 0.f);
                juce::AudioBuffer<float> buf (2, 512);
                buf.clear();
                engine.process (buf);
                engine.allSoundOff();
                return estimateZeroCrossingHz (buf, buf.getNumSamples(), 44100.0);
            };

            const float p36 = measurePitch (36);
            const float p48 = measurePitch (48);
            const float p60 = measurePitch (60);
            expect (std::abs (p36 - p48) < p48 * 0.15f);
            expect (std::abs (p60 - p48) < p48 * 0.15f);
        }
    }
};

static PlaybackPolicyTests playbackPolicyTests;
static VoiceLifecycleTests voiceLifecycleTests;
