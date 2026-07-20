#include "AviationMainView.h"
#include "AviationIcons.h"
#include "AviationTheme.h"
#include "../../PluginProcessor.h"
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

    // Top-right dropdown is the preset selector for the active category.
    addAndMakeVisible (presetDropdown);
    presetDropdown.onClicked = [this] { openPresetMenu(); };

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
        // Break out of menu/mouse stack before heavy work and UI refresh.
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
}

// -----------------------------------------------------------------------------
//  Preset flow
// -----------------------------------------------------------------------------
void AviationMainView::selectCategory (const juce::String& category)
{
    auto& pm = processor.getPresetManager();
    categoryTabs.setActiveCategory (category);

    const auto names = pm.getPresetsForCategory (category);
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

    dashboard->setTitleText (PresetDisplayUtils::shortenDisplayName (name));
    dashboard->setKeyText (parseKeyFromPresetName (name, pm.getCurrentRootNote()));

    presetDropdown.setSourceText (PresetDisplayUtils::shortenDisplayName (name));
}

void AviationMainView::openPresetMenu()
{
    auto& pm = processor.getPresetManager();
    const auto category = pm.getCurrentCategory();
    const auto names = pm.getPresetsForCategory (category);
    if (names.isEmpty())
        return;

    const auto current = pm.getCurrentPresetName();

    juce::PopupMenu menu;
    for (int i = 0; i < names.size(); ++i)
    {
        auto display = PresetDisplayUtils::shortenDisplayName (names[i]);
        if (isFavourited (category, names[i]))
            display = juce::String::fromUTF8 ("\xe2\x98\x85 ") + display; // filled star prefix
        menu.addItem (i + 1, display, true, names[i] == current);
    }

    menu.showMenuAsync (juce::PopupMenu::Options()
                            .withTargetComponent (&presetDropdown)
                            .withMinimumWidth (presetDropdown.getWidth())
                            .withMaximumNumColumns (1),
        [this, names] (int result)
        {
            if (result <= 0 || result > names.size())
                return;
            auto& presetManager = processor.getPresetManager();
            presetManager.loadPreset (presetManager.getCurrentCategory(), names[result - 1]);
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
    presetDropdown.setBounds (Aviation::sourceDropdownBounds());
    macroDeck->setBounds (Aviation::macroDeckBounds());
    statusBar.setBounds (Aviation::statusBarBounds());

    const auto cockpit = Aviation::cockpitBounds();

    // center console — on the global center axis so it stacks over the
    // cockpit pillar and the macro deck's brand block
    dashboard->setBounds (Aviation::kDesignW / 2 - 305, cockpit.getY() + 222, 610, 250);

    // cockpit side panels, mirrored insets from the cockpit edges
    velocityPanel->setBounds (cockpit.getX() + 26, cockpit.getY() + 274, 118, 104);
    layerMixPanel->setBounds (cockpit.getX() + 22, cockpit.getY() + 386, 126, 118);
    filterPanel->setBounds (cockpit.getRight() - 148, cockpit.getY() + 274, 122, 100);
    envelopePanel->setBounds (cockpit.getRight() - 152, cockpit.getY() + 382, 130, 132);

    if (libraryOverlay != nullptr)
        libraryOverlay->setBounds (getLocalBounds());
    if (aboutOverlay != nullptr)
        aboutOverlay->setBounds (getLocalBounds());
}

void AviationMainView::paint (juce::Graphics& g)
{
    juce::ColourGradient grad (Aviation::bgDeep(), 0.0f, 0.0f,
                               Aviation::bgBlack(), 0.0f, (float) getHeight(), false);
    g.setGradientFill (grad);
    g.fillAll();
}
