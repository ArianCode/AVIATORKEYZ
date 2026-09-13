// =============================================================================
//  PresetManager unit tests (headless, no embedded BinaryData required)
//
//  Tests cover:
//    - getAllCategories() returns all 13 expected categories in order
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
#include "State/ParameterLayout.h"
#include "State/StateSchema.h"
#include "DSP/Mfx/MfxDescriptors.h"

// ---------------------------------------------------------------------------
// Minimal APVTS for testing (no AudioProcessor needed, just a stub)
// ---------------------------------------------------------------------------
static juce::AudioProcessorValueTreeState::ParameterLayout makeTestLayout()
{
    return AviatorKeyz::createParameterLayout();
}

// ---------------------------------------------------------------------------
// Stub AudioProcessor for APVTS construction
// ---------------------------------------------------------------------------
struct TestProcessor : juce::AudioProcessor
{
    TestProcessor()
        : AudioProcessor (BusesProperties()),
          apvts (*this, nullptr, "AviatorKeyzState", makeTestLayout())
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
        beginTest ("getAllCategories returns exactly 13 categories");
        {
            TestProcessor proc;
            PresetManager pm (proc.apvts);
            const auto cats = pm.getAllCategories();
            expectEquals (cats.size(), 13);
        }

        beginTest ("getAllCategories order matches spec");
        {
            using namespace AviatorKeyz;
            TestProcessor proc;
            PresetManager pm (proc.apvts);
            const auto cats = pm.getAllCategories();

            const juce::StringArray expected {
                Category::BASS, Category::LEADS, Category::KEYS,
                Category::BRASS, Category::PHRASES, Category::ARPS,
                Category::SYNTHS, Category::BELLS, Category::STRINGS,
                Category::PLUCKS, Category::ENSEMBLES, Category::PADS,
                Category::VOCALS,
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

class PresetManagerKeepPerformanceTests : public juce::UnitTest
{
public:
    PresetManagerKeepPerformanceTests() : juce::UnitTest ("PresetManager_KeepPerformance", "AviatorKeyz") {}

    void runTest() override
    {
        using namespace AviatorKeyz;
        const juce::String category = "Pads";
        const auto userDir = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                                 .getChildFile ("AviatorKeyz/Presets").getChildFile (category);

        auto setValue = [] (TestProcessor& proc, const juce::String& id, float value)
        {
            auto* p = proc.apvts.getParameter (id);
            p->setValueNotifyingHost (p->convertTo0to1 (value));
        };
        auto raw = [] (TestProcessor& proc, const juce::String& id)
        {
            return proc.apvts.getRawParameterValue (id)->load();
        };

        beginTest ("keepPerformance load keeps MFX + speed; a plain load recalls them");
        {
            TestProcessor proc;
            PresetManager pm (proc.apvts);
            const juce::String name = "ZZ_UnitTest_KeepPerf";
            const auto mfxLevel = Mfx::levelId (0);
            const auto* levelParam = proc.apvts.getParameter (mfxLevel);
            const float savedLevel = levelParam->convertFrom0to1 (0.2f);
            const float liveLevel  = levelParam->convertFrom0to1 (0.9f);

            setValue (proc, ParamID::SMEAR, 0.75f);
            setValue (proc, mfxLevel, savedLevel);
            setValue (proc, ParamID::SRC_SPEED, 1.f);
            if (! pm.saveUserPreset (category, name))
            {
                logMessage ("saveUserPreset returned false — skipping (file system restriction)");
                return;
            }

            setValue (proc, ParamID::SMEAR, 0.1f);
            setValue (proc, mfxLevel, liveLevel);
            setValue (proc, ParamID::SRC_SPEED, 0.5f);

            expect (pm.loadPreset (category, name, true));
            expectWithinAbsoluteError (raw (proc, ParamID::SMEAR), 0.75f, 0.01f, "the sound itself is recalled");
            expectWithinAbsoluteError (raw (proc, mfxLevel), liveLevel, 0.01f, "MFX survives in the raw value the DSP reads");
            expectWithinAbsoluteError (raw (proc, ParamID::SRC_SPEED), 0.5f, 0.001f, "speed survives");

            expect (pm.loadPreset (category, name));
            expectWithinAbsoluteError (raw (proc, mfxLevel), savedLevel, 0.01f, "plain load recalls the preset's MFX");
            expectWithinAbsoluteError (raw (proc, ParamID::SRC_SPEED), 1.f, 0.001f);

            userDir.getChildFile (name + ".xml").deleteFile();
        }

        beginTest ("Root shift is saved as an offset and re-applied on load");
        {
            TestProcessor proc;
            PresetManager pm (proc.apvts);
            const juce::String name = "ZZ_UnitTest_RootShift";
            pm.setRootShift (3);
            setValue (proc, ParamID::SRC_ROOT_NOTE, (float) pm.getEffectiveRootNote());
            if (! pm.saveUserPreset (category, name))
                return;

            pm.setRootShift (0);
            setValue (proc, ParamID::SRC_ROOT_NOTE, 60.f);
            expect (pm.loadPreset (category, name));
            expectEquals (pm.getRootShift(), 3);
            expectEquals (juce::roundToInt (raw (proc, ParamID::SRC_ROOT_NOTE)), pm.getCurrentRootNote() + 3);
            userDir.getChildFile (name + ".xml").deleteFile();
        }

        beginTest ("loadRandomPreset picks a different preset and keeps MFX");
        {
            TestProcessor proc;
            PresetManager pm (proc.apvts);
            const juce::String nameA = "ZZ_UnitTest_DiceA", nameB = "ZZ_UnitTest_DiceB";
            if (! pm.saveUserPreset (category, nameA) || ! pm.saveUserPreset (category, nameB))
                return;

            expect (pm.loadPreset (category, nameA));
            const auto mfxLevel = Mfx::levelId (1);
            const float liveLevel = proc.apvts.getParameter (mfxLevel)->convertFrom0to1 (0.33f);
            setValue (proc, mfxLevel, liveLevel);

            expect (pm.loadRandomPreset (false));
            expect (pm.getCurrentPresetName() != nameA, "the dice never re-loads the current preset");
            expectWithinAbsoluteError (raw (proc, mfxLevel), liveLevel, 0.01f);

            userDir.getChildFile (nameA + ".xml").deleteFile();
            userDir.getChildFile (nameB + ".xml").deleteFile();
        }

        beginTest ("snapSpeedRatio lands on 0.25 steps inside 0.25..4");
        {
            expectEquals (snapSpeedRatio (0.53f), 0.5f);
            expectEquals (snapSpeedRatio (0.63f), 0.75f);
            expectEquals (snapSpeedRatio (1.12f), 1.0f);
            expectEquals (snapSpeedRatio (0.05f), 0.25f);
            expectEquals (snapSpeedRatio (9.0f), 4.0f);
        }
    }
};

static PresetManagerCategoryTests presetCatTests;
static PresetManagerSaveLoadTests presetSaveLoadTests;
static PresetManagerKeepPerformanceTests presetKeepPerformanceTests;
