// =============================================================================
//  CategorySoundPolicy unit tests
// =============================================================================

#include <juce_core/juce_core.h>
#include "State/CategorySoundPolicy.h"

class CategoryPolicyTests : public juce::UnitTest
{
public:
    CategoryPolicyTests() : juce::UnitTest ("CategoryPolicy", "AviatorKeyz") {}

    void runTest() override
    {
        beginTest ("Canonical categories count is 10");
        {
            expectEquals (AviatorKeyz::getCanonicalCategories().size(), 10);
        }

        beginTest ("Chromatic categories use ChromaticResample");
        {
            for (const auto& cat : { AviatorKeyz::Category::LEADS,
                                     AviatorKeyz::Category::BRASS,
                                     AviatorKeyz::Category::STRINGS })
            {
                const auto policy = AviatorKeyz::getPolicyForCategory (cat);
                expect (policy.defaultPlaybackMode == SamplePlaybackMode::ChromaticResample,
                        cat + juce::String (" should be chromatic"));
            }
        }

        beginTest ("Phrase categories use PhraseOriginal default");
        {
            const auto policy = AviatorKeyz::getPolicyForCategory (AviatorKeyz::Category::VOCALS);
            expect (policy.defaultPlaybackMode == SamplePlaybackMode::PhraseOriginal);
        }

        beginTest ("sampleId prefix compatibility");
        {
            expect (AviatorKeyz::isSampleIdCompatibleWithCategory ("factory_leads_init", "Leads"));
            expect (! AviatorKeyz::isSampleIdCompatibleWithCategory ("factory_vocals_test", "Leads"));
            expect (AviatorKeyz::isSampleIdCompatibleWithCategory ("factory_default", "Leads"));
        }

        beginTest ("inferSoundTypeFromStem classifies one-shots and phrases");
        {
            expect (AviatorKeyz::inferSoundTypeFromStem ("Leads", "BOS_AA_Synth_One_Shot_Shadows_C")
                        == AviatorKeyz::SoundType::OneShot);
            expect (AviatorKeyz::inferSoundTypeFromStem ("Vocals", "VOX_DCV_85_vocal_adlib_zephyr_wet_Cmin")
                        == AviatorKeyz::SoundType::Phrase);
            expect (AviatorKeyz::inferSoundTypeFromStem ("Arps", "RKU_SRNB_71_synth_arp_loop_delay_thief_wet_Cmin")
                        == AviatorKeyz::SoundType::Loop);
        }

        beginTest ("playbackModeFor maps sound types");
        {
            expect (AviatorKeyz::playbackModeFor ("Leads", AviatorKeyz::SoundType::OneShot)
                        == SamplePlaybackMode::ChromaticResample);
            expect (AviatorKeyz::playbackModeFor ("Chords", AviatorKeyz::SoundType::OneShot)
                        == SamplePlaybackMode::OneShotOriginal);
            expect (AviatorKeyz::playbackModeFor ("Vocals", AviatorKeyz::SoundType::Phrase)
                        == SamplePlaybackMode::PhraseOriginal);
        }

        beginTest ("inferOriginalBpmFromStem finds tempo tokens");
        {
            expectWithinAbsoluteError (
                AviatorKeyz::inferOriginalBpmFromStem ("SO_MTS_98_indigo_C", "factory_strings_so_mts_98_indigo_c"),
                98.f, 0.01f);
            expectWithinAbsoluteError (
                AviatorKeyz::inferOriginalBpmFromStem ("VOX_DCV_85_vocal_adlib", "x"),
                85.f, 0.01f);
            expectWithinAbsoluteError (
                AviatorKeyz::inferOriginalBpmFromStem ("no_tempo_here_C", "factory_leads_x"),
                0.f, 0.01f);
        }
    }
};

static CategoryPolicyTests categoryPolicyTests;
