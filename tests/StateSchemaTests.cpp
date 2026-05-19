// =============================================================================
//  StateSchema compile-time and constant tests
//
//  Tests cover:
//    - All ParamID constants are non-empty strings
//    - No two ParamID constants are equal (no accidental duplicates)
//    - All SampleID constants are non-empty strings
//    - All Category constants are non-empty strings
//    - STATE_SCHEMA_VERSION is 1
//    - Constant values match documented ranges from README/docs
// =============================================================================

#include <juce_core/juce_core.h>
#include "State/StateSchema.h"

class StateSchemaConstantTests : public juce::UnitTest
{
public:
    StateSchemaConstantTests() : juce::UnitTest ("StateSchema_Constants", "AviatorKeyz") {}

    void runTest() override
    {
        using namespace AviatorKeyz;

        beginTest ("STATE_SCHEMA_VERSION == 1");
        expectEquals (STATE_SCHEMA_VERSION, 1);

        // -----------------------------------------------------------------------
        beginTest ("All ParamID constants are non-empty strings");
        {
            const char* ids[] = {
                ParamID::INPUT_GAIN,
                ParamID::OUTPUT_GAIN,
                ParamID::REVERSE,
                ParamID::GLIDE_TIME,
                ParamID::SMEAR,
                ParamID::TONE,
                ParamID::REVERB_AMOUNT,
                ParamID::REVERB_SIZE,
                ParamID::STEREO_WIDTH,
                ParamID::ENV_ATTACK,
                ParamID::ENV_RELEASE,
                ParamID::PAN,
            };
            for (const char* id : ids)
                expect (id != nullptr && juce::String (id).isNotEmpty(),
                        "ParamID constant must not be empty");
        }

        beginTest ("ParamID constants have expected string values");
        {
            expectEquals (juce::String (ParamID::INPUT_GAIN),    juce::String ("input_gain"));
            expectEquals (juce::String (ParamID::OUTPUT_GAIN),   juce::String ("output_gain"));
            expectEquals (juce::String (ParamID::REVERSE),       juce::String ("reverse"));
            expectEquals (juce::String (ParamID::GLIDE_TIME),    juce::String ("glide_time"));
            expectEquals (juce::String (ParamID::SMEAR),         juce::String ("smear"));
            expectEquals (juce::String (ParamID::TONE),          juce::String ("tone"));
            expectEquals (juce::String (ParamID::REVERB_AMOUNT), juce::String ("reverb_amount"));
            expectEquals (juce::String (ParamID::REVERB_SIZE),   juce::String ("reverb_size"));
            expectEquals (juce::String (ParamID::STEREO_WIDTH),  juce::String ("stereo_width"));
            expectEquals (juce::String (ParamID::ENV_ATTACK),    juce::String ("env_attack"));
            expectEquals (juce::String (ParamID::ENV_RELEASE),   juce::String ("env_release"));
            expectEquals (juce::String (ParamID::PAN),           juce::String ("pan"));
        }

        beginTest ("No duplicate ParamID values");
        {
            juce::StringArray ids {
                ParamID::INPUT_GAIN, ParamID::OUTPUT_GAIN, ParamID::REVERSE,
                ParamID::GLIDE_TIME, ParamID::SMEAR, ParamID::TONE,
                ParamID::REVERB_AMOUNT, ParamID::REVERB_SIZE, ParamID::STEREO_WIDTH,
                ParamID::ENV_ATTACK, ParamID::ENV_RELEASE, ParamID::PAN,
            };
            expectEquals (ids.size(), 12);

            for (int i = 0; i < ids.size(); ++i)
                for (int j = i + 1; j < ids.size(); ++j)
                    expect (ids[i] != ids[j],
                            "Duplicate ParamID: " + ids[i] + " == " + ids[j]);
        }

        // -----------------------------------------------------------------------
        beginTest ("All SampleID constants are non-empty and follow naming convention");
        {
            const char* sids[] = {
                SampleID::DEFAULT,
                SampleID::LEADS,
                SampleID::BRASS,
                SampleID::ENSEMBLES,
                SampleID::STRINGS,
                SampleID::PADS,
                SampleID::CHORDS,
                SampleID::SYNTHS,
                SampleID::ARPS,
                SampleID::VOCALS,
                SampleID::BELLS,
            };
            for (const char* sid : sids)
            {
                juce::String s (sid);
                expect (s.isNotEmpty(), "SampleID must not be empty");
                expect (s.startsWith ("factory_"),
                        "SampleID must start with 'factory_': " + s);
            }
        }

        beginTest ("SampleID::DEFAULT is 'factory_default'");
        expectEquals (juce::String (SampleID::DEFAULT), juce::String ("factory_default"));

        beginTest ("No duplicate SampleID values");
        {
            juce::StringArray sids {
                SampleID::DEFAULT, SampleID::LEADS, SampleID::BRASS,
                SampleID::ENSEMBLES, SampleID::STRINGS, SampleID::PADS,
                SampleID::CHORDS, SampleID::SYNTHS, SampleID::ARPS,
                SampleID::VOCALS, SampleID::BELLS,
            };
            expectEquals (sids.size(), 11);
            for (int i = 0; i < sids.size(); ++i)
                for (int j = i + 1; j < sids.size(); ++j)
                    expect (sids[i] != sids[j],
                            "Duplicate SampleID: " + sids[i] + " == " + sids[j]);
        }

        // -----------------------------------------------------------------------
        beginTest ("All Category constants are non-empty");
        {
            const char* cats[] = {
                Category::LEADS,    Category::BRASS,  Category::ENSEMBLES,
                Category::STRINGS,  Category::PADS,   Category::CHORDS,
                Category::SYNTHS,   Category::ARPS,   Category::VOCALS,
                Category::BELLS,
            };
            for (const char* c : cats)
                expect (c != nullptr && juce::String (c).isNotEmpty(),
                        "Category constant must not be empty");
        }

        beginTest ("Category constant values match spec");
        {
            expectEquals (juce::String (Category::LEADS),     juce::String ("Leads"));
            expectEquals (juce::String (Category::BRASS),     juce::String ("Brass"));
            expectEquals (juce::String (Category::ENSEMBLES), juce::String ("Ensembles"));
            expectEquals (juce::String (Category::STRINGS),   juce::String ("Strings"));
            expectEquals (juce::String (Category::PADS),      juce::String ("Pads"));
            expectEquals (juce::String (Category::CHORDS),    juce::String ("Chords"));
            expectEquals (juce::String (Category::SYNTHS),    juce::String ("Synths"));
            expectEquals (juce::String (Category::ARPS),      juce::String ("Arps"));
            expectEquals (juce::String (Category::VOCALS),    juce::String ("Vocals"));
            expectEquals (juce::String (Category::BELLS),     juce::String ("Bells"));
        }

        beginTest ("No duplicate Category values");
        {
            juce::StringArray cats {
                Category::LEADS, Category::BRASS, Category::ENSEMBLES,
                Category::STRINGS, Category::PADS, Category::CHORDS,
                Category::SYNTHS, Category::ARPS, Category::VOCALS,
                Category::BELLS,
            };
            expectEquals (cats.size(), 10);
            for (int i = 0; i < cats.size(); ++i)
                for (int j = i + 1; j < cats.size(); ++j)
                    expect (cats[i] != cats[j],
                            "Duplicate Category: " + cats[i] + " == " + cats[j]);
        }

        // -----------------------------------------------------------------------
        beginTest ("PresetKey constants are non-empty");
        {
            expect (juce::String (PresetKey::CATEGORY).isNotEmpty());
            expect (juce::String (PresetKey::NAME).isNotEmpty());
            expect (juce::String (PresetKey::AUTHOR).isNotEmpty());
            expect (juce::String (PresetKey::SCHEMA_VER).isNotEmpty());
            expect (juce::String (PresetKey::SAMPLE_ID).isNotEmpty());
        }
    }
};

static StateSchemaConstantTests stateSchemaTests;
