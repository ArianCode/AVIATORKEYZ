#include "CockpitCrossworldPanel.h"
#include "CockpitLayout.h"
#include "../../PluginProcessor.h"
#include "../../State/StateSchema.h"
#include "../AviatorTokens.h"

namespace
{
bool zoneIsCircular (AviatorCockpit::ZoneId z)
{
    return z == AviatorCockpit::ZoneId::radarAdsr || z == AviatorCockpit::ZoneId::radarLfo;
}

const char* kCategoryFilters[] = {
    "ALL", "INIT", "KEYS", "LEADS", "PADS", "PLUCKS", "BASS", "ATMOS", "FX"
};

std::vector<juce::Component*> ptrs (const std::vector<std::unique_ptr<PrecisionKnob>>& knobs)
{
    std::vector<juce::Component*> out;
    out.reserve (knobs.size());
    for (const auto& k : knobs)
        out.push_back (k.get());
    return out;
}
} // namespace

struct CockpitCrossworldPanel::PresetListModel : public juce::ListBoxModel
{
    CockpitCrossworldPanel& owner;

    explicit PresetListModel (CockpitCrossworldPanel& o) : owner (o) {}

    int getNumRows() override { return owner.presetDisplayNames.size(); }

    void paintListBoxItem (int row, juce::Graphics& g, int w, int h, bool sel) override
    {
        if (! juce::isPositiveAndBelow (row, owner.presetDisplayNames.size()))
            return;

        const float sc = AviatorTokens::scaleFor (owner);

        if (sel)
        {
            g.setColour (AviatorTokens::instrumentCyan().withAlpha (0.22f));
            g.fillRect (0, 0, w, h);
            g.setColour (AviatorTokens::champagneGold().withAlpha (0.35f));
            g.drawHorizontalLine (h - 1, 2.f, (float) w - 2.f);
        }

        g.setFont (AviatorTokens::hud (11.f * sc));
        g.setColour (sel ? AviatorTokens::textPrimary() : AviatorTokens::textMuted());
        g.drawText (owner.presetDisplayNames[row], 6, 0, w - 10, h, juce::Justification::centredLeft);
    }

    void listBoxItemClicked (int row, const juce::MouseEvent&) override
    {
        if (! juce::isPositiveAndBelow (row, owner.presetFlatIndices.size()))
            return;

        owner.processor.getPresetManager().loadPresetByFlatIndex (owner.presetFlatIndices[row]);
        owner.refreshPresetUI();
    }
};

void CockpitCrossworldPanel::StatusStrip::paint (juce::Graphics& g)
{
    const float sc = AviatorTokens::scaleFor (*this);
    AviatorTokens::paintStatusStrip (g, getLocalBounds().toFloat().reduced (0.5f));

    readout.setFont (AviatorTokens::hud (12.f * sc));
    readout.setColour (juce::Label::textColourId, AviatorTokens::textPrimary());
}

CockpitCrossworldPanel::GlassZone::GlassZone (AviatorCockpit::ZoneId id)
    : zoneId (id)
{
    setOpaque (false);
    switch (id)
    {
        case AviatorCockpit::ZoneId::leftMfd:            title = "ENGINE / OSC"; break;
        case AviatorCockpit::ZoneId::rightMfd:           title = "FX / EFFECTS"; break;
        case AviatorCockpit::ZoneId::radarAdsr:          title = "ADSR"; break;
        case AviatorCockpit::ZoneId::radarLfo:           title = "LFO / MOD"; break;
        case AviatorCockpit::ZoneId::throttleQuadrant:   title = "MACRO"; break;
        case AviatorCockpit::ZoneId::autopilotStrip:
        case AviatorCockpit::ZoneId::overhead:           title = {}; break;
    }
}

void CockpitCrossworldPanel::GlassZone::paint (juce::Graphics& g)
{
    const float sc = AviatorTokens::scaleFor (*this);
    AviatorTokens::paintGlassPanel (g, getLocalBounds().toFloat().reduced (1.f), zoneIsCircular (zoneId));

    if (title.isNotEmpty())
    {
        auto titleArea = getLocalBounds().reduced (AviatorTokens::scaledFor (*this, AviatorTokens::kPanelPad))
                             .removeFromTop (AviatorTokens::scaledFor (*this, AviatorTokens::kTitleH));
        AviatorTokens::paintSectionTitle (g, titleArea, title, sc);
    }
}

