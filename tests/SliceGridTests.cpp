// =============================================================================
//  SliceGridTests — slice divisions (3/4/6/8/16), custom cut offsets, and the
//  key→pad mapping shared by SLICE mode, the arp SLICES target and flip SLICE.
// =============================================================================

#include <juce_core/juce_core.h>
#include "DSP/Performance/SliceGrid.h"
#include "State/StateSchema.h"

class SliceGridTests : public juce::UnitTest
{
public:
    SliceGridTests() : juce::UnitTest ("SliceGrid", "AviatorKeyz") {}

    void runTest() override
    {
        beginTest ("Division choices map to 3 / 4 / 6 / 8 / 16 and round-trip");
        {
            expectEquals (SliceGrid::divisionsForChoice (0), 3);
            expectEquals (SliceGrid::divisionsForChoice (1), 4);
            expectEquals (SliceGrid::divisionsForChoice (2), 6);
            expectEquals (SliceGrid::divisionsForChoice (3), 8);
            expectEquals (SliceGrid::divisionsForChoice (4), 16);
            // Out-of-range choices clamp rather than read past the table.
            expectEquals (SliceGrid::divisionsForChoice (-1), 3);
            expectEquals (SliceGrid::divisionsForChoice (99), 16);
            for (int c = 0; c < SliceGrid::kNumDivisionChoices; ++c)
                expectEquals (SliceGrid::choiceForDivisions (SliceGrid::divisionsForChoice (c)), c);
        }

        beginTest ("Equal division: pads tile the window with no gaps or overlaps");
        {
            for (int c = 0; c < SliceGrid::kNumDivisionChoices; ++c)
            {
                SliceSettings s;
                s.divisions = SliceGrid::divisionsForChoice (c);

                int prevEnd = 1000;
                for (int i = 0; i < s.divisions; ++i)
                {
                    int a = 0, b = 0;
                    SliceGrid::sliceBounds (s, 1000, 9000, i, a, b);
                    expectEquals (a, prevEnd, "pad " + juce::String (i) + " starts where the last ended");
                    expect (b > a, "pad " + juce::String (i) + " is non-empty");
                    prevEnd = b;
                }
                expectEquals (prevEnd, 9000, "last pad ends at the window end (div "
                                              + juce::String (s.divisions) + ")");
            }
        }

        beginTest ("Pad width matches the division: 4 slices are 4x wider than 16");
        {
            SliceSettings four;  four.divisions = 4;
            SliceSettings sixteen; sixteen.divisions = 16;
            int a4 = 0, b4 = 0, a16 = 0, b16 = 0;
            SliceGrid::sliceBounds (four, 0, 16000, 0, a4, b4);
            SliceGrid::sliceBounds (sixteen, 0, 16000, 0, a16, b16);
            expectEquals (b4 - a4, 4000);
            expectEquals (b16 - a16, 1000);
        }

        beginTest ("Cut offsets move the boundary and stay inside +/- half a slice");
        {
            SliceSettings s;
            s.divisions = 4;                       // 4000-frame slices over [0, 16000]
            s.cutOffsets[0] = 0.5f;                // cut 1: +25% of a slice
            int a = 0, b = 0;
            SliceGrid::sliceBounds (s, 0, 16000, 0, a, b);
            expectEquals (a, 0);
            expectEquals (b, 5000, "first pad grew by half of half a slice");

            SliceGrid::sliceBounds (s, 0, 16000, 1, a, b);
            expectEquals (a, 5000, "second pad starts at the moved cut");
            expectEquals (b, 8000, "second cut is untouched");

            // A full-scale offset is exactly half a slice — cuts can meet but never cross.
            s.cutOffsets[0] = -1.f;
            SliceGrid::sliceBounds (s, 0, 16000, 0, a, b);
            expectEquals (b, 2000);
            s.cutOffsets[0] = 5.f; // out of range: clamped to +1
            SliceGrid::sliceBounds (s, 0, 16000, 0, a, b);
            expectEquals (b, 6000);
        }

        beginTest ("Extreme offsets never produce an empty or inverted pad");
        {
            SliceSettings s;
            s.divisions = 16;
            for (auto& o : s.cutOffsets)
                o = 1.f; // every cut pushed hard right
            for (int i = 0; i < s.divisions; ++i)
            {
                int a = 0, b = 0;
                SliceGrid::sliceBounds (s, 0, 320, i, a, b);
                expect (b >= a + 2, "pad " + juce::String (i) + " has at least 2 frames");
                expect (a >= 0 && b <= 320, "pad " + juce::String (i) + " stays inside the window");
            }
        }

        beginTest ("Degenerate windows are safe");
        {
            SliceSettings s;
            s.divisions = 16;
            int a = 0, b = 0;
            SliceGrid::sliceBounds (s, 100, 101, 7, a, b);
            expect (b > a, "1-frame window still yields an ordered range");
            s.divisions = 0; // clamped to at least 1 internally
            SliceGrid::sliceBounds (s, 0, 8000, 0, a, b);
            expect (b > a);
        }

        beginTest ("C1 is pad 0 and keys wrap at the division");
        {
            expectEquals (SliceGrid::indexForNote (36, 16), 0);
            expectEquals (SliceGrid::indexForNote (37, 16), 1);
            expectEquals (SliceGrid::indexForNote (52, 16), 0);   // an octave of pads later
            expectEquals (SliceGrid::indexForNote (35, 16), 15);  // below C1 wraps to the last pad
            // With 4 pads, every fourth key returns to pad 0.
            expectEquals (SliceGrid::indexForNote (36, 4), 0);
            expectEquals (SliceGrid::indexForNote (39, 4), 3);
            expectEquals (SliceGrid::indexForNote (40, 4), 0);
            expectEquals (SliceGrid::indexForNote (35, 4), 3);
        }

        beginTest ("Slice base note and max match the schema the rest of the engine uses");
        {
            expectEquals (SliceGrid::kSliceBaseNote, 36);
            expectEquals (SliceGrid::kMaxSlices, AviatorKeyz::ParamID::ARP_NUM_SLICES);
            expectEquals ((int) SliceSettings().cutOffsets.size(),
                          AviatorKeyz::ParamID::SLICE_CUT_COUNT);
            expect (AviatorKeyz::ParamID::SLICE_CUT_COUNT >= SliceGrid::kMaxSlices - 1,
                    "one movable cut per interior boundary at max division");
        }
    }
};

static SliceGridTests sliceGridTests;
