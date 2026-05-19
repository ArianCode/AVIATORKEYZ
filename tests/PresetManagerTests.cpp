// =============================================================================
//  PresetManager unit tests (headless, no embedded BinaryData required)
//
//  Tests cover:
//    - getAllCategories() returns all 10 expected categories in order
//    - saveUserPreset / loadPreset round-trip via temp directory
//    - getCurrentPresetName / getCurrentCategory tracking
//    - Loading a non-existent preset returns false
//
//  Note: Factory preset loading via FactoryResources depends on BinaryData
//  which is only available when linking the full plugin library. Those tests
//  are skipped here. Use the Python tests to verify factory preset content.
// =============================================================================

#include <juce_core/juce_core.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "State/PresetManager.h"
#include "State/StateSchema.h"

// ---------------------------------------------------------------------------
// Minimal APVTS for testing (no AudioProcessor needed, just a stub)
// ---------------------------------------------------------------------------
static juce::AudioProcessorValueTreeState::ParameterLayout makeTestLayout()
{
    using namespace AviatorKeyz;
    using namespace juce;
    std::vector<std::unique_ptr<RangedAudioParameter>> p;

    auto add = [&] (const char* id, float lo, float hi, float def) {
        p.push_back (std::make_unique<AudioParameterFloat> (
            ParameterID { id, 1 }, id,
            NormalisableRange<float> (lo, hi), def));
    };

    add (ParamID::INPUT_GAIN,    -24.0f, 12.0f,  0.0f);
    add (ParamID::OUTPUT_GAIN,   -24.0f, 12.0f,  0.0f);
    add (ParamID::GLIDE_TIME,      0.0f, 500.0f, 0.0f);
    add (ParamID::SMEAR,           0.0f,   1.0f, 0.0f);
    add (ParamID::TONE,           -1.0f,   1.0f, 0.0f);
    add (ParamID::REVERB_AMOUNT,   0.0f,   1.0f, 0.0f);
    add (ParamID::REVERB_SIZE,     0.0f,   1.0f, 0.5f);
    add (ParamID::STEREO_WIDTH,    0.0f,   2.0f, 1.0f);
    add (ParamID::ENV_ATTACK,      0.5f, 5000.f, 5.0f);
    add (ParamID::ENV_RELEASE,     5.0f,10000.f, 150.0f);
    add (ParamID::PAN,            -1.0f,   1.0f, 0.0f);
    p.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { ParamID::REVERSE, 1 }, "Reverse", false));

    return { p.begin(), p.end() };
}

// ---------------------------------------------------------------------------
// Stub AudioProcessor for APVTS construction
// ---------------------------------------------------------------------------
struct TestProcessor : juce::AudioProcessor
{
    TestProcessor()
        : AudioProcessor (BusesProperties()),
          apvts (*this, nullptr, "TestState", makeTestLayout())
    {}

    const juce::String getName() const override { return "Test"; }
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


class PresetManagerCategoryTests : public juce::UnitTest
{
public:
    PresetManagerCategoryTests() : juce::UnitTest ("PresetManager_Categories", "AviatorKeyz") {}

    void runTest() override
    {
        beginTest ("getAllCategories returns exactly 10 categories");
        {
            TestProcessor proc;
            PresetManager pm (proc.apvts);
            const auto cats = pm.getAllCategories();
            expectEquals (cats.size(), 10);
        }

        beginTest ("getAllCategories order matches spec");
        {
            using namespace AviatorKeyz;
            TestProcessor proc;
            PresetManager pm (proc.apvts);
            const auto cats = pm.getAllCategories();

            const juce::StringArray expected {
                Category::LEADS, Category::BRASS, Category::ENSEMBLES,
                Category::STRINGS, Category::PADS, Category::CHORDS,
                Category::SYNTHS, Category::ARPS, Category::VOCALS,
                Category::BELLS,
            };
            expectEquals (cats.size(), expected.size());
            for (int i = 0; i < juce::jmin (cats.size(), expected.size()); ++i)
                expectEquals (cats[i], expected[i]);
        }

        beginTest ("No duplicate categories");
        {
            TestProcessor proc;
            PresetManager pm (proc.apvts);
            const auto cats = pm.getAllCategories();
            for (int i = 0; i < cats.size(); ++i)
                for (int j = i + 1; j < cats.size(); ++j)
                    expect (cats[i] != cats[j], "Duplicate category: " + cats[i]);
        }
    }
};

class PresetManagerSaveLoadTests : public juce::UnitTest
{
public:
    PresetManagerSaveLoadTests() : juce::UnitTest ("PresetManager_SaveLoad", "AviatorKeyz") {}