CockpitCrossworldPanel::CockpitCrossworldPanel (AviatorKeyzProcessor& p)
    : processor (p)
    , presetList ("presets", nullptr)
    , searchBox()
    , selectedPresetLabel ("selected", "")
    , syncReadout ("sync", "SYNC")
    , decayReadout ("decay", "DECAY")
    , sustainReadout ("sustain", "SUSTAIN")
    , presetModel (std::make_unique<PresetListModel> (*this))
{
    setOpaque (true);
    addAndMakeVisible (photoBackground);

    for (auto* z : { &leftBrowserPanel, &effectsPanel, &envelopePanel,
                     &modulationPanel, &centerMacroZone })
        addAndMakeVisible (*z);

    addAndMakeVisible (headerStatusStrip);
    headerStatusStrip.addAndMakeVisible (headerStatusStrip.readout);

    presetList.setModel (presetModel.get());
    presetList.setColour (juce::ListBox::backgroundColourId, juce::Colours::transparentBlack);
    presetList.setOutlineThickness (0);
    presetList.setRowHeight (22);
    addAndMakeVisible (presetList);

    searchBox.setTextToShowWhenEmpty ("Search Presets", AviatorTokens::textMuted());
    searchBox.setFont (AviatorTokens::hud (11.f));
    searchBox.setColour (juce::TextEditor::backgroundColourId, juce::Colour (0x40050d1a));
    searchBox.setColour (juce::TextEditor::outlineColourId, AviatorTokens::instrumentCyan().withAlpha (0.45f));
    searchBox.setColour (juce::TextEditor::textColourId, AviatorTokens::textPrimary());
    searchBox.setColour (juce::CaretComponent::caretColourId, AviatorTokens::instrumentCyan());
    searchBox.setIndents (6, 4);
    searchBox.onTextChange = [this] { applyCategoryFilter(); };
    addAndMakeVisible (searchBox);

    selectedPresetLabel.setFont (AviatorTokens::hud (10.f));
    selectedPresetLabel.setColour (juce::Label::textColourId, AviatorTokens::mfdAmber());
    selectedPresetLabel.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (selectedPresetLabel);

    for (auto* lab : { &decayReadout, &sustainReadout, &syncReadout })
    {
        lab->setFont (AviatorTokens::hud (9.f));
        lab->setColour (juce::Label::textColourId, AviatorTokens::textMuted());
        lab->setJustificationType (juce::Justification::centred);
        addAndMakeVisible (*lab);
    }

    addAndMakeVisible (footer);
    footer.onSettingsClicked = [this] {
        if (onLibraryRequested)
            onLibraryRequested();
    };
    footer.onAboutClicked = [this] {
        if (onAboutRequested)
            onAboutRequested();
    };

    buildPanelControls();
    buildCategoryFilters();
    buildPresetList();
    refreshPresetUI();
    startTimerHz (30);
}

CockpitCrossworldPanel::~CockpitCrossworldPanel() = default;

std::unique_ptr<PrecisionKnob> CockpitCrossworldPanel::makeKnob (const char* paramId,
                                                                 const juce::String& name,
                                                                 const juce::String& sublabel,
                                                                 PrecisionKnob::ValueFormat format)
{
    auto knob = std::make_unique<PrecisionKnob> (processor.getAPVTS(), paramId, name, sublabel, format);
    addAndMakeVisible (*knob);
    return knob;
}

PrecisionKnob::ValueFormat CockpitCrossworldPanel::formatForParam (const char* paramId) const
{
    if (paramId == nullptr)
        return PrecisionKnob::ValueFormat::percent;

    const juce::String id (paramId);
    if (id == AviatorKeyz::ParamID::GLIDE_TIME)
        return PrecisionKnob::ValueFormat::glideSeconds;
    if (id == AviatorKeyz::ParamID::TONE)
        return PrecisionKnob::ValueFormat::toneDb;
    return PrecisionKnob::ValueFormat::percent;
}

