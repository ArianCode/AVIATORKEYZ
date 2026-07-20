#include "CockpitCrossworldPanel.h"
#include "../../PluginProcessor.h"
#include "../../State/StateSchema.h"
#include "../AviatorTokens.h"

namespace
{
using GF = AviationGauge::ValueFormat;

struct GaugeSpec
{
    const char* paramId;
    const char* name;
    const char* hint;
    GF format;
    bool freezeToggle;
};
} // namespace

CockpitCrossworldPanel::CockpitCrossworldPanel (AviatorKeyzProcessor& p)
    : processor (p)
{
    setOpaque (true);
    addAndMakeVisible (presetControlBar);
    addAndMakeVisible (topPresetBar);
    addAndMakeVisible (photoBackground);
    photoBackground.setInterceptsMouseClicks (false, false);

    addAndMakeVisible (presetNavigator);
    instrumentBar = std::make_unique<InstrumentPanelBar>();
    addAndMakeVisible (*instrumentBar);
    buildInstrumentGauges();
    buildPhotoAnchors();

    searchOverlay = std::make_unique<PresetSearchOverlay> (processor);
    addChildComponent (*searchOverlay);

    libraryOverlay = std::make_unique<PresetLibraryOverlay> (processor);
    addChildComponent (*libraryOverlay);

    aboutOverlay = std::make_unique<AboutOverlay>();
    addChildComponent (*aboutOverlay);

    presetSidebar = std::make_unique<AdvancedPresetSidebar> (processor);
    addAndMakeVisible (*presetSidebar);

    topPresetBar.onCategorySelected = [this] (const juce::String& cat) { selectCategory (cat); };

    presetControlBar.onBrowseRequested = [this] { openSearchOverlay(); };
    presetControlBar.onPrevPreset = [this] { processor.getPresetManager().loadAdjacentPresetInCategory (-1); };
    presetControlBar.onNextPreset = [this] { processor.getPresetManager().loadAdjacentPresetInCategory (+1); };
    presetControlBar.onFavoriteToggled = [this] (bool fav) {
        auto& pm = processor.getPresetManager();
        setFavourited (pm.getCurrentCategory(), pm.getCurrentPresetName(), fav);
    };
    presetControlBar.onSaveRequested = [this] { saveCurrentPreset(); };

    presetNavigator.onPrevPreset = [this] { processor.getPresetManager().loadAdjacentPresetInCategory (-1); };
    presetNavigator.onNextPreset = [this] { processor.getPresetManager().loadAdjacentPresetInCategory (+1); };
    presetNavigator.onFavoriteToggled = [this] (bool fav) {
        auto& pm = processor.getPresetManager();
        setFavourited (pm.getCurrentCategory(), pm.getCurrentPresetName(), fav);
    };

    addAndMakeVisible (footer);
    footer.onSettingsClicked = [this] { openLibraryOverlay(); };
    footer.onAboutClicked = [this] { openAboutOverlay(); };

    buildTopPresetBar();
    activeCategory = processor.getPresetManager().getCurrentCategory();
    refreshPresetUIImpl();
}

CockpitCrossworldPanel::~CockpitCrossworldPanel() = default;

juce::String CockpitCrossworldPanel::favouriteKey (const juce::String& cat, const juce::String& name) const
{
    return cat + "|" + name;
}

bool CockpitCrossworldPanel::isFavourited (const juce::String& cat, const juce::String& name) const
{
    return favourites.count (favouriteKey (cat, name).toStdString()) > 0;
}

void CockpitCrossworldPanel::setFavourited (const juce::String& cat, const juce::String& name, bool fav)
{
    const auto key = favouriteKey (cat, name).toStdString();
    if (fav)
        favourites.insert (key);
    else
        favourites.erase (key);

    presetControlBar.setFavourited (fav);
    presetNavigator.setFavourited (fav);
}

void CockpitCrossworldPanel::saveCurrentPreset()
{
    auto& pm = processor.getPresetManager();
    pm.saveUserPreset (pm.getCurrentCategory(), pm.getCurrentPresetName());
}

