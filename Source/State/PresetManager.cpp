#include "PresetManager.h"
#include "ApvtsStateHelpers.h"
#include "FactoryResources.h"
#include <vector>

PresetManager::PresetManager (juce::AudioProcessorValueTreeState& apvtsRef)
    : apvts (apvtsRef)
    , currentPresetName ("Init")
    , currentCategory (AviatorKeyz::Category::LEADS)
{
}

PresetManager::~PresetManager() = default;

namespace
{
int noteLetterToMidi (juce::juce_wchar letter, int octave) noexcept
{
    const auto u = juce::CharacterFunctions::toUpperCase (letter);
    int semitone = 0;
    switch (u)
    {
        case 'C': semitone = 0; break;
        case 'D': semitone = 2; break;
        case 'E': semitone = 4; break;
        case 'F': semitone = 5; break;
        case 'G': semitone = 7; break;
        case 'A': semitone = 9; break;
        case 'B': semitone = 11; break;
        default: return 60;
    }
    return juce::jlimit (0, 127, (octave + 1) * 12 + semitone);
}

constexpr const char* kFxPresetParamIds[] {
    AviatorKeyz::ParamID::REVERB_AMOUNT,
    AviatorKeyz::ParamID::REVERB_SIZE,
    AviatorKeyz::ParamID::FX_REVERB_ON,
    AviatorKeyz::ParamID::FX_REVERB_DAMP,
    AviatorKeyz::ParamID::FX_DELAY_ON,
    AviatorKeyz::ParamID::FX_DELAY_TIME,
    AviatorKeyz::ParamID::FX_DELAY_FEEDBACK,
    AviatorKeyz::ParamID::FX_DELAY_MIX,
    AviatorKeyz::ParamID::FX_DELAY_SYNC,
    AviatorKeyz::ParamID::FX_CHORUS_ON,
    AviatorKeyz::ParamID::FX_CHORUS_RATE,
    AviatorKeyz::ParamID::FX_CHORUS_DEPTH,
    AviatorKeyz::ParamID::FX_CHORUS_MIX,
    AviatorKeyz::ParamID::FX_LOFI_ON,
    AviatorKeyz::ParamID::FX_LOFI_AMOUNT,
    AviatorKeyz::ParamID::FX_DIST_ON,
    AviatorKeyz::ParamID::FX_DIST_DRIVE,
    nullptr
};

using FxSnapshot = std::vector<std::pair<juce::String, float>>;

FxSnapshot captureFxParams (juce::AudioProcessorValueTreeState& apvts)
{
    FxSnapshot snap;
    for (auto* id = kFxPresetParamIds; *id != nullptr; ++id)
        if (auto* p = apvts.getParameter (*id))
            snap.emplace_back (*id, p->getValue());
    return snap;
}

void restoreFxParams (juce::AudioProcessorValueTreeState& apvts, const FxSnapshot& snap)
{
    for (const auto& [id, val] : snap)
        if (auto* p = apvts.getParameter (id))
            p->setValueNotifyingHost (val);
}

bool fxEditsEnabledInTree (const juce::ValueTree& state)
{
    if (state.hasProperty (AviatorKeyz::ParamID::FX_EDITS_ON))
        return (float) state.getProperty (AviatorKeyz::ParamID::FX_EDITS_ON) > 0.5f;
    return true;
}

void stripFxFromTree (juce::ValueTree& state)
{
    for (auto* id = kFxPresetParamIds; *id != nullptr; ++id)
        state.removeProperty (*id, nullptr);
}
} // namespace

int PresetManager::parseRootNoteAttribute (const juce::XmlElement* presetRoot)
{
    if (presetRoot == nullptr)
        return 60;

    if (presetRoot->hasAttribute (AviatorKeyz::PresetKey::ROOT_NOTE))
        return juce::jlimit (0, 127, presetRoot->getIntAttribute (AviatorKeyz::PresetKey::ROOT_NOTE, 60));

    return 60;
}