void CockpitCrossworldPanel::buildPanelControls()
{
    using PF = PrecisionKnob::ValueFormat;
    namespace P = AviatorKeyz::ParamID;

    macroKnobs.push_back (makeKnob (P::GLIDE_TIME,    "THROTTLE",      "Glide",  PF::glideSeconds));
    macroKnobs.push_back (makeKnob (P::INPUT_GAIN,    "ENGINE",        "Drive",  PF::percent));
    macroKnobs.push_back (makeKnob (P::STEREO_WIDTH,  "WINGS",         "Width",  PF::percent));
    macroKnobs.push_back (makeKnob (P::REVERB_AMOUNT, "ALTITUDE",      "Reverb", PF::percent));
    macroKnobs.push_back (makeKnob (P::TONE,          "CABIN LIGHTS",  "Tone",   PF::toneDb));
    macroKnobs.push_back (makeKnob (P::SMEAR,         "TURBULENCE",    "Lo-Fi",  PF::percent));

    envKnobs.push_back (makeKnob (P::ENV_ATTACK,  "ATTACK",  {}, PF::percent));
    envKnobs.push_back (makeKnob (P::ENV_RELEASE, "RELEASE", {}, PF::percent));

    modKnobs.push_back (makeKnob (P::GLIDE_TIME, "RATE",   "Hz",    formatForParam (P::GLIDE_TIME)));
    modKnobs.push_back (makeKnob (P::SMEAR,      "DEPTH",  {},      PF::percent));
    modKnobs.push_back (makeKnob (P::TONE,       "SHAPE",  {},      PF::toneDb));
    modKnobs.push_back (makeKnob (P::PAN,        "AMOUNT", {},      PF::percent));

    motionToggle = std::make_unique<ReverseToggle> (processor.getAPVTS(), P::REVERSE, "MOTION", "On/Off");
    addAndMakeVisible (*motionToggle);

    fxKnobs.push_back (makeKnob (P::REVERB_AMOUNT, "REVERB", {}, PF::percent));
    fxKnobs.push_back (makeKnob (P::REVERB_SIZE,   "DELAY",  {}, PF::percent));
    fxKnobs.push_back (makeKnob (P::STEREO_WIDTH,  "WIDTH",  {}, PF::percent));
    fxKnobs.push_back (makeKnob (P::SMEAR,         "LO-FI",  {}, PF::percent));
    fxKnobs.push_back (makeKnob (P::OUTPUT_GAIN,   "MIX",    {}, PF::percent));
}

void CockpitCrossworldPanel::buildCategoryFilters()
{
    categoryButtons.clear();

    for (const char* label : kCategoryFilters)
    {
        auto btn = std::make_unique<juce::TextButton> (label);
        btn->setClickingTogglesState (false);
        btn->setColour (juce::TextButton::buttonColourId, juce::Colour (0x50050d1a));
        btn->setColour (juce::TextButton::buttonOnColourId, AviatorTokens::instrumentCyan().withAlpha (0.35f));
        btn->setColour (juce::TextButton::textColourOffId, AviatorTokens::textMuted());
        btn->setColour (juce::TextButton::textColourOnId, AviatorTokens::textPrimary());

        btn->onClick = [this, filter = juce::String (label)]
        {
            categoryFilter = (filter == "ALL" || filter == "INIT") ? juce::String() : filter;
            updateCategoryButtonStates();
            applyCategoryFilter();
        };

        addAndMakeVisible (*btn);
        categoryButtons.push_back (std::move (btn));
    }

    categoryFilter.clear();
    updateCategoryButtonStates();
}

void CockpitCrossworldPanel::updateCategoryButtonStates()
{
    for (auto& btn : categoryButtons)
    {
        const auto text = btn->getButtonText();
        const bool active = (text == "ALL" || text == "INIT")
                                ? categoryFilter.isEmpty()
                                : (categoryFilter == text);
        btn->setToggleState (active, juce::dontSendNotification);
        btn->setColour (juce::TextButton::textColourOffId,
                        active ? AviatorTokens::instrumentCyan() : AviatorTokens::textMuted());
    }
}

