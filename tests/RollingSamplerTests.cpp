// =============================================================================
//  RollingSamplerTests — the live 30-second window: audio rolls in at the right
//  and is deleted off the left, absolute frame positions keep pointing at the
//  same audio while it scrolls, the drawn envelope tracks the samples, and a
//  region round-trips into the active sound.
// =============================================================================

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

#include "PluginProcessor.h"
#include "DSP/RollingSampler.h"
#include "State/StateSchema.h"

namespace
{
constexpr double kSr = 48000.0;
constexpr int kBlock = 480;

/** Stereo block whose samples equal their absolute frame index. */
void fillCounting (juce::AudioBuffer<float>& b, int64_t firstFrame)
{
    for (int ch = 0; ch < b.getNumChannels(); ++ch)
        for (int i = 0; i < b.getNumSamples(); ++i)
            b.setSample (ch, i, (float) (firstFrame + i));
}

/** Stereo block of a constant value. */
void fillConst (juce::AudioBuffer<float>& b, float v)
{
    for (int ch = 0; ch < b.getNumChannels(); ++ch)
        juce::FloatVectorOperations::fill (b.getWritePointer (ch), v, b.getNumSamples());
}

void writeBlocks (RollingSampler& rs, int numBlocks, int64_t startFrame = 0)
{
    juce::AudioBuffer<float> b (2, kBlock);
    for (int i = 0; i < numBlocks; ++i)
    {
        fillCounting (b, startFrame + (int64_t) i * kBlock);
        rs.write (b);
    }
}
} // namespace

class RollingSamplerTests : public juce::UnitTest
{
public:
    RollingSamplerTests() : juce::UnitTest ("RollingSampler", "AviatorKeyz") {}

