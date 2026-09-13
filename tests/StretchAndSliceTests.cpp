// =============================================================================
//  StretchAndSliceTests — STRETCH playback mode (Signalsmith), SLICE keyboard
//  mode, the MIDI flip trigger, and the envelope master switch.
// =============================================================================

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <cmath>

#include "PluginProcessor.h"
#include "DSP/StretchPlayer.h"
#include "State/StateSchema.h"

namespace
{
constexpr double kSr = 48000.0;
constexpr int kBlock = 480;

void setParam (AviatorKeyzProcessor& p, const char* id, float value)
{
    auto* param = p.getAPVTS().getParameter (id);
    jassert (param != nullptr);
    param->setValueNotifyingHost (param->convertTo0to1 (value));
}

struct Proc
{
    AviatorKeyzProcessor p;
    Proc()
    {
        p.getPresetManager().loadPreset (AviatorKeyz::Category::LEADS, "Init");
        setParam (p, AviatorKeyz::ParamID::FX_REVERB_ON, 0.f);
        setParam (p, AviatorKeyz::ParamID::REVERB_AMOUNT, 0.f);
        setParam (p, AviatorKeyz::ParamID::OUTPUT_LIMITER, 0.f);
        setParam (p, AviatorKeyz::ParamID::ENV_ATTACK, 0.f);
        setParam (p, AviatorKeyz::ParamID::ENV_RELEASE, 10.f);
        p.prepareToPlay (kSr, kBlock);
    }

    /** Holds `note` for the whole run; returns per-block RMS. */
    std::vector<float> run (int numBlocks, int note = 60, int releaseAtBlock = -1)
    {
        std::vector<float> rms;
        juce::AudioBuffer<float> buffer (2, kBlock);
        for (int b = 0; b < numBlocks; ++b)
        {
            juce::MidiBuffer midi;
            if (b == 0) midi.addEvent (juce::MidiMessage::noteOn (1, note, (juce::uint8) 100), 0);
            if (b == releaseAtBlock) midi.addEvent (juce::MidiMessage::noteOff (1, note), 0);
            buffer.clear();
            p.processBlock (buffer, midi);
            rms.push_back (buffer.getRMSLevel (0, 0, kBlock));
        }
        return rms;
    }
};

/** Number of leading blocks with audio before the tail goes quiet for good. */
int audibleBlocks (const std::vector<float>& rms, float floor = 2.0e-4f)
{
    int last = -1;
    for (int i = 0; i < (int) rms.size(); ++i)
        if (rms[(size_t) i] > floor) last = i;
    return last + 1;
}
} // namespace

class StretchAndSliceTests : public juce::UnitTest
{
public:
    StretchAndSliceTests() : juce::UnitTest ("Stretch_Slice", "AviatorKeyz") {}