bool CockpitCrossworldPanel::categoryMatchesFilter (const juce::String& category) const
{
    if (categoryFilter.isEmpty())
        return true;

    const auto c = category.toUpperCase();

    if (categoryFilter == "LEADS")   return c.contains ("LEAD");
    if (categoryFilter == "PADS")    return c.contains ("PAD");
    if (categoryFilter == "KEYS")    return c.contains ("CHORD") || c.contains ("KEY");
    if (categoryFilter == "PLUCKS")  return c.contains ("ARP") || c.contains ("BELL");
    if (categoryFilter == "BASS")    return c.contains ("BRASS") || c.contains ("ENSEMBLE");
    if (categoryFilter == "ATMOS")   return c.contains ("PAD") || c.contains ("STRING") || c.contains ("ENSEMBLE");
    if (categoryFilter == "FX")      return c.contains ("VOCAL") || c.contains ("SYNTH");

    return true;
}

void CockpitCrossworldPanel::buildPresetList()
{
    applyCategoryFilter();
}

void CockpitCrossworldPanel::applyCategoryFilter()
{
    presetDisplayNames.clear();
    presetCategories.clear();
    presetFlatIndices.clear();

    const auto query = searchBox.getText().trim().toLowerCase();
    auto& pm = processor.getPresetManager();
    int flatIndex = 0;

    for (const auto& category : pm.getAllCategories())
    {
        for (const auto& name : pm.getPresetsForCategory (category))
        {
            const bool matchesCat = categoryMatchesFilter (category);
            bool matchesSearch = true;

            if (query.isNotEmpty())
            {
                const auto hay = (name + " " + category).toLowerCase();
                matchesSearch = hay.contains (query);
            }

            if (matchesCat && matchesSearch)
            {
                presetCategories.add (category);
                presetDisplayNames.add (name);
                presetFlatIndices.add (flatIndex);
            }

            ++flatIndex;
        }
    }

    presetList.updateContent();

    const int currentFlat = pm.getCurrentPresetIndex();
    int selectRow = 0;
    for (int i = 0; i < presetFlatIndices.size(); ++i)
    {
        if (presetFlatIndices[i] == currentFlat)
        {
            selectRow = i;
            break;
        }
    }

    if (presetDisplayNames.size() > 0)
        presetList.selectRow (selectRow);
}

void CockpitCrossworldPanel::refreshPresetUI()
{
    auto& pm = processor.getPresetManager();
    const auto name = pm.getCurrentPresetName();
    const auto cat  = pm.getCurrentCategory();
    const int patch = pm.getCurrentPresetIndex() + 1;

    selectedPresetLabel.setText (cat.toUpperCase() + " / " + name.toUpperCase(), juce::dontSendNotification);

    headerStatusStrip.readout.setText (
        "PATCH " + juce::String (patch).paddedLeft ('0', 2)
        + "  |  " + name.toUpperCase()
        + "  |  BPM 128  |  VOICES 04",
        juce::dontSendNotification);

    footer.setActiveStatus ("Active");
    footer.setSampleRate (processor.getSampleRate() > 0 ? processor.getSampleRate() : 44100.0);
    footer.setBlockSize (processor.getBlockSize() > 0 ? processor.getBlockSize() : 256);
    footer.setHostDescription (juce::PluginHostType().getHostDescription());

    buildPresetList();
    applyCategoryFilter();
    repaint();
}

void CockpitCrossworldPanel::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff030309));
}

void CockpitCrossworldPanel::timerCallback()
{
    lfoSweep += 0.08f;
    if (lfoSweep > juce::MathConstants<float>::twoPi)
        lfoSweep -= juce::MathConstants<float>::twoPi;

    if (auto* rev = processor.getAPVTS().getRawParameterValue (AviatorKeyz::ParamID::REVERSE))
    {
        const bool on = rev->load() > 0.5f;
        syncReadout.setText ("SYNC\n" + juce::String (on ? "On" : "Off"), juce::dontSendNotification);
    }

    repaint (modulationPanel.getBounds());
}

