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
        beginTest ("Canonical categories are the 13 browser tabs, in display order");
        {
            const juce::StringArray expected {
                "Bass", "Leads", "Keys", "Brass", "Phrases", "Arps", "Synths",
                "Bells", "Strings", "Plucks", "Ensembles", "Pads", "Vocals"
            };
            expectEquals (AviatorKeyz::getCanonicalCategories().size(), expected.size());
            expect (AviatorKeyz::getCanonicalCategories() == expected,
                    "tab order drifted from the browser's display order");
        }

        beginTest ("BASS is the only self-choking tab");
        {
            expect (AviatorKeyz::isBassCategory (AviatorKeyz::Category::BASS));
            for (const auto& cat : AviatorKeyz::getCanonicalCategories())
                if (cat != AviatorKeyz::Category::BASS)
                    expect (! AviatorKeyz::isBassCategory (cat),
                            cat + juce::String (" should not self-choke"));
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

        beginTest ("PHRASES is the only STRETCH tab (speed never changes key there)");
        {
            const auto phrases = AviatorKeyz::getPolicyForCategory (AviatorKeyz::Category::PHRASES);
            expect (phrases.defaultPlaybackMode == SamplePlaybackMode::PhraseTimeStretch,
                    "Phrases should default to STRETCH");
            expect (phrases.keytrack, "Phrases should keytrack");
            expect (AviatorKeyz::isPhraseCategory (AviatorKeyz::Category::PHRASES));
        }

        beginTest ("Every other tab defaults to chromatic");
        {
            for (const auto& cat : AviatorKeyz::getCanonicalCategories())
            {
                if (AviatorKeyz::isPhraseCategory (cat))
                    continue;
                const auto policy = AviatorKeyz::getPolicyForCategory (cat);
                expect (policy.defaultPlaybackMode == SamplePlaybackMode::ChromaticResample,
                        cat + juce::String (" should default to chromatic"));
                expect (policy.keytrack, cat + juce::String (" should keytrack"));
            }
        }

        beginTest ("Legacy category names map to their current tab");
        {
            expectEquals (AviatorKeyz::normaliseCategory ("Chords"), juce::String ("Phrases"));
            expectEquals (AviatorKeyz::normaliseCategory ("Leads"), juce::String ("Leads"));
        }

        beginTest ("Keytrack is on for every mode except SLICE");
        {
            expect (AviatorKeyz::keytrackFor (SamplePlaybackMode::OneShotOriginal));
            expect (AviatorKeyz::keytrackFor (SamplePlaybackMode::PhraseOriginal));
            expect (AviatorKeyz::keytrackFor (SamplePlaybackMode::ChromaticResample));
            expect (AviatorKeyz::keytrackFor (SamplePlaybackMode::PhraseTimeStretch));
            expect (! AviatorKeyz::keytrackFor (SamplePlaybackMode::SlicePhrase));
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
            expect (AviatorKeyz::playbackModeFor ("Ensembles", AviatorKeyz::SoundType::OneShot)
                        == SamplePlaybackMode::ChromaticResample);
            expect (AviatorKeyz::playbackModeFor ("Phrases", AviatorKeyz::SoundType::OneShot)
                        == SamplePlaybackMode::OneShotOriginal);
            expect (AviatorKeyz::playbackModeFor ("Phrases", AviatorKeyz::SoundType::Phrase)
                        == SamplePlaybackMode::PhraseTimeStretch);
            expect (AviatorKeyz::playbackModeFor ("Phrases", AviatorKeyz::SoundType::Loop)
                        == SamplePlaybackMode::PhraseTimeStretch);
            // Outside PHRASES a phrase or loop is repitched by the keyboard.
            expect (AviatorKeyz::playbackModeFor ("Vocals", AviatorKeyz::SoundType::Phrase)
                        == SamplePlaybackMode::ChromaticResample);
            expect (AviatorKeyz::playbackModeFor ("Ensembles", AviatorKeyz::SoundType::Phrase)
                        == SamplePlaybackMode::ChromaticResample);
            expect (AviatorKeyz::playbackModeFor ("Arps", AviatorKeyz::SoundType::Loop)
                        == SamplePlaybackMode::ChromaticResample);
            expect (AviatorKeyz::playbackModeFor ("Vocals", AviatorKeyz::SoundType::Slice)
                        == SamplePlaybackMode::SlicePhrase);
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
