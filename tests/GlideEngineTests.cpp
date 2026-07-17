#include <juce_core/juce_core.h>
#include "DSP/GlideEngine.h"

class GlideEngineTests : public juce::UnitTest
{
public:
    GlideEngineTests() : juce::UnitTest ("GlideEngine", "AviatorKeyz") {}

    void runTest() override
    {
        beginTest ("snapToPitch sets current pitch without gliding");
        {
            GlideEngine glide;
            glide.setSampleRate (44100.0);
            glide.snapToPitch (55.f);
            expectEquals (glide.getCurrentPitchSemitones(), 55.f);
            expect (! glide.isGliding());
        }

        beginTest ("noteOn with 0ms snaps instantly");
        {
            GlideEngine glide;
            glide.setSampleRate (44100.0);
            glide.snapToPitch (60.f);
            glide.noteOn (72, 0.f);
            expectEquals (glide.getCurrentPitchSemitones(), 72.f);
            expect (! glide.isGliding());
        }

        beginTest ("100ms ramp reaches target without overshoot");
        {
            GlideEngine glide;
            glide.setSampleRate (44100.0);
            glide.snapToPitch (60.f);
            glide.noteOn (72, 100.f);

            expect (glide.isGliding());

            const int samplesForGlide = static_cast<int> (std::round (0.1 * 44100.0));
            for (int i = 0; i < samplesForGlide + 4; ++i)
                glide.tick();

            expectEquals (glide.getCurrentPitchSemitones(), 72.f);
            expect (! glide.isGliding());
        }

        beginTest ("tick does not overshoot target pitch");
        {
            GlideEngine glide;
            glide.setSampleRate (44100.0);
            glide.snapToPitch (60.f);
            glide.noteOn (72, 50.f);

            for (int i = 0; i < 10000; ++i)
                glide.tick();

            expect (glide.getCurrentPitchSemitones() <= 72.f + 0.001f);
            expect (glide.getCurrentPitchSemitones() >= 72.f - 0.001f);
        }
    }
};

static GlideEngineTests glideEngineTests;