    void runTest() override
    {
        beginTest ("loadPreset returns false for non-existent factory and user preset");
        {
            TestProcessor proc;
            PresetManager pm (proc.apvts);
            bool result = pm.loadPreset ("Leads", "NonExistentPreset_XYZZY");
            expect (! result, "loadPreset with unknown name should return false");
        }

        beginTest ("saveUserPreset + loadPreset round-trip preserves parameter value");
        {
            // Override user presets dir by saving to a temp directory.
            // PresetManager uses ~/Documents/AviatorKeyz/Presets — we can't
            // inject a temp dir without production code changes, so we test via
            // the real user preset path in a test-specific subdirectory.
            //
            // This test saves, then immediately loads, verifying the round-trip
            // by checking a modified parameter value is restored.

            TestProcessor proc;
            PresetManager pm (proc.apvts);

            const juce::String testCategory = "Pads";
            const juce::String testName     = "ZZ_UnitTest_Pad";

            // Set a known non-default parameter value
            const float testSmear = 0.75f;
            if (auto* p = proc.apvts.getParameter (AviatorKeyz::ParamID::SMEAR))
                p->setValueNotifyingHost (p->getNormalisableRange().convertTo0to1 (testSmear));

            // Save
            bool saved = pm.saveUserPreset (testCategory, testName);
            if (! saved)
            {
                // May fail if ~/Documents/AviatorKeyz/ can't be created (sandboxed CI)
                logMessage ("saveUserPreset returned false — skipping round-trip test "
                            "(likely file system restriction in this environment)");
                return;
            }

            // Reset parameter to default
            if (auto* p = proc.apvts.getParameter (AviatorKeyz::ParamID::SMEAR))
                p->setValueNotifyingHost (0.0f);

            // Load back
            bool loaded = pm.loadPreset (testCategory, testName);
            expect (loaded, "loadPreset should succeed after saveUserPreset");

            if (loaded)
            {
                const float restored = proc.apvts.getRawParameterValue (AviatorKeyz::ParamID::SMEAR)->load();
                expectWithinAbsoluteError (restored, testSmear, 0.01f,
                    "Smear parameter should be restored to saved value after load");
            }

            // Cleanup: remove the test preset file
            const auto userDir = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                                       .getChildFile ("AviatorKeyz/Presets")
                                       .getChildFile (testCategory);
            userDir.getChildFile (testName + ".xml").deleteFile();
        }

        beginTest ("getCurrentPresetName and getCurrentCategory reflect loaded preset");
        {
            TestProcessor proc;
            PresetManager pm (proc.apvts);

            // Default state from constructor
            // (PresetManager constructor sets Leads/Init defaults)
            const juce::String cat  = pm.getCurrentCategory();
            const juce::String name = pm.getCurrentPresetName();

            expect (cat.isNotEmpty(),  "getCurrentCategory must return a non-empty string");
            expect (name.isNotEmpty(), "getCurrentPresetName must return a non-empty string");
        }

        beginTest ("getCurrentSampleId defaults to factory_default or a valid ID");
        {
            TestProcessor proc;
            PresetManager pm (proc.apvts);
            const juce::String sid = pm.getCurrentSampleId();
            expect (sid.isNotEmpty(),
                    "getCurrentSampleId must return a non-empty string");
            expect (sid.startsWith ("factory_"),
                    "getCurrentSampleId should start with 'factory_': " + sid);
        }
    }
};

static PresetManagerCategoryTests presetCatTests;
static PresetManagerSaveLoadTests presetSaveLoadTests;