    void runTest() override
    {
        beginTest ("StretchPlayer: renders finite audio and reports a moving playhead");
        {
            std::vector<float> tone ((size_t) 48000);
            for (size_t i = 0; i < tone.size(); ++i)
                tone[i] = 0.5f * std::sin (juce::MathConstants<float>::twoPi * 220.f * (float) i / 48000.f);
            SampleLibrary::AudioRegion region;
            region.data = tone.data(); region.numFrames = (int) tone.size(); region.fileSampleRate = 48000.0; region.rootNote = 60;

            StretchPlayer sp;
            sp.prepare (kSr, kBlock);
            sp.setRegion (&region);
            StretchPlayer::Params prm; prm.loopMode = LoopMode::Loop; prm.speed = 1.f;
            sp.setParams (prm);
            sp.noteOn (60, 1.f);
            juce::AudioBuffer<float> buf (2, kBlock);
            float energy = 0.f;
            for (int b = 0; b < 50; ++b)
            {
                buf.clear();
                sp.render (buf);
                for (int i = 0; i < kBlock; ++i)
                    expect (std::isfinite (buf.getSample (0, i)) && std::abs (buf.getSample (0, i)) < 4.f);
                energy += buf.getRMSLevel (0, 0, kBlock);
            }
            expect (energy > 0.5f, "stretch voice is audible: " + juce::String (energy));
            expect (sp.getPlayheadNorm() > 0.3f, "playhead advanced: " + juce::String (sp.getPlayheadNorm()));
        }

        beginTest ("STRETCH mode: speed changes duration, not pitch (2x plays ~half as long as 0.5x... x4)");
        {
            auto durationAt = [] (float speed)
            {
                Proc t;
                setParam (t.p, AviatorKeyz::ParamID::SRC_PLAYBACK_MODE, 3.f); // Time Stretch
                setParam (t.p, AviatorKeyz::ParamID::SRC_LOOP_MODE, 0.f);     // one shot -> ends
                setParam (t.p, AviatorKeyz::ParamID::SRC_BPM_SYNC, 0.f);
                setParam (t.p, AviatorKeyz::ParamID::SRC_START, 0.f);
                setParam (t.p, AviatorKeyz::ParamID::SRC_END, 0.15f);         // ~1 s of the 6.7 s sample
                setParam (t.p, AviatorKeyz::ParamID::SRC_SPEED, speed);
                const auto rms = t.run (600);                                 // 6 s
                return audibleBlocks (rms);
            };
            const int fast = durationAt (2.f);
            const int slow = durationAt (0.5f);
            expect (fast > 10, "fast run audible: " + juce::String (fast));
            expect (slow > fast * 2, "0.5x lasts far longer than 2x: slow=" + juce::String (slow) + " fast=" + juce::String (fast));
            expect (slow < fast * 8, "but not absurdly so");
        }

        beginTest ("STRETCH mode: keytrack transposes without changing duration");
        {
            auto durationAt = [] (int note)
            {
                Proc t;
                setParam (t.p, AviatorKeyz::ParamID::SRC_PLAYBACK_MODE, 3.f);
                setParam (t.p, AviatorKeyz::ParamID::SRC_LOOP_MODE, 0.f);
                setParam (t.p, AviatorKeyz::ParamID::SRC_BPM_SYNC, 0.f);
                setParam (t.p, AviatorKeyz::ParamID::SRC_KEYTRACK, 1.f);
                setParam (t.p, AviatorKeyz::ParamID::SRC_END, 0.12f);
                return audibleBlocks (t.run (400, note));
            };
            const int root = durationAt (60);
            const int octaveUp = durationAt (72);
            expect (root > 10);
            expect (std::abs (root - octaveUp) <= juce::jmax (6, root / 8),
                    "same length an octave up: " + juce::String (root) + " vs " + juce::String (octaveUp));
        }

        beginTest ("SLICE mode: each key starts inside its own slice");
        {
            auto playheadFor = [] (int note)
            {
                Proc t;
                setParam (t.p, AviatorKeyz::ParamID::SRC_PLAYBACK_MODE, 4.f); // Slice
                setParam (t.p, AviatorKeyz::ParamID::SRC_LOOP_MODE, 1.f);     // loop the slice
                t.run (2, note);
                return t.p.getPlayheadNorm();
            };
            const float slice0 = playheadFor (36);   // C1
            const float slice8 = playheadFor (44);   // G#1 -> slice 8
            expect (slice0 < 0.08f, "slice 0 near the start: " + juce::String (slice0));
            expectWithinAbsoluteError (slice8, 0.5f, 0.08f);
        }

        beginTest ("SLICE divisions: 4 pads span the window in quarters, and keys wrap");
        {
            auto playheadFor = [] (int note, float divChoice)
            {
                Proc t;
                setParam (t.p, AviatorKeyz::ParamID::SRC_PLAYBACK_MODE, 4.f); // Slice
                setParam (t.p, AviatorKeyz::ParamID::SRC_LOOP_MODE, 1.f);     // loop the slice
                setParam (t.p, AviatorKeyz::ParamID::SLICE_DIV, divChoice);
                t.run (2, note);
                return t.p.getPlayheadNorm();
            };
            // Choice 1 = 4 slices: C1/C#1/D1/D#1 land at 0 / 0.25 / 0.5 / 0.75.
            expect (playheadFor (36, 1.f) < 0.1f, "pad 0 near the start");
            expectWithinAbsoluteError (playheadFor (37, 1.f), 0.25f, 0.1f);
            expectWithinAbsoluteError (playheadFor (38, 1.f), 0.5f, 0.1f);
            expectWithinAbsoluteError (playheadFor (39, 1.f), 0.75f, 0.1f);
            // E1 is the fifth key: with 4 pads it wraps back to pad 0.
            expect (playheadFor (40, 1.f) < 0.1f, "fifth key wraps to pad 0");
        }

        beginTest ("SLICE cut offset moves where a pad starts");
        {
            auto playheadWithCut = [] (int note, float cut0)
            {
                Proc t;
                setParam (t.p, AviatorKeyz::ParamID::SRC_PLAYBACK_MODE, 4.f);
                setParam (t.p, AviatorKeyz::ParamID::SRC_LOOP_MODE, 1.f);
                setParam (t.p, AviatorKeyz::ParamID::SLICE_DIV, 1.f); // 4 slices
                setParam (t.p, AviatorKeyz::ParamID::sliceCutParamId (0).toRawUTF8(), cut0);
                t.run (2, note);
                return t.p.getPlayheadNorm();
            };
            const float unmoved = playheadWithCut (37, 0.f);   // pad 1 at 0.25
            const float pushed  = playheadWithCut (37, 1.f);   // cut 1 pushed +half a slice
            expect (pushed > unmoved + 0.05f,
                    "pad 1 starts later once its cut moves: " + juce::String (unmoved, 3)
                        + " -> " + juce::String (pushed, 3));
        }

        beginTest ("SLICE random at full scale scatters pads instead of pinning one position");
        {
            auto playhead = [] (int note, float random)
            {
                Proc t;
                setParam (t.p, AviatorKeyz::ParamID::SRC_PLAYBACK_MODE, 4.f);
                setParam (t.p, AviatorKeyz::ParamID::SRC_LOOP_MODE, 1.f);
                setParam (t.p, AviatorKeyz::ParamID::SLICE_DIV, 4.f); // 16 slices
                setParam (t.p, AviatorKeyz::ParamID::SLICE_RANDOM, random);
                t.run (2, note);
                return t.p.getPlayheadNorm();
            };
            // Random off: the same key is always the same slice.
            for (int i = 0; i < 4; ++i)
                expectWithinAbsoluteError (playhead (44, 0.f), 0.5f, 0.08f);

            // Random on: repeated triggers of one key must not all land on its own slice.
            int elsewhere = 0;
            for (int i = 0; i < 24; ++i)
                if (std::abs (playhead (44, 1.f) - 0.5f) > 0.08f)
                    ++elsewhere;
            expect (elsewhere > 0, "full random moved the pad at least once");
        }

        beginTest ("Flip trigger note throws the lever and is not played");
        {
            Proc t;
            setParam (t.p, AviatorKeyz::ParamID::FLIP_TRIGGER_ON, 1.f);
            setParam (t.p, AviatorKeyz::ParamID::FLIP_TRIGGER_NOTE, 24.f);
            setParam (t.p, AviatorKeyz::ParamID::FLIP_MODE, 0.f); // latch
            juce::AudioBuffer<float> buffer (2, kBlock);
            juce::MidiBuffer midi;
            midi.addEvent (juce::MidiMessage::noteOn (1, 24, (juce::uint8) 100), 0);
            buffer.clear();
            t.p.processBlock (buffer, midi);
            expect (t.p.getAPVTS().getRawParameterValue (AviatorKeyz::ParamID::REVERSE)->load() > 0.5f, "latched to REV");
            expectEquals (t.p.getActiveVoiceCount(), 0);
            expect (buffer.getRMSLevel (0, 0, kBlock) < 1.0e-5f, "trigger note makes no sound");

            juce::MidiBuffer again;
            again.addEvent (juce::MidiMessage::noteOn (1, 24, (juce::uint8) 100), 0);
            buffer.clear();
            t.p.processBlock (buffer, again);
            expect (t.p.getAPVTS().getRawParameterValue (AviatorKeyz::ParamID::REVERSE)->load() < 0.5f, "second press toggles back");

            setParam (t.p, AviatorKeyz::ParamID::FLIP_MODE, 1.f); // momentary
            juce::MidiBuffer down; down.addEvent (juce::MidiMessage::noteOn (1, 24, (juce::uint8) 100), 0);
            buffer.clear(); t.p.processBlock (buffer, down);
            expect (t.p.getAPVTS().getRawParameterValue (AviatorKeyz::ParamID::REVERSE)->load() > 0.5f);
            juce::MidiBuffer up; up.addEvent (juce::MidiMessage::noteOff (1, 24), 0);
            buffer.clear(); t.p.processBlock (buffer, up);
            expect (t.p.getAPVTS().getRawParameterValue (AviatorKeyz::ParamID::REVERSE)->load() < 0.5f, "release returns to FWD");
        }

        beginTest ("Envelope switch OFF gives a flat envelope regardless of A/D/S/R");
        {
            Proc shaped;
            setParam (shaped.p, AviatorKeyz::ParamID::ENV_AMP_SUSTAIN, 0.2f);
            setParam (shaped.p, AviatorKeyz::ParamID::SRC_LOOP_MODE, 1.f);
            const auto a = shaped.run (20);

            Proc flat;
            setParam (flat.p, AviatorKeyz::ParamID::ENV_AMP_SUSTAIN, 0.2f);
            setParam (flat.p, AviatorKeyz::ParamID::SRC_LOOP_MODE, 1.f);
            setParam (flat.p, AviatorKeyz::ParamID::ENV_ENABLED, 0.f);
            const auto b = flat.run (20);

            float ra = 0.f, rb = 0.f;
            for (int i = 5; i < 20; ++i) { ra += a[(size_t) i]; rb += b[(size_t) i]; }
            expect (ra > 1.0e-3f);
            expect (rb > ra * 3.f, "flat envelope is louder than sustain 20%: " + juce::String (rb) + " vs " + juce::String (ra));
        }

        beginTest ("Every factory category loads with keytrack on; only PHRASES lands in STRETCH");
        {
            Proc t;
            auto& pm = t.p.getPresetManager();
            const auto& apvts = t.p.getAPVTS();
            int stretchPhraseLoads = 0;

            for (const auto& cat : { AviatorKeyz::Category::VOCALS, AviatorKeyz::Category::ARPS,
                                     AviatorKeyz::Category::PHRASES, AviatorKeyz::Category::PADS,
                                     AviatorKeyz::Category::ENSEMBLES, AviatorKeyz::Category::LEADS,
                                     AviatorKeyz::Category::BELLS })
            {
                const auto names = pm.getPresetsForCategory (cat);
                expect (! names.isEmpty(), juce::String (cat) + " has factory presets");
                const bool phraseTab = AviatorKeyz::isPhraseCategory (cat);
                for (const auto& name : names)
                {
                    expect (pm.loadPreset (cat, name), "loads " + name);
                    const int mode = (int) apvts.getRawParameterValue (AviatorKeyz::ParamID::SRC_PLAYBACK_MODE)->load();
                    const bool keytrack = apvts.getRawParameterValue (AviatorKeyz::ParamID::SRC_KEYTRACK)->load() > 0.5f;
                    expect (keytrack, name + " should track the keyboard");
                    expect (mode != (int) SamplePlaybackMode::PhraseOriginal, name + " must not default to varispeed PHRASE");
                    if (phraseTab)
                    {
                        expect (mode == (int) SamplePlaybackMode::PhraseTimeStretch
                                    || mode == (int) SamplePlaybackMode::OneShotOriginal,
                                name + " in PHRASES should stretch (or play through, for a one-shot)");
                        if (mode == (int) SamplePlaybackMode::PhraseTimeStretch)
                            ++stretchPhraseLoads;
                    }
                    else
                    {
                        expect (mode == (int) SamplePlaybackMode::ChromaticResample,
                                name + " outside PHRASES should be chromatic, was mode " + juce::String (mode));
                    }
                }
            }
            expect (stretchPhraseLoads > 10, "PHRASES presets route to STRETCH: " + juce::String (stretchPhraseLoads));
        }

        beginTest ("Dropped cargo follows the tab: long file STRETCHes in PHRASES, is chromatic elsewhere");
        {
            auto writeSine = [] (const juce::File& f, double seconds) -> bool
            {
                f.deleteFile();
                juce::WavAudioFormat wav;
                std::unique_ptr<juce::FileOutputStream> os (f.createOutputStream());
                if (os == nullptr) return false;
                std::unique_ptr<juce::AudioFormatWriter> w (wav.createWriterFor (os.get(), kSr, 1, 16, {}, 0));
                if (w == nullptr) return false;
                os.release(); // writer owns the stream now
                const int n = (int) (seconds * kSr);
                juce::AudioBuffer<float> b (1, n);
                for (int i = 0; i < n; ++i)
                    b.setSample (0, i, 0.5f * (float) std::sin (juce::MathConstants<double>::twoPi * 220.0 * i / kSr));
                return w->writeFromAudioSampleBuffer (b, 0, n);
            };

            const auto dir = juce::File::getSpecialLocation (juce::File::tempDirectory);
            const auto longFile = dir.getChildFile ("aviatorkeyz_test_cargo_long.wav");
            const auto shortFile = dir.getChildFile ("aviatorkeyz_test_cargo_short.wav");
            expect (writeSine (longFile, 3.0));
            expect (writeSine (shortFile, 0.5));

            Proc t;
            auto& pm = t.p.getPresetManager();
            const auto& apvts = t.p.getAPVTS();
            juce::String err;
            auto playbackMode = [&] {
                return (int) apvts.getRawParameterValue (AviatorKeyz::ParamID::SRC_PLAYBACK_MODE)->load();
            };
            auto openTab = [&] (const juce::String& cat)
            {
                const auto names = pm.getPresetsForCategory (cat);
                expect (! names.isEmpty(), cat + " has presets");
                expect (pm.loadPreset (cat, names[0]), "opens " + cat);
            };

            // On PHRASES a long drop keeps its pitch and stretches to the host.
            openTab (AviatorKeyz::Category::PHRASES);
            expect (t.p.loadUserSample (longFile, err), err);
            expectEquals (playbackMode(), (int) SamplePlaybackMode::PhraseTimeStretch);
            expect (apvts.getRawParameterValue (AviatorKeyz::ParamID::SRC_KEYTRACK)->load() > 0.5f, "long cargo keytracks");
            const auto up = t.run (12, 72);
            expect (audibleBlocks (up) > 4, "stretch voice plays a transposed note: " + juce::String (audibleBlocks (up)));

            // The same file on an instrument tab is repitched by the keyboard.
            openTab (AviatorKeyz::Category::LEADS);
            expect (t.p.loadUserSample (longFile, err), err);
            expectEquals (playbackMode(), (int) SamplePlaybackMode::ChromaticResample);

            expect (t.p.loadUserSample (shortFile, err), err);
            expect (apvts.getRawParameterValue (AviatorKeyz::ParamID::SRC_KEYTRACK)->load() > 0.5f, "short cargo keytracks");
            expect (playbackMode() != (int) SamplePlaybackMode::PhraseOriginal);

            longFile.deleteFile();
            shortFile.deleteFile();
        }
    }
};

static StretchAndSliceTests stretchAndSliceTests;
