// =============================================================================
//  Arpeggiator tests — pattern building, host-synced timing, gate, hold latch,
//  slice mapping, and realtime safety of the per-block path.
// =============================================================================

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "DSP/Arpeggiator.h"
#include "DSP/ArpPattern.h"

namespace
{
constexpr double kSr = 48000.0;
constexpr int    kBlock = 256;

struct Rig
{
    Arpeggiator arp;
    Arpeggiator::Event events[Arpeggiator::kMaxEvents];
    std::vector<std::pair<int, Arpeggiator::Event>> log; // (absolute sample, event)
    int absolute = 0;

    explicit Rig (Arp::Settings s)
    {
        arp.prepare (kSr);
        arp.setSettings (s);
    }

    /** Runs numBlocks blocks at bpm with the given MIDI in the first block. */
    void run (juce::MidiBuffer first, int numBlocks, double bpm = 120.0,
              bool hostPlaying = false, double hostBeatStart = -1.0)
    {
        for (int b = 0; b < numBlocks; ++b)
        {
            juce::MidiBuffer in;
            if (b == 0) in = first;
            const double beat = hostBeatStart >= 0.0
                                    ? hostBeatStart + absolute * bpm / 60.0 / kSr
                                    : -1.0;
            const int count = arp.process (in, kBlock, bpm, beat, hostPlaying, events);
            for (int i = 0; i < count; ++i)
                log.push_back ({ absolute + events[i].samplePos, events[i] });
            absolute += kBlock;
        }
    }

    std::vector<int> noteOnTimes() const
    {
        std::vector<int> out;
        for (auto& e : log) if (e.second.noteOn) out.push_back (e.first);
        return out;
    }

    std::vector<int> noteOnNotes() const
    {
        std::vector<int> out;
        for (auto& e : log) if (e.second.noteOn) out.push_back (e.second.note);
        return out;
    }
};

juce::MidiBuffer chord (std::initializer_list<int> notes, int pos = 0)
{
    juce::MidiBuffer m;
    for (int n : notes)
        m.addEvent (juce::MidiMessage::noteOn (1, n, (juce::uint8) 100), pos);
    return m;
}
} // namespace

class ArpeggiatorTests : public juce::UnitTest
{
public:
    ArpeggiatorTests() : juce::UnitTest ("Arpeggiator", "AviatorKeyz") {}

