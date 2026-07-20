#include "AviationMainView.h"
#include "AviationIcons.h"
#include "AviationTheme.h"
#include "../../PluginProcessor.h"
#include "../../State/FactoryResources.h"
#include "../../State/StateSchema.h"
#include "../Cockpit/PresetDisplayUtils.h"

namespace
{
/** Parses a musical key from preset name tokens ("...anthem_Cmin" -> "C min"). */
juce::String parseKeyFromPresetName (const juce::String& presetName, int fallbackRootNote)
{
    auto tokens = juce::StringArray::fromTokens (presetName, "_", "");
    for (int i = tokens.size(); --i >= 0;)
    {
        auto t = tokens[i].trim();
        if (t.isEmpty() || t.length() > 5)
            continue;

        const juce::juce_wchar note = (juce::juce_wchar) juce::CharacterFunctions::toUpperCase (t[0]);
        if (note < 'A' || note > 'G')
            continue;

        juce::String rest = t.substring (1);
        juce::String accidental;
        if (rest.startsWithChar ('#') || rest.startsWithIgnoreCase ("s"))
        {
            accidental = "#";
            rest = rest.substring (1);
        }
        else if (rest.startsWithChar ('b') && rest.length() > 1) // "b" alone would be ambiguous
        {
            accidental = "b";
            rest = rest.substring (1);
        }

        const auto mode = rest.toLowerCase();
        if (mode.isEmpty())
            return juce::String::charToString (note) + accidental;
        if (mode == "m" || mode == "min" || mode == "mi")
            return juce::String::charToString (note) + accidental + " min";
        if (mode == "maj" || mode == "ma")
            return juce::String::charToString (note) + accidental + " maj";
    }

    static const char* names[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    return names[juce::jlimit (0, 127, fallbackRootNote) % 12];
}

juce::String prettySourceName (const juce::String& sampleId)
{
    auto pretty = PresetDisplayUtils::shortenDisplayName (
        sampleId.fromFirstOccurrenceOf ("factory_", false, true));
    return pretty.isNotEmpty() ? pretty : sampleId;
}
} // namespace

AviationMainView::AviationMainView (AviatorKeyzProcessor& p)
    : processor (p)
{
    setOpaque (true);

    auto& apvts = processor.getAPVTS();

    addAndMakeVisible (cockpitBackground);

    dashboard = std::make_unique<CenterDashboard> (apvts);
    dashboard->bpmProvider = [this] { return processor.getLastKnownHostBpm(); };
    addAndMakeVisible (*dashboard);

    velocityPanel = std::make_unique<VelocityPanel> (apvts);
    addAndMakeVisible (*velocityPanel);
    layerMixPanel = std::make_unique<LayerMixPanel> (apvts);
    addAndMakeVisible (*layerMixPanel);
    filterPanel = std::make_unique<FilterPanel> (apvts);
    addAndMakeVisible (*filterPanel);
    envelopePanel = std::make_unique<EnvelopePanel> (apvts);
    addAndMakeVisible (*envelopePanel);

    addAndMakeVisible (sourceDropdown);
    sourceDropdown.onClicked = [this] { openSourceMenu(); };

    addAndMakeVisible (topHeader);
    topHeader.onModeChanged = [this] (bool performance) { if (onModeChanged) onModeChanged (performance); };
    topHeader.onSaveClicked = [this] {
        auto& pm = processor.getPresetManager();
        pm.saveUserPreset (pm.getCurrentCategory(), pm.getCurrentPresetName());
    };
    topHeader.onSettingsClicked = [this] {
        if (libraryOverlay == nullptr)
        {
            libraryOverlay = std::make_unique<PresetLibraryOverlay> (processor);
            addChildComponent (*libraryOverlay);
        }
        libraryOverlay->setBounds (getLocalBounds());
        libraryOverlay->showOverlay();
        libraryOverlay->toFront (true);
    };
    topHeader.onUtilityClicked = [this] {
        if (aboutOverlay == nullptr)
        {
            aboutOverlay = std::make_unique<AboutOverlay>();
            addChildComponent (*aboutOverlay);
        }
        aboutOverlay->setBounds (getLocalBounds());
        aboutOverlay->showOverlay();
        aboutOverlay->toFront (true);
    };

    addAndMakeVisible (presetHeader);
    presetHeader.onPrevPreset = [this] { processor.getPresetManager().loadAdjacentPresetInCategory (-1); };
    presetHeader.onNextPreset = [this] { processor.getPresetManager().loadAdjacentPresetInCategory (+1); };
    presetHeader.onFavoriteToggled = [this] (bool fav) {
        auto& pm = processor.getPresetManager();
        setFavourited (pm.getCurrentCategory(), pm.getCurrentPresetName(), fav);
    };

    addAndMakeVisible (categoryTabs);
    categoryTabs.setVersionText ("v" + juce::String (JucePlugin_VersionString));
    categoryTabs.onCategorySelected = [this] (const juce::String& cat) { selectCategory (cat); };

    addAndMakeVisible (presetBrowser);
    presetBrowser.onPresetChosen = [this] (const juce::String& name) {
        auto& pm = processor.getPresetManager();
        pm.loadPreset (pm.getCurrentCategory(), name);
    };
    presetBrowser.isFavourited = [this] (const juce::String& name) {
        return isFavourited (processor.getPresetManager().getCurrentCategory(), name);
    };
    presetBrowser.onFavouriteToggled = [this] (const juce::String& name, bool fav) {
        setFavourited (processor.getPresetManager().getCurrentCategory(), name, fav);
    };

    macroDeck = std::make_unique<MacroDeck> (apvts);
    addAndMakeVisible (*macroDeck);

    addAndMakeVisible (statusBar);
    statusBar.setVersionText ("v" + juce::String (JucePlugin_VersionString));
    statusBar.sampleRateProvider = [this] { return processor.getSampleRate(); };
    statusBar.bpmProvider = [this] { return processor.getLastKnownHostBpm(); };
    statusBar.activeProvider = [this] { return processor.getSampleRate() > 0.0; };
    statusBar.onAbClicked = [this] { openAbMenu(); };

    loadFavourites();

    // Chain onto the preset-loaded pipeline (sample loading stays first).
    auto& pm = processor.getPresetManager();
    previousPresetLoadedHandler = pm.onPresetLoaded;
    pm.onPresetLoaded = [this, loadSampleHook = previousPresetLoadedHandler] (const juce::String& category,
                                                                              const juce::String& name,
                                                                              const juce::String& sampleId,
                                                                              int rootNote)
    {
        // Break out of ListBox/mouse stack before heavy work and UI refresh.
        juce::Timer::callAfterDelay (0, [safe = juce::Component::SafePointer<AviationMainView> (this),
                                         loadSampleHook, category, name, sampleId, rootNote]
        {
            if (safe == nullptr)
                return;

            if (loadSampleHook)
                loadSampleHook (category, name, sampleId, rootNote);

            safe->requestPresetUiRefresh();
        });
    };

    categoryTabs.setCategories (pm.getAllCategories());

    // Children exist now — safe to trigger the initial resized() pass.
    setSize (Aviation::kDesignW, Aviation::kDesignH);
    refreshPresetUI();
}

AviationMainView::~AviationMainView()
{
    auto& pm = processor.getPresetManager();
    if (pm.onPresetLoaded)
        pm.onPresetLoaded = previousPresetLoadedHandler;
}

// -----------------------------------------------------------------------------
//  Favorites
// -----------------------------------------------------------------------------
juce::File AviationMainView::favouritesFile() const
{
    return juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
        .getChildFile ("AviatorKeyz")
        .getChildFile ("favorites.txt");
}

void AviationMainView::loadFavourites()
{
    favourites.clear();
    juce::StringArray lines;
    favouritesFile().readLines (lines);
    for (const auto& line : lines)
        if (line.isNotEmpty())
            favourites.insert (line.toStdString());
}

void AviationMainView::saveFavourites() const
{
    auto file = favouritesFile();
    file.getParentDirectory().createDirectory();
    juce::String content;
    for (const auto& key : favourites)
        content << key << juce::newLine;
    file.replaceWithText (content);
}

juce::String AviationMainView::favouriteKey (const juce::String& cat, const juce::String& name) const
{
    return cat + "|" + name;
}

bool AviationMainView::isFavourited (const juce::String& cat, const juce::String& name) const
{
    return favourites.count (favouriteKey (cat, name).toStdString()) > 0;
}

void AviationMainView::setFavourited (const juce::String& cat, const juce::String& name, bool fav)
{
    const auto key = favouriteKey (cat, name).toStdString();
    if (fav)
        favourites.insert (key);
    else
        favourites.erase (key);
    saveFavourites();

    auto& pm = processor.getPresetManager();
    if (pm.getCurrentCategory() == cat && pm.getCurrentPresetName() == name)
        presetHeader.setFavourited (fav);
    presetBrowser.repaint();
}

// -----------------------------------------------------------------------------
//  Preset flow
// -----------------------------------------------------------------------------
void AviationMainView::selectCategory (const juce::String& category)
{
    auto& pm = processor.getPresetManager();
    categoryTabs.setActiveCategory (category);

    const auto names = pm.getPresetsForCategory (category);
    presetBrowser.setPresets (category, names);
    if (names.isEmpty())
        return;

    if (! names.contains (pm.getCurrentPresetName(), true)
        || ! pm.getCurrentCategory().equalsIgnoreCase (category))
        pm.loadPreset (category, names[0]);
    else
        refreshPresetUI();
}

void AviationMainView::refreshPresetUI()
{
    auto& pm = processor.getPresetManager();
    const auto category = pm.getCurrentCategory();
    const auto name = pm.getCurrentPresetName();

    categoryTabs.setActiveCategory (category);
    presetHeader.setPresetName (name);
    presetHeader.setFavourited (isFavourited (category, name));

    presetBrowser.setPresets (category, pm.getPresetsForCategory (category));
    presetBrowser.setSelectedPreset (name);

    dashboard->setTitleText (PresetDisplayUtils::shortenDisplayName (name));
    dashboard->setKeyText (parseKeyFromPresetName (name, pm.getCurrentRootNote()));

    sourceDropdown.setSourceText (prettySourceName (pm.getCurrentSampleId()));

    repaint (Aviation::presetCountBounds());
}

void AviationMainView::openSourceMenu()
{
    auto& pm = processor.getPresetManager();
    const auto category = pm.getCurrentCategory();
    const auto currentSampleId = pm.getCurrentSampleId();

    juce::PopupMenu menu;
    juce::StringArray sampleIds;

    for (const auto& entry : FactoryResources::getFactoryPresets())
    {
        if (! entry.category.equalsIgnoreCase (category) || entry.sampleId.isEmpty())
            continue;
        if (sampleIds.contains (entry.sampleId))
            continue;

        sampleIds.add (entry.sampleId);
        menu.addItem (sampleIds.size(), PresetDisplayUtils::shortenDisplayName (entry.name),
                      true, entry.sampleId == currentSampleId);
    }

    if (sampleIds.isEmpty())
        return;

    menu.showMenuAsync (juce::PopupMenu::Options()
                            .withTargetComponent (&sourceDropdown)
                            .withMinimumWidth (sourceDropdown.getWidth()),
        [this, sampleIds] (int result)
        {
            if (result <= 0 || result > sampleIds.size())
                return;

            const auto sampleId = sampleIds[result - 1];

            // Root note comes from the owning preset XML when present.
            int rootNote = 60;
            for (const auto& entry : FactoryResources::getFactoryPresets())
            {
                if (entry.sampleId != sampleId)
                    continue;
                if (auto xml = juce::XmlDocument::parse (juce::String::fromUTF8 (entry.xmlData,
                                                                                 entry.xmlSize)))
                    rootNote = xml->getIntAttribute (AviatorKeyz::PresetKey::ROOT_NOTE, 60);
                break;
            }

            auto& presetManager = processor.getPresetManager();
            if (processor.loadFactorySample (sampleId, rootNote))
            {
                presetManager.setPresetIdentity (presetManager.getCurrentCategory(),
                                                 presetManager.getCurrentPresetName(),
                                                 sampleId, rootNote);
                sourceDropdown.setSourceText (prettySourceName (sampleId));
            }
        });
}

// -----------------------------------------------------------------------------
//  A/B compare
// -----------------------------------------------------------------------------
void AviationMainView::openAbMenu()
{
    juce::PopupMenu menu;
    menu.addItem (1, "Slot A", true, currentAbSlot == 0);
    menu.addItem (2, "Slot B", true, currentAbSlot == 1);
    menu.addSeparator();
    menu.addItem (3, currentAbSlot == 0 ? "Copy A to B" : "Copy B to A");

    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&statusBar),
        [this] (int result)
        {
            if (result == 0)
                return;

            auto& apvts = processor.getAPVTS();

            if (result == 3)
            {
                abSnapshots[1 - currentAbSlot] = apvts.copyState().createCopy();
                return;
            }

            const int target = result - 1;
            if (target == currentAbSlot)
                return;

            abSnapshots[currentAbSlot] = apvts.copyState().createCopy();
            if (abSnapshots[target].isValid())
                apvts.replaceState (abSnapshots[target].createCopy());
            currentAbSlot = target;
            abUsed = true;
            statusBar.setAbLabel (currentAbSlot == 0 ? "A" : "B");
        });
}

