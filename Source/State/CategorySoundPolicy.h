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
    SamplePlaybackMode     defaultPlaybackMode { SamplePlaybackMode::PhraseTimeStretch };
    bool                   keytrack { true };
};

/** MIDI pitch tracking is on for every mode except SLICE, where the key picks
    the slice rather than the note. Phrase content transposes through the
    stretcher at constant length; one-shots repitch by varispeed. */
inline bool keytrackFor (SamplePlaybackMode mode) noexcept
{
    return mode != SamplePlaybackMode::SlicePhrase;
}

/** PHRASES is the one tab that runs through the stretcher; everything else is
    a chromatic instrument tab, so SPEED / host sync never changes key there. */
inline bool isPhraseCategory (const juce::String& category) noexcept
{
    return category.equalsIgnoreCase (Category::PHRASES);
}

inline bool isChromaticCategory (const juce::String& category) noexcept
{
    return ! isPhraseCategory (category);
}

/** BASS plays monophonically: a new note cuts the one before it so overlapping
    tails never stack sub energy into a muddy low end. */
inline bool isBassCategory (const juce::String& category) noexcept
{
    return category.equalsIgnoreCase (Category::BASS);
}

/** Categories renamed after release map to their current tab so old user
    presets and saved sessions still resolve. Chords became Phrases. */
inline juce::String normaliseCategory (const juce::String& category)
{
    if (category.equalsIgnoreCase ("Chords"))
        return Category::PHRASES;
    return category;
}

/** Browser tab order. PHRASES is the one stretcher tab; everything around it
    is a chromatic instrument tab. */
inline juce::StringArray getCanonicalCategories()
{
    return {
        Category::BASS,
        Category::LEADS,
        Category::KEYS,
        Category::BRASS,
        Category::PHRASES,
        Category::ARPS,
        Category::SYNTHS,
        Category::BELLS,
        Category::STRINGS,
        Category::PLUCKS,
        Category::ENSEMBLES,
        Category::PADS,
        Category::VOCALS,
    };
}

inline CategoryPolicy getPolicyForCategory (const juce::String& category) noexcept
{
    CategoryPolicy p;
    p.name = category;

    if (! isPhraseCategory (category))
    {
        p.defaultSoundType = SoundType::OneShot;
        p.defaultPlaybackMode = SamplePlaybackMode::ChromaticResample;
        return p;
    }

    // PHRASES runs through the pitch-preserving stretcher so SPEED and host
    // sync change length, never key.
    p.defaultSoundType = SoundType::Phrase;
    p.defaultPlaybackMode = SamplePlaybackMode::PhraseTimeStretch;
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

    // Outside PHRASES every tab is a chromatic instrument: the keyboard
    // repitches the sample and SPEED is a plain varispeed control.
    if (! isPhraseCategory (category))
        return SamplePlaybackMode::ChromaticResample;

    // A one-shot filed under PHRASES (a chord hit, a stab) keeps its recorded
    // pitch and plays through.
    if (soundType == SoundType::OneShot)
        return SamplePlaybackMode::OneShotOriginal;

    // Phrases and loops: STRETCH keeps pitch fixed under SPEED / BPM sync and
    // makes keytrack a transpose at constant length. PHRASE (varispeed) stays
    // available as an explicit mode choice.
    return SamplePlaybackMode::PhraseTimeStretch;
}

inline LoopMode loopModeFor (const juce::String& category, SoundType soundType) noexcept
{
    if (soundType == SoundType::Loop)
        return LoopMode::Loop;

    // Fixed-pitch one-shots in PHRASES (chord hits, stabs) play through;
    // every chromatic tab gates like Brass.
    if (soundType == SoundType::OneShot && isPhraseCategory (category))
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
    // Keytrack is the runtime switch for MIDI pitch tracking. Every category
    // follows the keyboard; only SLICE turns it off (keys select slices).
    setBool (P::SRC_KEYTRACK, keytrackFor (mode));

    setChoice (P::SRC_LOOP_MODE, static_cast<float> (loopModeFor (category, soundType)));
    setBool (P::SRC_BPM_SYNC, soundType == SoundType::Loop
                              || (! isChromaticCategory (category)
                                  && soundType != SoundType::OneShot));

    // BASS loads monophonic (play mode 1) so each note chokes the previous one
    // through the engine's short crossfade — two overlapping sub tails read as
    // a mud build-up, never as a chord.
    //
    // Play mode is otherwise the player's choice, not the content's, so a
    // non-bass preset only undoes the mono this rule imposed: it clears Mono
    // back to Poly and leaves a deliberate Legato alone.
    if (auto* playMode = apvts.getRawParameterValue (P::VOICE_PLAY_MODE))
    {
        // Play Mode choice indices: 0 = Poly, 1 = Mono, 2 = Legato.
        constexpr int kPoly = 0, kMono = 1;

        if (isBassCategory (category))
            setChoice (P::VOICE_PLAY_MODE, static_cast<float> (kMono));
        else if (static_cast<int> (playMode->load()) == kMono)
            setChoice (P::VOICE_PLAY_MODE, static_cast<float> (kPoly));
    }
}

/** Write authoritative root into APVTS so host state matches the loaded sample. */
inline void syncRootNoteToApvts (juce::AudioProcessorValueTreeState& apvts, int rootNote) noexcept
{
    if (auto* param = apvts.getParameter (ParamID::SRC_ROOT_NOTE))
        if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (param))
            param->setValueNotifyingHost (
                ranged->convertTo0to1 (static_cast<float> (juce::jlimit (0, 127, rootNote))));
}

inline void syncOriginalBpmToApvts (juce::AudioProcessorValueTreeState& apvts, float bpm) noexcept
{
    if (auto* param = apvts.getParameter (ParamID::SRC_ORIGINAL_BPM))
        if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (param))
            param->setValueNotifyingHost (
                ranged->convertTo0to1 (juce::jlimit (40.f, 240.f, bpm)));
}

/** Infer tempo from stems like `_90_`, `_130_`, `98_indigo` — returns 0 if unknown. */
inline float inferOriginalBpmFromStem (const juce::String& presetName,
                                       const juce::String& sampleId) noexcept
{
    const auto hay = (presetName + "_" + sampleId).toUpperCase();
    const int len = hay.length();

    for (int i = 0; i < len; ++i)
    {
        if (i > 0)
        {
            const auto prev = hay[i - 1];
            if (prev != '_' && prev != '-' && prev != ' ')
                continue;
        }

        if (! juce::CharacterFunctions::isDigit (hay[i]))
            continue;

        int j = i;
        int value = 0;
        while (j < len && juce::CharacterFunctions::isDigit (hay[j]) && j - i < 3)
        {
            value = value * 10 + (hay[j] - '0');
            ++j;
        }

        if (j < len)
        {
            const auto after = hay[j];
            if (after != '_' && after != '-' && after != ' ')
                continue;
        }

        if (value >= 40 && value <= 240)
            return static_cast<float> (value);
    }

    return 0.f;
}

} // namespace AviatorKeyz