void CockpitCrossworldPanel::buildInstrumentGauges()
{
    namespace P = AviatorKeyz::ParamID;
    auto& apvts = processor.getAPVTS();
    auto& gauges = instrumentBar->getGauges();

    const GaugeSpec specs[] {
        { P::GLIDE_TIME,    "THROTTLE",   "Glide",     GF::glideSeconds, false },
        { P::INPUT_GAIN,    "ENGINE",     "Gain",      GF::decibels,     false },
        { P::STEREO_WIDTH,  "WINGS",      "Brightness", GF::percent,      false },
        { P::REVERB_AMOUNT, "ALTITUDE",   "Reverb",    GF::percent,      false },
        { P::TONE,          "CABIN",      "Tone",      GF::toneDb,       false },
        { P::SMEAR,         "TURBULENCE", "Filter",    GF::percent,      false },
        { P::ENV_ATTACK,    "ATTACK",     "Attack",    GF::envelopeMs,   false },
        { P::ENV_RELEASE,   "RELEASE",    "Release",   GF::envelopeMs,   false },
    };

    for (const auto& spec : specs)
    {
        auto gauge = std::make_unique<AviationGauge> (apvts, spec.paramId, spec.name, spec.hint,
                                                      spec.format, spec.freezeToggle);
        instrumentBar->addAndMakeVisible (*gauge);
        gauges.push_back (std::move (gauge));
    }
}

namespace
{
PrecisionKnob::ValueFormat knobFormatForParam (const char* paramId) noexcept
{
    namespace P = AviatorKeyz::ParamID;
    if (paramId == nullptr)
        return PrecisionKnob::ValueFormat::percent;

    const juce::String id (paramId);
    if (id == P::GLIDE_TIME)
        return PrecisionKnob::ValueFormat::glideSeconds;
    if (id == P::TONE)
        return PrecisionKnob::ValueFormat::toneDb;
    if (id == P::ENV_ATTACK || id == P::ENV_RELEASE)
        return PrecisionKnob::ValueFormat::envelopeMs;
    return PrecisionKnob::ValueFormat::percent;
}
} // namespace

void CockpitCrossworldPanel::buildPhotoAnchors()
{
    namespace AC = AviatorCockpit;
    auto& apvts = processor.getAPVTS();
    const int count = AC::getKnobAnchorCount();

    for (int i = 0; i < count; ++i)
    {
        const auto& anchor = AC::getKnobAnchors()[i];
        photoAnchorSpecs.push_back (anchor);

        if (anchor.paramId != nullptr)
        {
            const juce::String paramId (anchor.paramId);

            if (anchor.kind == AC::AnchorKind::toggle)
            {
                auto toggle = std::make_unique<ReverseToggle> (apvts, paramId,
                                                               juce::String (anchor.label), juce::String());
                toggle->setOpaque (false);
                addAndMakeVisible (*toggle);
                photoControls.push_back (std::move (toggle));
            }
            else
            {
                auto knob = std::make_unique<PhotoAnchoredKnob> (apvts, paramId,
                                                                 knobFormatForParam (anchor.paramId));
                addAndMakeVisible (*knob);
                photoControls.push_back (std::move (knob));
            }
            continue;
        }

        if (anchor.action == nullptr)
            continue;

        auto button = std::make_unique<juce::TextButton> (anchor.label);
        button->setColour (juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
        button->setColour (juce::TextButton::buttonOnColourId, AviatorTokens::instrumentCyan().withAlpha (0.35f));
        button->setColour (juce::TextButton::textColourOffId, AviatorTokens::instrumentCyan().withAlpha (0.85f));

        const juce::String action (anchor.action);
        if (action == "presetPrev")
            button->onClick = [this] { processor.getPresetManager().loadAdjacentPresetInCategory (-1); };
        else if (action == "presetNext")
            button->onClick = [this] { processor.getPresetManager().loadAdjacentPresetInCategory (+1); };
        else if (action == "library")
            button->onClick = [this] { openSearchOverlay(); };

        addAndMakeVisible (*button);
        photoControls.push_back (std::move (button));
    }
}

void CockpitCrossworldPanel::buildTopPresetBar()
{
    auto& pm = processor.getPresetManager();
    topPresetBar.setCategories (pm.getAllCategories());
    topPresetBar.setBuildStampText ("v" + juce::String (JucePlugin_VersionString));
    topBarCategoriesBuilt = true;
}

void CockpitCrossworldPanel::selectCategory (const juce::String& category)
{
    activeCategory = category;
    topPresetBar.setActiveCategory (category);

    auto& pm = processor.getPresetManager();
    const auto names = pm.getPresetsForCategory (category);
    if (names.isEmpty())
        return;

    // Only load a different preset when the user actually changes category.
    // Never auto-jump to names[0] while already on a valid preset in this category.
    if (pm.getCurrentCategory().equalsIgnoreCase (category)
        && names.contains (pm.getCurrentPresetName(), true))
        return;

    pm.loadPreset (category, names[0]);
}

void CockpitCrossworldPanel::openSearchOverlay()
{
    if (searchOverlay == nullptr)
        return;

    auto& pm = processor.getPresetManager();
    searchOverlay->setBounds (getLocalBounds());
    searchOverlay->showForCategory (activeCategory.isEmpty() ? pm.getCurrentCategory() : activeCategory);
    searchOverlay->toFront (true);
}

void CockpitCrossworldPanel::openLibraryOverlay()
{
    if (libraryOverlay == nullptr)
        return;

    libraryOverlay->setBounds (getLocalBounds());
    libraryOverlay->showOverlay();
    libraryOverlay->toFront (true);
}

void CockpitCrossworldPanel::openAboutOverlay()
{
    if (aboutOverlay == nullptr)
        return;

    aboutOverlay->setBounds (getLocalBounds());
    aboutOverlay->showOverlay();
    aboutOverlay->toFront (true);
}

void CockpitCrossworldPanel::requestPresetUiRefresh()
{
    triggerAsyncUpdate();
}

void CockpitCrossworldPanel::refreshPresetUI()
{
    requestPresetUiRefresh();
}

void CockpitCrossworldPanel::handleAsyncUpdate()
{
    refreshPresetUIImpl();
}

void CockpitCrossworldPanel::refreshPresetUIImpl()
{
    auto& pm = processor.getPresetManager();
    const auto name = pm.getCurrentPresetName();
    const auto cat  = pm.getCurrentCategory();

    if (! topBarCategoriesBuilt)
        buildTopPresetBar();

    activeCategory = cat;
    topPresetBar.setActiveCategory (cat);
    presetControlBar.setPresetName (name);
    presetNavigator.setPreset (cat, name);

    const bool fav = isFavourited (cat, name);
    presetControlBar.setFavourited (fav);
    presetNavigator.setFavourited (fav);

    footer.setActiveStatus ("Active");
    footer.setSampleRate (processor.getSampleRate() > 0 ? processor.getSampleRate() : 44100.0);
    footer.setBlockSize (processor.getBlockSize() > 0 ? processor.getBlockSize() : 256);
    footer.setHostDescription (juce::PluginHostType().getHostDescription());

    repaint();

    if (presetSidebar != nullptr)
        presetSidebar->refresh();

    if (searchOverlay != nullptr && searchOverlay->isVisible())
        searchOverlay->showForCategory (cat);
}

void CockpitCrossworldPanel::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff030309));
}