void CockpitCrossworldPanel::layoutLeftBrowser()
{
    const float sc = AviatorTokens::scaleFor (*this);
    const int pad  = AviatorTokens::scaledFor (*this, AviatorTokens::kPanelPad);
    const int titleH = AviatorTokens::scaledFor (*this, AviatorTokens::kTitleH);

    auto area = leftBrowserPanel.getBounds().reduced (pad);
    area.removeFromTop (titleH);

    const int catH = AviatorTokens::scaledFor (*this, 20);
    const int catRows = 2;
    auto catArea = area.removeFromTop (catH * catRows + 4);
    const int cols = 5;
    const int catBtnW = (catArea.getWidth() - 4 * 3) / cols;
    const int catBtnH = catH;

    int idx = 0;
    for (auto& btn : categoryButtons)
    {
        const int row = idx / cols;
        const int col = idx % cols;
        btn->setBounds (catArea.getX() + col * (catBtnW + 3),
                        catArea.getY() + row * (catBtnH + 2),
                        catBtnW, catBtnH);
        juce::ignoreUnused (sc);
        ++idx;
    }

    area.removeFromTop (4);
    searchBox.setBounds (area.removeFromTop (AviatorTokens::scaledFor (*this, 24)));
    area.removeFromTop (4);

    selectedPresetLabel.setBounds (area.removeFromBottom (AviatorTokens::scaledFor (*this, 28)));
    presetList.setBounds (area);
    presetList.setRowHeight (AviatorTokens::scaledFor (*this, 22));
}

void CockpitCrossworldPanel::layoutCenterMacros()
{
    const float sc = AviatorTokens::scaleFor (*this);
    const int pad = AviatorTokens::scaledFor (*this, AviatorTokens::kPanelPad);
    const int titleH = AviatorTokens::scaledFor (*this, AviatorTokens::kTitleH);

    auto area = CockpitLayout::contentArea (centerMacroZone.getBounds(), titleH, pad);
    const int cellW = AviatorTokens::scaledFor (*this, AviatorTokens::kKnobCellW);
    const int cellH = AviatorTokens::scaledFor (*this, AviatorTokens::kKnobCellH);
    const int gapX = AviatorTokens::scaledFor (*this, AviatorTokens::kGridGapX);
    const int gapY = AviatorTokens::scaledFor (*this, AviatorTokens::kGridGapY);

    CockpitLayout::layoutGrid (area, ptrs (macroKnobs), 3, cellW, cellH, gapX, gapY);
    juce::ignoreUnused (sc);
}

void CockpitCrossworldPanel::layoutEnvelopePanel()
{
    const int pad = AviatorTokens::scaledFor (*this, AviatorTokens::kPanelPad);
    const int titleH = AviatorTokens::scaledFor (*this, AviatorTokens::kTitleH);

    auto area = CockpitLayout::contentArea (envelopePanel.getBounds(), titleH, pad);
    const int cellH = AviatorTokens::scaledFor (*this, AviatorTokens::kKnobCellH);
    const int gap = AviatorTokens::scaledFor (*this, AviatorTokens::kGridGapX);

    decayReadout.setText ("DECAY\n—", juce::dontSendNotification);
    sustainReadout.setText ("SUSTAIN\n—", juce::dontSendNotification);

    std::vector<juce::Component*> row;
    if (envKnobs.size() > 0) row.push_back (envKnobs[0].get());
    row.push_back (&decayReadout);
    row.push_back (&sustainReadout);
    if (envKnobs.size() > 1) row.push_back (envKnobs[1].get());

    const int totalGap = gap * (int) (row.size() - 1);
    const int w = juce::jmax (1, (area.getWidth() - totalGap) / (int) row.size());

    for (auto* comp : row)
    {
        comp->setBounds (area.removeFromLeft (w).withHeight (cellH));
        if (area.getWidth() > 0)
            area.removeFromLeft (gap);
    }
}