// -----------------------------------------------------------------------------
//  Layout / paint
// -----------------------------------------------------------------------------
void AviationMainView::resized()
{
    cockpitBackground.setBounds (Aviation::cockpitBounds());
    topHeader.setBounds (Aviation::topHeaderBounds());
    presetHeader.setBounds (Aviation::presetHeaderBounds());
    categoryTabs.setBounds (Aviation::categoryTabsBounds());
    presetBrowser.setBounds (Aviation::browserBounds());
    sourceDropdown.setBounds (Aviation::sourceDropdownBounds());
    macroDeck->setBounds (Aviation::macroDeckBounds());
    statusBar.setBounds (Aviation::statusBarBounds());

    const auto cockpit = Aviation::cockpitBounds();

    // center console — on the global center axis so it stacks over the
    // cockpit pillar and the macro deck's brand block
    dashboard->setBounds (Aviation::kDesignW / 2 - 305, cockpit.getY() + 222, 610, 250);

    // left cockpit side panels
    velocityPanel->setBounds (cockpit.getX() + 26, cockpit.getY() + 274, 118, 104);
    layerMixPanel->setBounds (cockpit.getX() + 22, cockpit.getY() + 386, 126, 118);

    // right cockpit side panels
    filterPanel->setBounds (cockpit.getRight() - 148, cockpit.getY() + 274, 122, 100);
    envelopePanel->setBounds (cockpit.getRight() - 152, cockpit.getY() + 382, 130, 132);

    if (libraryOverlay != nullptr)
        libraryOverlay->setBounds (getLocalBounds());
    if (aboutOverlay != nullptr)
        aboutOverlay->setBounds (getLocalBounds());
}

