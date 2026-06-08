// =============================================================================
//  ApvtsStateHelpers — factory PARAM format vs host property format
// =============================================================================

#include <juce_core/juce_core.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "State/ApvtsStateHelpers.h"
#include "State/ParameterLayout.h"
#include "State/StateSchema.h"

struct ApvtsTestProcessor : juce::AudioProcessor
{
    ApvtsTestProcessor()
        : AudioProcessor (BusesProperties()),
          apvts (*this, nullptr, "AviatorKeyzState", AviatorKeyz::createParameterLayout())
    {}

    const juce::String getName() const override { return "ApvtsTest"; }
    bool acceptsMidi() const override { return false; }
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

class ApvtsStateHelpersTests : public juce::UnitTest
{
public:
    ApvtsStateHelpersTests() : juce::UnitTest ("ApvtsStateHelpers", "AviatorKeyz") {}

    void runTest() override
    {
        beginTest ("Factory PARAM children apply denormalized values");
        {
            ApvtsTestProcessor proc;
            const auto factoryXml = juce::XmlDocument::parse (R"(
<AviatorKeyzState stateVersion="1">
  <PARAM id="smear" value="0.75"/>
  <PARAM id="tone" value="-0.4"/>
  <PARAM id="input_gain" value="-6.0"/>
</AviatorKeyzState>)");

            expect (factoryXml != nullptr);
            const auto state = juce::ValueTree::fromXml (*factoryXml);
            expect (AviatorKeyz::stateTreeUsesParamChildren (state));

            AviatorKeyz::applyStateTreeToApvts (proc.apvts, state);

            expectWithinAbsoluteError (
                proc.apvts.getRawParameterValue (AviatorKeyz::ParamID::SMEAR)->load(),
                0.75f, 0.001f);
            expectWithinAbsoluteError (
                proc.apvts.getRawParameterValue (AviatorKeyz::ParamID::TONE)->load(),
                -0.4f, 0.001f);
            expectWithinAbsoluteError (
                proc.apvts.getRawParameterValue (AviatorKeyz::ParamID::INPUT_GAIN)->load(),
                -6.0f, 0.01f);
            expectWithinAbsoluteError (
                proc.apvts.getRawParameterValue (AviatorKeyz::ParamID::SOURCE_BLEND)->load(),
                0.0f, 0.001f);
        }

        beginTest ("Host copyState round-trip via property format");
        {
            ApvtsTestProcessor proc;

            if (auto* p = proc.apvts.getParameter (AviatorKeyz::ParamID::SMEAR))
            {
                if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (p))
                    p->setValueNotifyingHost (ranged->convertTo0to1 (0.42f));
            }

            auto saved = proc.apvts.copyState();
            saved.setProperty ("stateVersion", AviatorKeyz::STATE_SCHEMA_VERSION, nullptr);
            expect (! AviatorKeyz::isPartialFactoryPresetState (saved));

            if (auto* p = proc.apvts.getParameter (AviatorKeyz::ParamID::SMEAR))
                p->setValueNotifyingHost (0.0f);

            AviatorKeyz::applyStateTreeToApvts (proc.apvts, saved);

            expectWithinAbsoluteError (
                proc.apvts.getRawParameterValue (AviatorKeyz::ParamID::SMEAR)->load(),
                0.42f, 0.001f);
        }

        beginTest ("Partial factory preset resets unstored params and sample blend");
        {
            ApvtsTestProcessor proc;

            if (auto* p = proc.apvts.getParameter (AviatorKeyz::ParamID::SOURCE_BLEND))
            {
                if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (p))
                    p->setValueNotifyingHost (ranged->convertTo0to1 (0.8f));
            }

            const auto factoryXml = juce::XmlDocument::parse (R"(
<AviatorKeyzState stateVersion="1">
  <PARAM id="smear" value="0.5"/>
</AviatorKeyzState>)");

            const auto state = juce::ValueTree::fromXml (*factoryXml);
            expect (AviatorKeyz::isPartialFactoryPresetState (state));

            AviatorKeyz::applyStateTreeToApvts (proc.apvts, state);

            expectWithinAbsoluteError (
                proc.apvts.getRawParameterValue (AviatorKeyz::ParamID::SMEAR)->load(),
                0.5f, 0.001f);
            expectWithinAbsoluteError (
                proc.apvts.getRawParameterValue (AviatorKeyz::ParamID::SOURCE_BLEND)->load(),
                0.0f, 0.001f);
            expectWithinAbsoluteError (
                proc.apvts.getRawParameterValue (AviatorKeyz::ParamID::PERF_MACRO_2)->load(),
                0.5f, 0.001f);
            expectWithinAbsoluteError (
                proc.apvts.getRawParameterValue (AviatorKeyz::ParamID::TEX_AMOUNT)->load(),
                0.0f, 0.001f);
        }

        beginTest ("Real factory Init.xml PARAM values apply");
        {
            const auto initFile = juce::File::getCurrentWorkingDirectory()
                .getChildFile ("Resources/Presets/Factory/Leads/Init.xml");
            if (! initFile.existsAsFile())
            {
                logMessage ("Init.xml not found from CWD — skipping file-based test");
                return;
            }

            const auto parsed = juce::XmlDocument::parse (initFile);
            expect (parsed != nullptr);
            const auto stateEl = parsed->getChildByName ("AviatorKeyzState");
            expect (stateEl != nullptr);

            ApvtsTestProcessor proc;
            const auto state = juce::ValueTree::fromXml (*stateEl);
            AviatorKeyz::applyStateTreeToApvts (proc.apvts, state);

            expectWithinAbsoluteError (
                proc.apvts.getRawParameterValue (AviatorKeyz::ParamID::TONE)->load(),
                0.1f, 0.001f);
            expectWithinAbsoluteError (
                proc.apvts.getRawParameterValue (AviatorKeyz::ParamID::REVERB_AMOUNT)->load(),
                0.1f, 0.001f);
        }
    }
};

static ApvtsStateHelpersTests apvtsStateHelpersTests;