void CockpitCrossworldPanel::layoutZones()
{
    auto bounds = getLocalBounds();

    const int footerH = AviatorTokens::kFooterH;
    footer.setBounds (bounds.removeFromBottom (footerH));

    const int instrumentH = AviatorTokens::scaledFor (*this, InstrumentPanelBar::kDesignHeight);
    if (instrumentBar != nullptr)
        instrumentBar->setBounds (bounds.removeFromBottom (instrumentH));

    const int ctrlH = AviatorTokens::scaledFor (*this, kPresetControlBarH);
    presetControlBar.setBounds (bounds.removeFromTop (ctrlH));

    const int topH = AviatorTokens::scaledFor (*this, kTopPresetBarH);
    topPresetBar.setBounds (bounds.removeFromTop (topH));

    auto cockpitArea = bounds;
    const int sidebarW = AviatorTokens::scaledFor (*this, kSidebarDesignW);
    if (presetSidebar != nullptr)
        presetSidebar->setBounds (cockpitArea.removeFromLeft (sidebarW));

    photoBackground.setBounds (cockpitArea);
    presetNavigator.setBounds (cockpitArea);

    const auto photoArea = photoBackground.getBounds();
    for (size_t i = 0; i < photoControls.size() && i < photoAnchorSpecs.size(); ++i)
    {
        if (photoControls[i] != nullptr)
        {
            photoControls[i]->setBounds (AviatorCockpit::anchorBounds (photoArea, photoAnchorSpecs[i]));
            photoControls[i]->toFront (false);
        }
    }

    if (searchOverlay != nullptr)
        searchOverlay->setBounds (getLocalBounds());
    if (libraryOverlay != nullptr)
        libraryOverlay->setBounds (getLocalBounds());
    if (aboutOverlay != nullptr)
        aboutOverlay->setBounds (getLocalBounds());

    presetControlBar.toFront (false);
    topPresetBar.toFront (false);
    if (presetSidebar != nullptr)
        presetSidebar->toFront (false);
    presetNavigator.toFront (false);
    if (instrumentBar != nullptr)
        instrumentBar->toFront (false);
    footer.toFront (false);
    if (searchOverlay != nullptr && searchOverlay->isVisible())
        searchOverlay->toFront (true);
    if (libraryOverlay != nullptr && libraryOverlay->isVisible())
        libraryOverlay->toFront (true);
    if (aboutOverlay != nullptr && aboutOverlay->isVisible())
        aboutOverlay->toFront (true);
}

void CockpitCrossworldPanel::resized()
{
    layoutZones();
}