    void runTest() override
    {
        beginTest ("Pattern: up / down / up-down / as-played with octaves");
        {
            Arp::HeldNote held[3] = { { 64, 1.f, 1 }, { 60, 1.f, 0 }, { 67, 1.f, 2 } };
            Arp::Step out[Arp::kMaxSequence];

            int n = Arp::buildSequence (held, 3, Arp::Mode::up, 1, out, Arp::kMaxSequence);
            expectEquals (n, 3);
            expectEquals (out[0].note, 60); expectEquals (out[1].note, 64); expectEquals (out[2].note, 67);

            n = Arp::buildSequence (held, 3, Arp::Mode::down, 2, out, Arp::kMaxSequence);
            expectEquals (n, 6);
            expectEquals (out[0].note, 79); expectEquals (out[5].note, 60);

            n = Arp::buildSequence (held, 3, Arp::Mode::upDown, 1, out, Arp::kMaxSequence);
            expectEquals (n, 4); // 60 64 67 64
            expectEquals (out[3].note, 64);

            n = Arp::buildSequence (held, 3, Arp::Mode::upDown, 2, out, Arp::kMaxSequence);
            expectEquals (n, 10); // 6 up + 4 back (no repeated turnarounds)

            held[0].order = 0; held[1].order = 2; held[2].order = 1; // 64, 67, 60 as played
            n = Arp::buildSequence (held, 3, Arp::Mode::asPlayed, 1, out, Arp::kMaxSequence);
            expectEquals (out[0].note, 64); expectEquals (out[1].note, 67); expectEquals (out[2].note, 60);
        }

        beginTest ("Step length follows rate and feel");
        {
            expectWithinAbsoluteError (Arp::stepLengthBeats (Arp::Rate::sixteenth, Arp::Feel::straight), 0.25, 1e-9);
            expectWithinAbsoluteError (Arp::stepLengthBeats (Arp::Rate::eighth, Arp::Feel::triplet), 1.0 / 3.0, 1e-9);
            expectWithinAbsoluteError (Arp::stepLengthBeats (Arp::Rate::quarter, Arp::Feel::dotted), 1.5, 1e-9);
        }

        beginTest ("Slice mapping: C1 = slice 0, wraps every 16 notes, +4 per octave");
        {
            expectEquals (Arp::sliceIndexForNote (36), 0);
            expectEquals (Arp::sliceIndexForNote (37), 1);
            expectEquals (Arp::sliceIndexForNote (52), 0);
            expectEquals (Arp::sliceIndexForNote (35), 15);
            expectEquals (Arp::sliceIndexForNote (36, 1), 4);
        }

        beginTest ("Free-run: 1/16 at 120 BPM emits a note every 6000 samples, first one immediately");
        {
            Arp::Settings s; s.on = true; s.rate = Arp::Rate::sixteenth; s.gate = 0.5f;
            Rig rig (s);
            rig.run (chord ({ 60, 64, 67 }), 200); // ~1.07 s
            const auto times = rig.noteOnTimes();
            expect (times.size() >= 8, "expected at least 8 steps");
            expectEquals (times[0], 0);
            for (size_t i = 1; i < juce::jmin<size_t> (8, times.size()); ++i)
                expectWithinAbsoluteError ((double) (times[i] - times[i - 1]), 6000.0, 1.0);

            const auto notes = rig.noteOnNotes();
            expectEquals (notes[0], 60); expectEquals (notes[1], 64); expectEquals (notes[2], 67); expectEquals (notes[3], 60);
        }

        beginTest ("Gate: note-off lands gate * step after each note-on");
        {
            Arp::Settings s; s.on = true; s.rate = Arp::Rate::eighth; s.gate = 0.25f; // step 12000 → gate 3000
            Rig rig (s);
            rig.run (chord ({ 60 }), 260); // ~1.4 s → 5 steps
            int ons = 0, offsMatched = 0;
            for (size_t i = 0; i < rig.log.size(); ++i)
            {
                if (! rig.log[i].second.noteOn) continue;
                ++ons;
                for (size_t j = i + 1; j < rig.log.size(); ++j)
                    if (! rig.log[j].second.noteOn && rig.log[j].second.note == 60)
                    {
                        expectWithinAbsoluteError ((double) (rig.log[j].first - rig.log[i].first), 3000.0, 1.0);
                        ++offsMatched;
                        break;
                    }
            }
            expect (ons >= 4 && offsMatched >= 4, "every note-on gets a matching note-off");
        }

        beginTest ("Host sync: steps align to the PPQ grid, not the key press");
        {
            Arp::Settings s; s.on = true; s.rate = Arp::Rate::quarter;
            Rig rig (s);
            // Transport at beat 0.5 when the key is pressed at sample 100 → first
            // step must land on beat 1.0 = 0.5 beat later = 12000 samples after
            // block start at 120 BPM.
            rig.run (chord ({ 60 }, 100), 100, 120.0, true, 0.5);
            const auto times = rig.noteOnTimes();
            expect (! times.empty());
            expectWithinAbsoluteError ((double) times[0], 12000.0, 2.0);
            if (times.size() > 1)
                expectWithinAbsoluteError ((double) (times[1] - times[0]), 24000.0, 2.0);
        }

        beginTest ("Release stops the pattern; HOLD latches it until a new chord");
        {
            Arp::Settings s; s.on = true; s.rate = Arp::Rate::sixteenth;
            {
                Rig rig (s);
                juce::MidiBuffer m = chord ({ 60 });
                m.addEvent (juce::MidiMessage::noteOff (1, 60), 10);
                rig.run (m, 60);
                expectEquals ((int) rig.noteOnTimes().size(), 1); // only the immediate step
            }
            {
                s.hold = true;
                Rig rig (s);
                juce::MidiBuffer m = chord ({ 60, 64 });
                m.addEvent (juce::MidiMessage::noteOff (1, 60), 10);
                m.addEvent (juce::MidiMessage::noteOff (1, 64), 10);
                rig.run (m, 200); // ~1.07 s → 8+ steps at 1/16
                expect (rig.noteOnTimes().size() > 4, "latched chord keeps arpeggiating");

                // a new key replaces the latched chord
                juce::MidiBuffer next = chord ({ 72 });
                const size_t before = rig.log.size();
                rig.run (next, 40);
                bool sawOld = false, sawNew = false;
                for (size_t i = before; i < rig.log.size(); ++i)
                {
                    if (! rig.log[i].second.noteOn) continue;
                    if (rig.log[i].second.note == 72) sawNew = true;
                    if (rig.log[i].first > rig.log[before - 1].first + 6000 && rig.log[i].second.note != 72) sawOld = true;
                }
                expect (sawNew && ! sawOld, "latch replaced by the new chord");
            }
        }

        beginTest ("flushAllNotesOff closes every sounding note");
        {
            Arp::Settings s; s.on = true; s.gate = 1.0f;
            Rig rig (s);
            rig.run (chord ({ 60, 64 }), 4);
            Arpeggiator::Event ev[Arpeggiator::kMaxEvents];
            const int n = rig.arp.flushAllNotesOff (ev);
            expect (n >= 1);
            for (int i = 0; i < n; ++i)
                expect (! ev[i].noteOn);
            expect (! rig.arp.hasHeldNotes());
        }

        beginTest ("Events are sorted with offs before ons at equal positions");
        {
            Arp::Settings s; s.on = true; s.rate = Arp::Rate::thirtySecond; s.gate = 1.0f;
            Rig rig (s);
            rig.run (chord ({ 60 }), 40);
            for (size_t i = 1; i < rig.log.size(); ++i)
            {
                expect (rig.log[i].first >= rig.log[i - 1].first, "monotonic");
                if (rig.log[i].first == rig.log[i - 1].first && rig.log[i - 1].second.noteOn)
                    expect (rig.log[i].second.noteOn, "an off never follows an on at the same sample");
            }
        }
    }
};

static ArpeggiatorTests arpeggiatorTests;
