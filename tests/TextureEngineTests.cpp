#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

#include "DSP/TextureEngine.h"

class TextureEngineTests : public juce::UnitTest
{
public:
    TextureEngineTests() : juce::UnitTest ("TextureEngine", "AviatorKeyz") {}

    void runTest() override
    {
        beginTest ("wetMix=0 leaves buffer unchanged (no white-noise leak)");
        {
            TextureEngine engine;
            juce::dsp::ProcessSpec spec { 48000.0, 512, 2 };
            engine.prepare (spec);

            juce::AudioBuffer<float> buffer (2, 512);
            buffer.clear();
            for (int i = 0; i < 512; ++i)
            {
                const float x = 0.25f * std::sin (2.f * juce::MathConstants<float>::pi * 440.f * (float) i / 48000.f);
                buffer.setSample (0, i, x);
                buffer.setSample (1, i, x);
            }

            const float inMag = buffer.getMagnitude (0, 512);
            engine.process (buffer, true, 0.f, false,
                            0.55f, 0.4f, 0, 0.7f, 0.55f, 0.f, 0.35f, 0.35f,
                            1.f, false, 0.8f, 0.f, 120.0);

            expect (std::abs (buffer.getMagnitude (0, 512) - inMag) < 1.0e-5f,
                    "wetMix=0 must pass input through unchanged");
        }

        beginTest ("wetMix=0 with AIR=1 does not inject rng noise");
        {
            TextureEngine engine;
            juce::dsp::ProcessSpec spec { 48000.0, 512, 2 };
            engine.prepare (spec);

            juce::AudioBuffer<float> buffer (2, 512);
            buffer.clear();

            engine.process (buffer, true, 0.f, false,
                            0.55f, 0.4f, 0, 0.7f, 0.55f, 0.f, 0.35f, 0.35f,
                            1.f, false, 0.8f, 0.f, 120.0);

            expect (buffer.getMagnitude (0, 512) < 1.0e-7f,
                    "wetMix=0 with AIR=1 must stay silent");
        }

        beginTest ("AIR noise is gated at source by wetMix (not just downstream mix)");
        {
            TextureEngine engine;
            juce::dsp::ProcessSpec spec { 48000.0, 512, 2 };
            engine.prepare (spec);

            auto makeDry = []
            {
                juce::AudioBuffer<float> buffer (2, 512);
                for (int i = 0; i < 512; ++i)
                {
                    buffer.setSample (0, i, 0.5f);
                    buffer.setSample (1, i, 0.5f);
                }
                return buffer;
            };

            auto withAir = makeDry();
            engine.process (withAir, true, 0.01f, false,
                            0.f, 0.4f, 0, 0.f, 0.f, 0.f, 0.f, 0.f,
                            1.f, false, 0.f, 0.f, 120.0);

            auto withoutAir = makeDry();
            engine.process (withoutAir, true, 0.01f, false,
                            0.f, 0.4f, 0, 0.f, 0.f, 0.f, 0.f, 0.f,
                            0.f, false, 0.f, 0.f, 120.0);

            float diff = 0.f;
            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < 512; ++i)
                    diff = juce::jmax (diff, std::abs (withAir.getSample (ch, i) - withoutAir.getSample (ch, i)));

            expect (diff < 0.0002f,
                    "AIR at wet=0.01 should be inaudible when gated by wet at source");
        }

        beginTest ("low wet keeps the grain bed proportional, and it dies once the capture refills");
        {
            TextureEngine engine;
            juce::dsp::ProcessSpec spec { 48000.0, 512, 2 };
            engine.prepare (spec);

            juce::AudioBuffer<float> hot (2, 512);
            for (int block = 0; block < 400; ++block)
            {
                for (int i = 0; i < 512; ++i)
                {
                    const float t = (float) (block * 512 + i) / 48000.f;
                    const float x = std::sin (2.f * juce::MathConstants<float>::pi * 440.f * t);
                    hot.setSample (0, i, x);
                    hot.setSample (1, i, x);
                }
                engine.process (hot, true, 1.f, false,
                                1.f, 1.f, 0, 1.f, 1.f, 0.f, 1.f, 1.f,
                                1.f, false, 1.f, 0.f, 120.0);
            }

            // A granular layer is meant to keep granulating what it just heard —
            // that tail is the effect. What it must not do is drone forever, or
            // come back louder than the mix asks for. (Grain density used to be
            // multiplied by the mix, which is why the layer was silent at any
            // normal setting; the tail is now proportional to the mix instead.)
            juce::AudioBuffer<float> silent (2, 512);
            silent.clear();
            engine.process (silent, true, 0.02f, false,
                            1.f, 1.f, 0, 1.f, 1.f, 0.f, 1.f, 1.f,
                            1.f, false, 1.f, 0.f, 120.0);

            expect (silent.getMagnitude (0, 512) < 0.05f,
                    "2 % wet stays proportionally quiet: " + juce::String (silent.getMagnitude (0, 512)));

            // The capture is a rolling 4 s window, so silence at the input
            // overwrites it and the bed has to decay to nothing on its own.
            for (int block = 0; block < 500; ++block)   // > 5 s at 512 / 48 kHz
            {
                silent.clear();
                engine.process (silent, true, 0.02f, false,
                                1.f, 1.f, 0, 1.f, 1.f, 0.f, 1.f, 1.f,
                                1.f, false, 1.f, 0.f, 120.0);
            }
            expect (silent.getMagnitude (0, 512) < 1.0e-4f,
                    "the grain bed dies once the capture has refilled with silence: "
                        + juce::String (silent.getMagnitude (0, 512)));
        }
    }
};

static TextureEngineTests textureEngineTests;