void AviationMainView::paint (juce::Graphics& g)
{
    // background behind all panels
    juce::ColourGradient grad (Aviation::bgDeep(), 0.0f, 0.0f,
                               Aviation::bgBlack(), 0.0f, (float) getHeight(), false);
    g.setGradientFill (grad);
    g.fillAll();
}

void AviationMainView::paintOverChildren (juce::Graphics& g)
{
    // preset count strip — floats over the macro deck's top-left corner
    {
        auto strip = Aviation::presetCountBounds().toFloat();
        Aviation::fillMetalPanel (g, strip, 6.0f, juce::Colour (0xff0b151f), juce::Colour (0xff060d14));
        g.setColour (Aviation::goldDeep().withAlpha (0.35f));
        g.drawRoundedRectangle (strip.reduced (0.5f), 6.0f, 1.0f);

        const int total = processor.getPresetManager().getTotalPresetCount();
        g.setFont (Aviation::label (11.0f, 0.12f));
        g.setColour (Aviation::textPrimary().withAlpha (0.9f));
        g.drawText (juce::String (total) + " PRESETS",
                    strip.toNearestInt().withTrimmedLeft (14), juce::Justification::centredLeft);

        AviationIcons::fill (g, AviationIcons::star(),
                             { strip.getRight() - 26.0f, strip.getCentreY() - 7.0f, 14.0f, 14.0f },
                             Aviation::goldBright());
    }
}