int PresetManager::inferRootNoteFromPresetName (const juce::String& presetName,
                                                const juce::String& sampleId)
{
    // Mirror of Python infer_root_note_midi — must stay in sync with
    // scripts/generate_factory_assets.py::infer_root_note_midi.
    //
    // Strategy:
    //   1. Explicit note+octave preceded by _ or -: _C3, _A#4, -G2
    //      Only match after a separator to avoid false positives from model
    //      numbers embedded in filenames (e.g. 'sb2' → 'B2', 'add11' → 'D1').
    //   2. Note-only token after separator: _C, _Cm, _Cmin → defaults to octave 4.
    //   3. Fallback: 60 (C4).

    const auto hay = (presetName + " " + sampleId).toUpperCase();
    const int len  = hay.length();

    // -----------------------------------------------------------------------
    // Pass 1: separator + note letter + optional sharp + digit
    // -----------------------------------------------------------------------
    for (int i = 1; i < len; ++i)
    {
        const auto prev = hay[i - 1];
        if (prev != '_' && prev != '-' && prev != ' ')
            continue;

        const auto ch = hay[i];
        if (ch != 'C' && ch != 'D' && ch != 'E' && ch != 'F'
            && ch != 'G' && ch != 'A' && ch != 'B')
            continue;

        bool sharp = false;
        int digitIdx = i + 1;
        if (digitIdx < len && hay[digitIdx] == '#')
        {
            sharp = true;
            ++digitIdx;
        }

        if (digitIdx < len && juce::CharacterFunctions::isDigit (hay[digitIdx]))
        {
            // Require the token to end with separator, space, or end-of-string
            const int afterDigit = digitIdx + 1;
            if (afterDigit < len)
            {
                const auto after = hay[afterDigit];
                if (after != '_' && after != '-' && after != ' ')
                    continue;  // digit is part of a longer token — skip
            }

            const int octave = hay[digitIdx] - '0';
            int midi = noteLetterToMidi (ch, octave);
            if (sharp)
                midi = juce::jmin (127, midi + 1);
            return midi;
        }
    }

    // -----------------------------------------------------------------------
    // Pass 2: separator + note letter + optional mode suffix (no octave digit)
    // -----------------------------------------------------------------------
    for (int i = 1; i < len; ++i)
    {
        const auto prev = hay[i - 1];
        if (prev != '_' && prev != '-' && prev != ' ')
            continue;

        const auto ch = hay[i];
        if (ch != 'C' && ch != 'D' && ch != 'E' && ch != 'F'
            && ch != 'G' && ch != 'A' && ch != 'B')
            continue;

        // Check the character after the note letter isn't a digit (that would
        // be caught by Pass 1 — or it's a false positive we want to skip)
        const int nextIdx = i + 1;
        if (nextIdx < len && juce::CharacterFunctions::isDigit (hay[nextIdx]))
            continue;

        // Accept _C, _Cm, _CM, _Cmin, _Cmaj, _C#, etc.
        return noteLetterToMidi (ch, 4);  // octave 4 → C4 = 60
    }

    return 60;
}

juce::StringArray PresetManager::getAllCategories() const
{
    return {
        AviatorKeyz::Category::LEADS,
        AviatorKeyz::Category::BRASS,
        AviatorKeyz::Category::ENSEMBLES,
        AviatorKeyz::Category::STRINGS,
        AviatorKeyz::Category::PADS,
        AviatorKeyz::Category::CHORDS,
        AviatorKeyz::Category::SYNTHS,
        AviatorKeyz::Category::ARPS,
        AviatorKeyz::Category::VOCALS,
        AviatorKeyz::Category::BELLS
    };
}

juce::StringArray PresetManager::getPresetsForCategory (const juce::String& category) const
{
    auto names = FactoryResources::getPresetNamesForCategory (category);

    const auto dir = getUserPresetsDir().getChildFile (category);
    if (dir.isDirectory())
    {
        juce::Array<juce::File> files;
        dir.findChildFiles (files, juce::File::findFiles, false, "*.xml");
        for (const auto& f : files)
        {
            const auto n = f.getFileNameWithoutExtension();
            if (! names.contains (n, true))
                names.add (n);
        }
    }

    return names;
}

