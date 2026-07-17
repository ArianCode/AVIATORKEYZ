#include "../Source/DSP/Performance/MacroMapper.h"
#include "../Source/DSP/Performance/PhraseChopper.h"
#include "../Source/State/ParameterLayout.h"
#include "../Source/State/StateSchema.h"
#include <juce_core/juce_core.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

class MacroMapperTests : public juce::UnitTest
{
public:
    MacroMapperTests() : juce::UnitTest ("MacroMapper", "Performance") {}

    void runTest() override
    {
        beginTest ("Category defaults produce names");
        const auto macros = MacroMapper::defaultsForCategory ("Vocals");
        expect (macros[0].name.equalsIgnoreCase ("Chop"));
        expect (macros[1].name.equalsIgnoreCase ("Texture"));

        beginTest ("Destination string parsing");
        expect (MacroMapper::destinationFromString ("ChopAmount") == MacroDestination::ChopAmount);
        expect (MacroMapper::destinationFromString ("TextureMix") == MacroDestination::TextureMix);

        beginTest ("Macro mapping applies offsets");
        EngineState base;
        base.chop.amount = 0.2f;
        std::array<MacroControl, 4> macros2 = MacroMapper::defaultsForCategory ("Leads");

        struct DummyProc : juce::AudioProcessor
        {
            DummyProc() : AudioProcessor (BusesProperties().withOutput ("Out", juce::AudioChannelSet::mono(), true))
                          , apvts (*this, nullptr, "S", AviatorKeyz::createParameterLayout()) {}
            const juce::String getName() const override { return "Dummy"; }
            void prepareToPlay (double, int) override {}
            void releaseResources() override {}
            void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override {}
            juce::AudioProcessorEditor* createEditor() override { return nullptr; }
            bool hasEditor() const override { return false; }
            double getTailLengthSeconds() const override { return 0; }
            bool acceptsMidi() const override { return false; }
            bool producesMidi() const override { return false; }
            int getNumPrograms() override { return 1; }
            int getCurrentProgram() override { return 0; }
            void setCurrentProgram (int) override {}
            const juce::String getProgramName (int) override { return {}; }
            void changeProgramName (int, const juce::String&) override {}
            void getStateInformation (juce::MemoryBlock&) override {}
            void setStateInformation (const void*, int) override {}
            juce::AudioProcessorValueTreeState apvts;
        } proc;

        if (auto* p = proc.apvts.getParameter (AviatorKeyz::ParamID::PERF_MACRO_1))
            p->setValueNotifyingHost (1.0f);

        PerformanceApvtsReader::ParamCache cache;
        cache.init (proc.apvts);

        const auto resolved = MacroMapper::applyMacros (base, macros2, cache);
        expect (resolved.chop.amount > base.chop.amount);
    }
};

class PhraseChopperTests : public juce::UnitTest
{
public:
    PhraseChopperTests() : juce::UnitTest ("PhraseChopper", "Performance") {}

    void runTest() override
    {
        PhraseChopper chopper;
        juce::dsp::ProcessSpec spec { 48000.0, 512, 2 };
        chopper.prepare (spec);
        chopper.reset();

        EngineState state;
        state.chop.enabled = true;
        state.chop.amount = 0.8f;
        state.source.start = 0.f;
        state.source.end = 1.f;

        beginTest ("updatePlayback returns active slice");
        const auto pb = chopper.updatePlayback (state, 48000, 120.0);
        expect (pb.active);
        expect (pb.sliceEndFrame > pb.sliceStartFrame);

        beginTest ("process does not produce NaN");
        juce::AudioBuffer<float> buf (2, 256);
        for (int ch = 0; ch < 2; ++ch)
            juce::FloatVectorOperations::fill (buf.getWritePointer (ch), 0.5f, 256);
        chopper.process (buf, state, 120.0);
        for (int i = 0; i < 256; ++i)
        {
            expect (std::isfinite (buf.getSample (0, i)));
            expect (std::isfinite (buf.getSample (1, i)));
        }
    }
};

static MacroMapperTests macroMapperTests;
static PhraseChopperTests phraseChopperTests;