    void runTest() override
    {
        const int64_t capacity = (int64_t) (RollingSampler::kWindowSeconds * kSr);

        beginTest ("Disarmed: nothing is captured");
        {
            RollingSampler rs;
            rs.prepare (kSr, kBlock);
            writeBlocks (rs, 4);
            expect (! rs.isArmed());
            expectEquals ((int) rs.nowFrame(), 0);
            juce::AudioBuffer<float> out;
            expect (! rs.snapshot (out, 1.0), "snapshot fails with an empty ring");
        }

        beginTest ("Armed: the window is the last 30 s, and nowFrame is the right edge");
        {
            RollingSampler rs;
            rs.prepare (kSr, kBlock);
            rs.setArmed (true);
            expectEquals ((int) rs.getCapacityFrames(), (int) capacity);

            writeBlocks (rs, 10);
            const int64_t total = 10 * (int64_t) kBlock;
            expectEquals ((int) rs.nowFrame(), (int) total);
            expectEquals ((int) rs.oldestFrame(), 0, "nothing deleted yet");
            expectWithinAbsoluteError (rs.getBufferedSeconds(), (double) total / kSr, 1.0e-6);

            juce::AudioBuffer<float> out;
            const double wantSeconds = (double) (2 * kBlock) / kSr;
            expect (rs.snapshot (out, wantSeconds));
            expectEquals (out.getNumSamples(), 2 * kBlock);
            for (int i = 0; i < out.getNumSamples(); ++i)
                expectWithinAbsoluteError (out.getSample (0, i), (float) (total - 2 * kBlock + i), 0.001f);
        }

        beginTest ("Past 30 s: audio is deleted off the left and the window stops growing");
        {
            RollingSampler rs;
            rs.prepare (kSr, kBlock);
            rs.setArmed (true);

            const int blocks = (int) (capacity / kBlock) + 25; // overrun
            writeBlocks (rs, blocks);
            const int64_t total = (int64_t) blocks * kBlock;

            expectEquals ((int) rs.nowFrame(), (int) total);
            expectEquals ((int) rs.oldestFrame(), (int) (total - capacity), "left edge trails by exactly the window");
            expectWithinAbsoluteError (rs.getBufferedSeconds(), RollingSampler::kWindowSeconds, 1.0e-6);

            juce::AudioBuffer<float> out;
            expect (rs.snapshot (out, 1.0));
            expectEquals (out.getNumSamples(), (int) kSr);
            // Still contiguous across the wrap in the underlying ring.
            for (int i = 1; i < out.getNumSamples(); ++i)
                expectWithinAbsoluteError (out.getSample (0, i) - out.getSample (0, i - 1), 1.f, 0.001f);
            expectWithinAbsoluteError (out.getSample (0, out.getNumSamples() - 1), (float) (total - 1), 0.001f);
        }

        beginTest ("An absolute range keeps pointing at the same audio as the window scrolls");
        {
            RollingSampler rs;
            rs.prepare (kSr, kBlock);
            rs.setArmed (true);
            writeBlocks (rs, 20);

            // Mark a range, then let 10 more blocks roll in under it.
            const int64_t from = 3 * (int64_t) kBlock;
            const int64_t to   = 5 * (int64_t) kBlock;
            writeBlocks (rs, 10, 20 * (int64_t) kBlock);

            juce::AudioBuffer<float> out;
            expect (rs.snapshotRange (out, from, to), "the marked audio is still reachable");
            expectEquals (out.getNumSamples(), (int) (to - from));
            for (int i = 0; i < out.getNumSamples(); ++i)
                expectWithinAbsoluteError (out.getSample (0, i), (float) (from + i), 0.001f);
        }

        beginTest ("A range that has rolled out is refused, not silently shifted");
        {
            RollingSampler rs;
            rs.prepare (kSr, kBlock);
            rs.setArmed (true);

            const int64_t from = 0, to = (int64_t) kBlock;
            writeBlocks (rs, (int) (capacity / kBlock) + 20); // push it off the left

            expect (rs.oldestFrame() > to, "the range is behind the left edge");
            juce::AudioBuffer<float> out;
            expect (! rs.snapshotRange (out, from, to), "aged-out range must fail");
        }

        beginTest ("A range straddling the left edge is clamped to what survives");
        {
            RollingSampler rs;
            rs.prepare (kSr, kBlock);
            rs.setArmed (true);
            writeBlocks (rs, (int) (capacity / kBlock) + 20);

            const int64_t oldest = rs.oldestFrame();
            juce::AudioBuffer<float> out;
            // Ask from well before the left edge to a little after it.
            expect (rs.snapshotRange (out, oldest - 10 * kBlock, oldest + 5 * kBlock));
            expectEquals (out.getNumSamples(), 5 * kBlock, "only the surviving part comes back");
            expectWithinAbsoluteError (out.getSample (0, 0), (float) oldest, 1.0f);
        }

        beginTest ("Envelope tracks the samples and is flat where audio never existed");
        {
            RollingSampler rs;
            rs.prepare (kSr, kBlock);
            rs.setArmed (true);

            // Half a second of silence, then half a second at +/-0.5.
            juce::AudioBuffer<float> b (2, kBlock);
            const int halfBlocks = (int) (kSr / 2 / kBlock);
            fillConst (b, 0.f);
            for (int i = 0; i < halfBlocks; ++i) rs.write (b);
            for (int i = 0; i < halfBlocks; ++i)
            {
                for (int ch = 0; ch < 2; ++ch)
                    for (int s = 0; s < kBlock; ++s)
                        b.setSample (ch, s, (s % 2 == 0) ? 0.5f : -0.5f);
                rs.write (b);
            }

            constexpr int kCols = 60;
            RollingSampler::Bucket cols[kCols];
            const int64_t now = rs.nowFrame();
            rs.readEnvelope (cols, kCols, now - (int64_t) kSr, now);

            // Last quarter of the columns is the loud half; first quarter is silence.
            expect (cols[3].max < 0.01f && cols[3].min > -0.01f, "silence reads flat");
            expect (cols[kCols - 4].max > 0.4f, "loud section reads loud: " + juce::String (cols[kCols - 4].max, 3));
            expect (cols[kCols - 4].min < -0.4f, "and negative: " + juce::String (cols[kCols - 4].min, 3));

            // A window reaching before anything was written must stay flat there.
            RollingSampler::Bucket early[kCols];
            rs.readEnvelope (early, kCols, -2 * (int64_t) kSr, -(int64_t) kSr);
            for (int i = 0; i < kCols; ++i)
                expect (early[i].max == 0.f && early[i].min == 0.f, "pre-history is flat");
        }

        beginTest ("reset() empties the window");
        {
            RollingSampler rs;
            rs.prepare (kSr, kBlock);
            rs.setArmed (true);
            writeBlocks (rs, 4);
            rs.reset();
            expectEquals ((int) rs.nowFrame(), 0);
            expectWithinAbsoluteError (rs.getBufferedSeconds(), 0.0, 1.0e-9);
            juce::AudioBuffer<float> out;
            expect (! rs.snapshot (out, 1.0));
        }

        beginTest ("Processor round-trip: arm, play, send a live region into cargo");
        {
            AviatorKeyzProcessor p;
            p.getPresetManager().loadPreset (AviatorKeyz::Category::LEADS, "Init");
            p.prepareToPlay (kSr, kBlock);

            juce::String err;
            juce::File unused;
            expect (! p.exportRollingRange (0, 4800, unused, err), "export before arming must fail");
            expect (err.isNotEmpty(), "and say why");

            p.setRollingSamplerArmed (true);
            expect (p.isRollingSamplerArmed());
            expectWithinAbsoluteError (p.getRollingWindowSeconds(), RollingSampler::kWindowSeconds, 1.0e-9);

            // Hold a note for ~1 s so the window has real output in it.
            juce::AudioBuffer<float> buffer (2, kBlock);
            for (int i = 0; i < (int) (kSr / kBlock); ++i)
            {
                juce::MidiBuffer midi;
                if (i == 0) midi.addEvent (juce::MidiMessage::noteOn (1, 60, (juce::uint8) 110), 0);
                buffer.clear();
                p.processBlock (buffer, midi);
            }
            expect (p.getRollingSamplerSeconds() > 0.9, "window filled: "
                        + juce::String (p.getRollingSamplerSeconds(), 2) + "s");
            expect (p.getRollingNowFrame() > 0);
            expectEquals ((int) p.getRollingOldestFrame(), 0, "nothing deleted inside the first 30 s");

            // Chop the middle half of what we captured and send it to cargo.
            const int64_t now = p.getRollingNowFrame();
            const int64_t from = now / 4, to = now * 3 / 4;

            err.clear();
            const bool loaded = p.loadRollingRange (from, to, err);
            expect (loaded, "region loaded as the active sound: " + err);
            if (loaded)
            {
                const auto& info = p.getUserSampleInfo();
                expect (info.loaded, "the region became the active sound");
                expect (info.name.startsWith ("Capture_"), "named as a capture: " + info.name);
                const double expectedSeconds = (double) (to - from) / kSr;
                expectWithinAbsoluteError (info.seconds, expectedSeconds, 0.05);
                expect (p.hasSamplerSampleForTest(), "sampler holds the captured audio");
                expect (p.getAPVTS().getRawParameterValue (AviatorKeyz::ParamID::SRC_KEYTRACK)->load() > 0.5f,
                        "a captured region tracks the keyboard like any other cargo");
                juce::File (info.path).deleteFile();
            }

            // Drag-out writes a WAV without disturbing the loaded sound.
            juce::File dragFile;
            err.clear();
            expect (p.exportRollingRange (from, to, dragFile, err), "export for DAW drag: " + err);
            expect (dragFile.existsAsFile());
            expect (dragFile.getFileExtension() == ".wav", "DAWs take WAV on a file drop");
            expect (dragFile.getSize() > 1000);
            dragFile.deleteFile();

            p.setRollingSamplerArmed (false);
            p.clearRollingSampler();
            expectWithinAbsoluteError (p.getRollingSamplerSeconds(), 0.0, 1.0e-9);
        }
    }
};

static RollingSamplerTests rollingSamplerTests;
