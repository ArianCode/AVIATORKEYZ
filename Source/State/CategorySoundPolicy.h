#pragma once

#include "../DSP/Performance/PerformanceTypes.h"
#include "StateSchema.h"
#include <juce_audio_processors/juce_audio_processors.h>

namespace AviatorKeyz
{

enum class SoundType : int
{
    OneShot = 0,
    Phrase,
    Loop,
    Slice
};

struct CategoryPolicy
{
    juce::String           name;
    SoundType              defaultSoundType { SoundType::Phrase };
    SamplePlaybackMode     defaultPlaybackMode { SamplePlaybackMode::PhraseOriginal };
    bool                   keytrack { false };
};

inline bool isChromaticCategory (const juce::String& category) noexcept
{
    return category.equalsIgnoreCase (Category::LEADS)
           || category.equalsIgnoreCase (Category::BRASS)
           || category.equalsIgnoreCase (Category::STRINGS)
           || category.equalsIgnoreCase (Category::SYNTHS)
           || category.equalsIgnoreCase (Category::BELLS);
}

inline juce::StringArray getCanonicalCategories()
{
    return {
        Category::LEADS,
        Category::BRASS,
        Category::ENSEMBLES,
        Category::STRINGS,
        Category::PADS,
        Category::CHORDS,
        Category::SYNTHS,
        Category::ARPS,
        Category::VOCALS,
        Category::BELLS,
    };
}

inline CategoryPolicy getPolicyForCategory (const juce::String& category) noexcept
{
    CategoryPolicy p;
    p.name = category;

    if (isChromaticCategory (category))
    {
        p.defaultSoundType = SoundType::OneShot;
        p.defaultPlaybackMode = SamplePlaybackMode::ChromaticResample;
        return p;
    }

    p.defaultSoundType = SoundType::Phrase;
    p.defaultPlaybackMode = SamplePlaybackMode::PhraseOriginal;
    return p;
}

inline SamplePlaybackMode defaultPlaybackModeForCategory (const juce::String& category) noexcept
{
    return getPolicyForCategory (category).defaultPlaybackMode;
}

inline bool isSampleIdCompatibleWithCategory (const juce::String& sampleId,
                                              const juce::String& category) noexcept
{
    if (sampleId == SampleID::DEFAULT)
        return true;

    const auto prefix = "factory_" + category.toLowerCase() + "_";
    return sampleId.startsWithIgnoreCase (prefix);
}

inline SoundType soundTypeFromString (const juce::String& text) noexcept
{
    const auto t = text.trim().toLowerCase();
    if (t == "one_shot" || t == "oneshot") return SoundType::OneShot;
    if (t == "loop") return SoundType::Loop;
    if (t == "slice") return SoundType::Slice;
    return SoundType::Phrase;
}

inline juce::String soundTypeToString (SoundType type) noexcept
{
    switch (type)
    {
        case SoundType::OneShot: return "one_shot";
        case SoundType::Loop:    return "loop";
        case SoundType::Slice:   return "slice";
        case SoundType::Phrase:
        default:                 return "phrase";
    }
}

inline SoundType inferSoundTypeFromStem (const juce::String& category,
                                         const juce::String& displayName) noexcept
{
    const auto text = displayName.toLowerCase().replaceCharacters ("-", "_");

    const bool hasArp = text.contains ("arp") || text.contains ("arpeggio");
    const bool hasVocal = text.contains ("vocal") || text.contains ("vox");
    const bool hasLoop = text.contains ("loop") || text.contains ("chop_loop");
    const bool hasPhrase = text.contains ("phrase") || text.contains ("motif")
                           || text.contains ("riff");
    const bool hasOneShot = text.contains ("one_shot") || text.contains ("oneshot")
                            || text.contains ("stab") || text.contains ("hit");

    if (hasLoop && ! hasOneShot)
        return SoundType::Loop;
    if (hasArp || (hasPhrase && ! hasOneShot))
        return SoundType::Phrase;
    if (hasVocal && category.equalsIgnoreCase (Category::VOCALS))
        return SoundType::Phrase;
    if (hasOneShot)
        return SoundType::OneShot;
    if (isChromaticCategory (category))
        return SoundType::OneShot;

    return getPolicyForCategory (category).defaultSoundType;
}

inline SamplePlaybackMode playbackModeFor (const juce::String& category, SoundType soundType) noexcept
{
    if (soundType == SoundType::Slice)
        return SamplePlaybackMode::SlicePhrase;

    if (soundType == SoundType::OneShot)
        return isChromaticCategory (category) ? SamplePlaybackMode::ChromaticResample
                                              : SamplePlaybackMode::OneShotOriginal;

    if (soundType == SoundType::Loop)
        return SamplePlaybackMode::PhraseOriginal;

    return SamplePlaybackMode::PhraseOriginal;
}

inline LoopMode loopModeFor (const juce::String& category, SoundType soundType) noexcept
{
    if (soundType == SoundType::Loop)
        return LoopMode::Loop;

    if (soundType == SoundType::OneShot && ! isChromaticCategory (category))
        return LoopMode::OneShot;

    return LoopMode::Gate;
}

inline NoteGatePolicy gatePolicyFor (const juce::String& category,
                                     SoundType soundType,
                                     SamplePlaybackMode playbackMode,
                                     LoopMode loopMode) noexcept
{
    juce::ignoreUnused (category);

    if (playbackMode == SamplePlaybackMode::ChromaticResample)
        return NoteGatePolicy::Gated;

    if (soundType == SoundType::Phrase || soundType == SoundType::Loop || soundType == SoundType::Slice)
        return NoteGatePolicy::Gated;

    if (soundType == SoundType::OneShot && loopMode == LoopMode::OneShot)
        return NoteGatePolicy::TriggerToEnd;

    return NoteGatePolicy::Gated;
}

inline RetriggerPolicy retriggerPolicyFor (const juce::String& category,
                                           SoundType soundType,
                                           SamplePlaybackMode playbackMode) noexcept
{
    juce::ignoreUnused (category);

    if (playbackMode == SamplePlaybackMode::ChromaticResample)
        return RetriggerPolicy::Polyphonic;

    if (soundType == SoundType::Phrase || soundType == SoundType::Loop)
        return RetriggerPolicy::PhraseChoke;

    return RetriggerPolicy::Polyphonic;
}

inline void applyPlaybackPolicyToApvts (juce::AudioProcessorValueTreeState& apvts,
                                        const juce::String& category,
                                        SoundType soundType) noexcept
{
    namespace P = ParamID;

    auto setChoice = [&] (const char* id, float value)
    {
        if (auto* param = apvts.getParameter (id))
            if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (param))
                param->setValueNotifyingHost (ranged->convertTo0to1 (value));
    };

    auto setBool = [&] (const char* id, bool value)
    {
        if (auto* param = apvts.getParameter (id))
            param->setValueNotifyingHost (value ? 1.f : 0.f);
    };

    const auto mode = playbackModeFor (category, soundType);
    setChoice (P::SRC_PLAYBACK_MODE, static_cast<float> (mode));
    setBool (P::SRC_KEYTRACK, false);

    setChoice (P::SRC_LOOP_MODE, static_cast<float> (loopModeFor (category, soundType)));
    setBool (P::SRC_BPM_SYNC, soundType == SoundType::Loop
                              || (! isChromaticCategory (category)
                                  && soundType != SoundType::OneShot));
}

} // namespace AviatorKeyz