bool PresetManager::loadPreset (const juce::String& category, const juce::String& name)
{
    std::unique_ptr<juce::XmlElement> parsed;
    juce::String sampleId = AviatorKeyz::SampleID::DEFAULT;

    if (const auto* factory = [&]() -> const FactoryResources::PresetEntry* {
            for (const auto& e : FactoryResources::getFactoryPresets())
            {
                if (e.category.equalsIgnoreCase (category) && e.name.equalsIgnoreCase (name))
                    return &e;
            }
            return nullptr;
        }())
    {
        parsed = juce::XmlDocument::parse (juce::String::fromUTF8 (factory->xmlData, factory->xmlSize));
        sampleId = factory->sampleId;
    }
    else
    {
        const auto file = getUserPresetsDir().getChildFile (category).getChildFile (name + ".xml");
        if (! file.existsAsFile())
            return false;
        parsed = juce::XmlDocument::parse (file);
        if (parsed != nullptr && parsed->hasTagName ("Preset"))
            sampleId = parsed->getStringAttribute (AviatorKeyz::PresetKey::SAMPLE_ID,
                                                 AviatorKeyz::SampleID::DEFAULT);
    }

    if (parsed == nullptr)
        return false;

    juce::XmlElement* stateEl = nullptr;

    int rootNote = 60;

    if (parsed->hasTagName ("Preset"))
    {
        if (sampleId.isEmpty())
            sampleId = parsed->getStringAttribute (AviatorKeyz::PresetKey::SAMPLE_ID,
                                                   AviatorKeyz::SampleID::DEFAULT);
        rootNote = parseRootNoteAttribute (parsed.get());
        if (! parsed->hasAttribute (AviatorKeyz::PresetKey::ROOT_NOTE))
            rootNote = inferRootNoteFromPresetName (name, sampleId);
        stateEl = parsed->getChildByName ("AviatorKeyzState");
    }
    else
        stateEl = parsed->getChildByName ("AviatorKeyzState");

    if (stateEl == nullptr && parsed->hasTagName ("AviatorKeyzState"))
        stateEl = parsed.get();

    if (stateEl == nullptr)
        return false;

    auto state = juce::ValueTree::fromXml (*stateEl);

    if (! state.isValid())
        return false;

    const bool recallFx = fxEditsEnabledInTree (state);
    const auto fxSnap   = recallFx ? FxSnapshot {} : captureFxParams (apvts);

    AviatorKeyz::applyStateTreeToApvts (apvts, state);

    if (! recallFx)
        restoreFxParams (apvts, fxSnap);
    currentCategory = category;
    currentPresetName = name;
    currentSampleId = sampleId;
    currentRootNote = rootNote;

    if (onPresetLoaded)
        onPresetLoaded (category, name, sampleId, rootNote);

    return true;
}

bool PresetManager::saveUserPreset (const juce::String& category, const juce::String& name)
{
    auto dir = getUserPresetsDir().getChildFile (category);
    if (! dir.createDirectory())
        return false;

    auto state = apvts.copyState();
    state.setProperty ("stateVersion", AviatorKeyz::STATE_SCHEMA_VERSION, nullptr);

    if (const auto* fxEdits = apvts.getRawParameterValue (AviatorKeyz::ParamID::FX_EDITS_ON))
    {
        if (fxEdits->load() <= 0.5f)
            stripFxFromTree (state);
    }

    juce::XmlElement preset ("Preset");
    preset.setAttribute (AviatorKeyz::PresetKey::CATEGORY, category);
    preset.setAttribute (AviatorKeyz::PresetKey::NAME, name);
    preset.setAttribute (AviatorKeyz::PresetKey::SCHEMA_VER, AviatorKeyz::STATE_SCHEMA_VERSION);
    preset.setAttribute (AviatorKeyz::PresetKey::SAMPLE_ID,
                          currentSampleId.isNotEmpty() ? currentSampleId
                                                       : juce::String (AviatorKeyz::SampleID::DEFAULT));

    if (auto inner = state.createXml())
        preset.addChildElement (inner.release());

    const auto file = dir.getChildFile (name + ".xml");
    return preset.writeTo (file);
}

