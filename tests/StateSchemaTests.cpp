// =============================================================================
//  StateSchema compile-time and constant tests
//
//  Tests cover:
//    - STATE_SCHEMA_VERSION is 1
//    - All ParamID constants (v1 + v2 + v3) are non-empty strings
//    - No two ParamID constants are equal (global duplicate check)
//    - Expected string values for v1 params (regression guard)
//    - All SampleID constants are non-empty and follow naming convention
//    - All Category constants are non-empty with expected values
//    - PresetKey constants are non-empty
//    - MOD_MATRIX_ROWS == 8
// =============================================================================

#include <juce_core/juce_core.h>
#include "State/StateSchema.h"
#include "SchemaParamList.h"

class StateSchemaConstantTests : public juce::UnitTest
{
public:
    StateSchemaConstantTests() : juce::UnitTest ("StateSchema_Constants", "AviatorKeyz") {}

    void runTest() override
    {
        using namespace AviatorKeyz;

        // -------------------------------------------------------------------
        beginTest ("STATE_SCHEMA_VERSION == 1");
        expectEquals (STATE_SCHEMA_VERSION, 1);

        // -------------------------------------------------------------------
        beginTest ("MOD_MATRIX_ROWS == 8");
        expectEquals (ParamID::MOD_MATRIX_ROWS, 8);

        // -------------------------------------------------------------------
        beginTest ("All ParamID constants are non-empty strings");
        {
            const auto ids = allSchemaParamIDs();
            for (const auto& id : ids)
                expect (id.isNotEmpty(), "ParamID constant must not be empty");
        }

        // -------------------------------------------------------------------
        beginTest ("All ParamID constants use lower_snake_case");
        {
            const auto ids = allSchemaParamIDs();
            for (const auto& id : ids)
            {
                // must be all lowercase, digits, or underscores
                bool ok = true;
                for (auto ch : id)
                    if (! (juce::CharacterFunctions::isLowerCase (ch)
                           || ch == '_'
                           || juce::CharacterFunctions::isDigit (ch)))
                        ok = false;
                expect (ok, "ParamID not lower_snake_case: " + id);
            }
        }

        // -------------------------------------------------------------------
        beginTest ("No duplicate ParamID values (global)");
        {
            const auto ids = allSchemaParamIDs();
            // Expected count — update this when you add new params so the
            // test will catch an accidental omission as well as a collision.
            constexpr int kExpectedCount = kExpectedSchemaParamCount;
            expectEquals (ids.size(), kExpectedCount,
                          "ParamID count mismatch — did you add/remove one without updating this test?");

            for (int i = 0; i < ids.size(); ++i)
                for (int j = i + 1; j < ids.size(); ++j)
                    expect (ids[i] != ids[j],
                            "Duplicate ParamID: " + ids[i] + " == " + ids[j]);
        }

        // -------------------------------------------------------------------
        // Regression guard: v1 IDs must never be renamed.
        beginTest ("v1 ParamID string values are locked");
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

        // Spot-check a selection of v2/v3 IDs to guard against typos.
        beginTest ("v2/v3 ParamID string values spot-check");
        {
            expectEquals (juce::String (ParamID::LFO1_RATE),          juce::String ("lfo1_rate"));
            expectEquals (juce::String (ParamID::LFO1_PHASE),         juce::String ("lfo1_phase"));
            expectEquals (juce::String (ParamID::SOURCE_BLEND),       juce::String ("source_blend"));
            expectEquals (juce::String (ParamID::FILTER_CUTOFF),      juce::String ("filter_cutoff"));
            expectEquals (juce::String (ParamID::FILTER_RESONANCE),   juce::String ("filter_resonance"));
            expectEquals (juce::String (ParamID::FILTER_TYPE),        juce::String ("filter_type"));
            expectEquals (juce::String (ParamID::FILTER_DRIVE),       juce::String ("filter_drive"));
            expectEquals (juce::String (ParamID::ENV_AMP_DECAY),      juce::String ("env_amp_decay"));
            expectEquals (juce::String (ParamID::ENV_AMP_SUSTAIN),    juce::String ("env_amp_sustain"));
            expectEquals (juce::String (ParamID::ENV_FLT_ATTACK),     juce::String ("env_flt_attack"));
            expectEquals (juce::String (ParamID::ENV_FLT_AMOUNT),     juce::String ("env_flt_amount"));
            expectEquals (juce::String (ParamID::TEX_ENABLED),        juce::String ("tex_enabled"));
            expectEquals (juce::String (ParamID::TEX_GRAIN_SIZE),     juce::String ("tex_grain_size"));
            expectEquals (juce::String (ParamID::TEX_GRAIN_DENSITY),  juce::String ("tex_grain_density"));
            expectEquals (juce::String (ParamID::TEX_FREEZE),         juce::String ("tex_freeze"));
            expectEquals (juce::String (ParamID::PHRASE_ENABLED),     juce::String ("phrase_enabled"));
            expectEquals (juce::String (ParamID::PHRASE_TEMPO_SYNC),  juce::String ("phrase_tempo_sync"));
            expectEquals (juce::String (ParamID::FX_REVERB_ON),       juce::String ("fx_reverb_on"));
            expectEquals (juce::String (ParamID::FX_REVERB_DAMP),     juce::String ("fx_reverb_damp"));
            expectEquals (juce::String (ParamID::FX_EDITS_ON),        juce::String ("fx_edits_on"));
            expectEquals (juce::String (ParamID::PERF_MACRO_1),       juce::String ("perf_macro_1"));
            expectEquals (juce::String (ParamID::PERF_MACRO_4),       juce::String ("perf_macro_4"));
            expectEquals (juce::String (ParamID::MOD0_ON),            juce::String ("mod_0_on"));
            expectEquals (juce::String (ParamID::MOD7_AMOUNT),        juce::String ("mod_7_amount"));
            expectEquals (juce::String (ParamID::OSC1_TYPE),          juce::String ("osc1_type"));
            expectEquals (juce::String (ParamID::OSC2_PAN),           juce::String ("osc2_pan"));
            expectEquals (juce::String (ParamID::VOICE_POLYPHONY),    juce::String ("voice_polyphony"));
            expectEquals (juce::String (ParamID::OUTPUT_LIMITER),     juce::String ("output_limiter"));
            expectEquals (juce::String (ParamID::FX_DELAY_SYNC),      juce::String ("fx_delay_sync"));
            expectEquals (juce::String (ParamID::FX_DIST_DRIVE),      juce::String ("fx_dist_drive"));
        }

        // -------------------------------------------------------------------
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

        // -------------------------------------------------------------------
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

        // -------------------------------------------------------------------
        beginTest ("PresetKey constants are non-empty");
        {
            expect (juce::String (PresetKey::CATEGORY).isNotEmpty());
            expect (juce::String (PresetKey::NAME).isNotEmpty());
            expect (juce::String (PresetKey::AUTHOR).isNotEmpty());
            expect (juce::String (PresetKey::SCHEMA_VER).isNotEmpty());
            expect (juce::String (PresetKey::SAMPLE_ID).isNotEmpty());
            expect (juce::String (PresetKey::ROOT_NOTE).isNotEmpty());
        }
    }
};

static StateSchemaConstantTests stateSchemaTests;