void CockpitCrossworldPanel::layoutModulationPanel()
{
    const int pad = AviatorTokens::scaledFor (*this, AviatorTokens::kPanelPad);
    const int titleH = AviatorTokens::scaledFor (*this, AviatorTokens::kTitleH);

    auto area = CockpitLayout::contentArea (modulationPanel.getBounds(), titleH, pad);
    const int cellW = AviatorTokens::scaledFor (*this, AviatorTokens::kKnobCellW);
    const int cellH = AviatorTokens::scaledFor (*this, AviatorTokens::kKnobCellH);
    const int gapX = AviatorTokens::scaledFor (*this, AviatorTokens::kGridGapX);
    const int gapY = AviatorTokens::scaledFor (*this, AviatorTokens::kGridGapY);

    syncReadout.setText ("SYNC\nOff", juce::dontSendNotification);

    std::vector<juce::Component*> items = ptrs (modKnobs);
    items.push_back (&syncReadout);
    CockpitLayout::layoutGrid (area, items, 3, cellW, cellH, gapX, gapY);
}

void CockpitCrossworldPanel::layoutEffectsPanel()
{
    const int pad = AviatorTokens::scaledFor (*this, AviatorTokens::kPanelPad);
    const int titleH = AviatorTokens::scaledFor (*this, AviatorTokens::kTitleH);

    auto area = CockpitLayout::contentArea (effectsPanel.getBounds(), titleH, pad);
    const int cellW = AviatorTokens::scaledFor (*this, AviatorTokens::kKnobCellW);
    const int cellH = AviatorTokens::scaledFor (*this, AviatorTokens::kKnobCellH);
    const int gapX = AviatorTokens::scaledFor (*this, AviatorTokens::kGridGapX);
    const int gapY = AviatorTokens::scaledFor (*this, AviatorTokens::kGridGapY);

    std::vector<juce::Component*> items = ptrs (fxKnobs);
    if (motionToggle != nullptr)
        items.insert (items.begin() + 3, motionToggle.get());

    CockpitLayout::layoutGrid (area, items, 2, cellW, cellH, gapX, gapY);
}

void CockpitCrossworldPanel::layoutStatusStrip()
{
    const float sc = AviatorTokens::scaleFor (*this);
    auto strip = headerStatusStrip.getBounds().reduced (AviatorTokens::scaledFor (*this, 4), 2);
    headerStatusStrip.readout.setBounds (strip);
    headerStatusStrip.readout.setFont (AviatorTokens::hud (12.f * sc));
    juce::ignoreUnused (sc);
}

void CockpitCrossworldPanel::layoutZones()
{
    auto bounds = getLocalBounds();
    const int footerH = AviatorTokens::kFooterH;
    footer.setBounds (bounds.removeFromBottom (footerH));
    photoArea = bounds;

    photoBackground.setBounds (photoArea);

    const auto place = [this] (GlassZone& z)
    {
        z.setBounds (AviatorCockpit::zoneRect (z.zoneId).toPixels (photoArea));
    };

    place (leftBrowserPanel);
    place (effectsPanel);
    place (envelopePanel);
    place (modulationPanel);
    place (centerMacroZone);

    auto stripBounds = AviatorCockpit::zoneRect (AviatorCockpit::ZoneId::autopilotStrip)
                           .toPixels (photoArea);
    stripBounds = stripBounds.reduced (AviatorTokens::scaledFor (*this, 3), 0)
                             .withTrimmedTop (AviatorTokens::scaledFor (*this, 8))
                             .withTrimmedBottom (AviatorTokens::scaledFor (*this, 8));
    headerStatusStrip.setBounds (stripBounds);

    layoutStatusStrip();
    layoutLeftBrowser();
    layoutCenterMacros();
    layoutEnvelopePanel();
    layoutModulationPanel();
    layoutEffectsPanel();

    headerStatusStrip.toFront (false);
    footer.toFront (false);
}

void CockpitCrossworldPanel::resized()
{
    const float sc = AviatorTokens::scaleFor (*this);
    searchBox.setFont (AviatorTokens::hud (11.f * sc));
    selectedPresetLabel.setFont (AviatorTokens::hud (10.f * sc));
    layoutZones();
}