juce::String PresetManager::getCurrentPresetName() const { return currentPresetName; }
juce::String PresetManager::getCurrentCategory() const { return currentCategory; }
juce::String PresetManager::getCurrentSampleId() const { return currentSampleId; }
int PresetManager::getCurrentRootNote() const { return currentRootNote; }

void PresetManager::setPresetIdentity (const juce::String& category,
                                         const juce::String& name,
                                         const juce::String& sampleId,
                                         int rootNote)
{
    if (category.isNotEmpty())
        currentCategory = category;
    if (name.isNotEmpty())
        currentPresetName = name;
    if (sampleId.isNotEmpty())
        currentSampleId = sampleId;
    currentRootNote = juce::jlimit (0, 127, rootNote);
}

juce::Array<PresetManager::FlatPreset> PresetManager::buildFlatPresetList() const
{
    juce::Array<FlatPreset> list;

    for (const auto& category : getAllCategories())
    {
        for (const auto& name : getPresetsForCategory (category))
            list.add ({ category, name });
    }

    return list;
}

int PresetManager::findCurrentFlatIndex (const juce::Array<FlatPreset>& list) const
{
    for (int i = 0; i < list.size(); ++i)
    {
        if (list[i].category.equalsIgnoreCase (currentCategory)
            && list[i].name.equalsIgnoreCase (currentPresetName))
            return i;
    }

    return 0;
}

int PresetManager::getTotalPresetCount() const
{
    return buildFlatPresetList().size();
}

int PresetManager::getCurrentPresetIndex() const
{
    const auto list = buildFlatPresetList();
    return findCurrentFlatIndex (list);
}

bool PresetManager::loadPresetByFlatIndex (int index)
{
    const auto list = buildFlatPresetList();
    if (list.isEmpty())
        return false;

    const int wrapped = ((index % list.size()) + list.size()) % list.size();
    return loadPreset (list[wrapped].category, list[wrapped].name);
}

bool PresetManager::loadAdjacentPreset (int delta)
{
    return loadPresetByFlatIndex (getCurrentPresetIndex() + delta);
}

int PresetManager::getCurrentPresetIndexInCategory() const
{
    const auto names = getPresetsForCategory (currentCategory);
    const int idx = names.indexOf (currentPresetName, false, 0);
    return idx >= 0 ? idx : 0;
}

bool PresetManager::loadAdjacentPresetInCategory (int delta)
{
    const auto names = getPresetsForCategory (currentCategory);
    if (names.isEmpty())
        return false;

    const int idx = getCurrentPresetIndexInCategory();
    const int wrapped = ((idx + delta) % names.size() + names.size()) % names.size();
    return loadPreset (currentCategory, names[wrapped]);
}

bool PresetManager::loadAdjacentCategory (int delta)
{
    const auto categories = getAllCategories();
    if (categories.isEmpty())
        return false;

    int idx = 0;
    for (int i = 0; i < categories.size(); ++i)
    {
        if (categories[i].equalsIgnoreCase (currentCategory))
        {
            idx = i;
            break;
        }
    }

    const int wrapped = ((idx + delta) % categories.size() + categories.size()) % categories.size();
    const auto& nextCat = categories[wrapped];
    const auto names = getPresetsForCategory (nextCat);
    if (names.isEmpty())
    {
        currentCategory = nextCat;
        return true;
    }

    const int presetIdx = juce::jmin (getCurrentPresetIndexInCategory(), names.size() - 1);
    return loadPreset (nextCat, names[juce::jmax (0, presetIdx)]);
}

juce::File PresetManager::getFactoryPresetsDir() const
{
    return {};
}

juce::File PresetManager::getUserPresetsDir() const
{
    return juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
        .getChildFile ("AviatorKeyz/Presets");
}
