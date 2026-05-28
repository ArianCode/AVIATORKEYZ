#include "CockpitCrossworldPanel.h"
#include "PhotoAnchoredKnob.h"
#include "../../PluginProcessor.h"
#include "../../State/StateSchema.h"
#include "../AviatorTokens.h"
#include "../ReverseToggle.h"

namespace
{
bool zoneIsCircular (AviatorCockpit::ZoneId z)
{
    return z == AviatorCockpit::ZoneId::radarAdsr || z == AviatorCockpit::ZoneId::radarLfo;
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

        if (sel)
        {
            g.setColour (AviatorTokens::instrumentCyan().withAlpha (0.25f));
            g.fillRect (0, 0, w, h);
        }

        g.setFont (AviatorTokens::hud (10.f));
        g.setColour (sel ? AviatorTokens::instrumentCyan() : AviatorTokens::textMuted());
        g.drawText (owner.presetDisplayNames[row], 4, 0, w - 8, h, juce::Justification::centredLeft);
    }

    void listBoxItemClicked (int row, const juce::MouseEvent&) override
    {
        owner.processor.getPresetManager().loadPresetByFlatIndex (row);
        owner.refreshPresetUI();
    }
};

CockpitCrossworldPanel::GlassZone::GlassZone (AviatorCockpit::ZoneId id)
    : zoneId (id)
{
    setOpaque (false);
    switch (id)
    {
        case AviatorCockpit::ZoneId::leftMfd:         title = "ENGINE / OSC"; break;
        case AviatorCockpit::ZoneId::rightMfd:        title = "FX / EFFECTS"; break;
        case AviatorCockpit::ZoneId::radarAdsr:       title = "ADSR"; break;
        case AviatorCockpit::ZoneId::radarLfo:        title = "LFO / MOD"; break;
        case AviatorCockpit::ZoneId::autopilotStrip:  title = ""; break;
        case AviatorCockpit::ZoneId::overhead:        title = ""; break;
        case AviatorCockpit::ZoneId::throttleQuadrant: title = ""; break;
    }
}

void CockpitCrossworldPanel::GlassZone::paint (juce::Graphics& g)
{
    AviatorTokens::paintGlassPanel (g, getLocalBounds().toFloat().reduced (1.f), zoneIsCircular (zoneId));

    if (title.isNotEmpty())
    {
        g.setFont (AviatorTokens::hud (9.f));
        g.setColour (AviatorTokens::instrumentCyan().withAlpha (0.85f));
        g.drawText (title, getLocalBounds().reduced (4).removeFromTop (12),
                    juce::Justification::centredLeft);
    }
}

CockpitCrossworldPanel::CockpitCrossworldPanel (AviatorKeyzProcessor& p)
    : processor (p)
    , presetList ("presets", nullptr)
    , searchBox()
    , engineReadout ("engine", "")
    , stripReadout ("strip", "")
    , presetModel (std::make_unique<PresetListModel> (*this))
{
    setOpaque (true);
    addAndMakeVisible (photoBackground);

    for (auto* z : { &leftMfd, &rightMfd, &radarAdsr, &radarLfo,
                     &autopilotStrip, &overheadZone, &throttleZone })
        addAndMakeVisible (*z);

    presetList.setModel (presetModel.get());
    presetList.setColour (juce::ListBox::backgroundColourId, juce::Colours::transparentBlack);
    presetList.setOutlineThickness (0);
    addAndMakeVisible (presetList);

    searchBox.setTextToShowWhenEmpty ("SEARCH PRESETS…", AviatorTokens::textMuted());
    searchBox.setFont (AviatorTokens::hud (10.f));
    searchBox.setColour (juce::TextEditor::backgroundColourId, juce::Colours::transparentBlack);
    searchBox.setColour (juce::TextEditor::outlineColourId, AviatorTokens::instrumentCyan().withAlpha (0.3f));
    addAndMakeVisible (searchBox);

    engineReadout.setFont (AviatorTokens::hud (10.f));
    engineReadout.setColour (juce::Label::textColourId, AviatorTokens::mfdAmber());
    engineReadout.setJustificationType (juce::Justification::topLeft);
    addAndMakeVisible (engineReadout);

    stripReadout.setFont (AviatorTokens::hud (11.f));
    stripReadout.setColour (juce::Label::textColourId, AviatorTokens::instrumentCyan());
    stripReadout.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (stripReadout);

    addAndMakeVisible (footer);
    footer.onSettingsClicked = [this] {
        if (onLibraryRequested)
            onLibraryRequested();
    };
    footer.onAboutClicked = [this] {
        if (onAboutRequested)
            onAboutRequested();
    };

    layoutPhysicalControls();
    buildPresetList();
    refreshPresetUI();
    startTimerHz (30);
}

CockpitCrossworldPanel::~CockpitCrossworldPanel() = default;

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

void CockpitCrossworldPanel::layoutPhysicalControls()
{
    photoKnobs.clear();
    photoToggles.clear();
    photoButtons.clear();
    photoLevers.clear();
    leverAttachments.clear();
    anchoredWidgets.clear();
    anchoredIndices.clear();

    auto& apvts = processor.getAPVTS();

    auto registerWidget = [this] (int anchorIndex, juce::Component& c)
    {
        anchoredIndices.push_back (anchorIndex);
        anchoredWidgets.push_back (&c);
    };

    for (int i = 0; i < AviatorCockpit::getKnobAnchorCount(); ++i)
    {
        const auto& a = AviatorCockpit::getKnobAnchors()[i];

        if (a.kind == AviatorCockpit::AnchorKind::rotary && a.paramId != nullptr)
        {
            auto knob = std::make_unique<PhotoAnchoredKnob> (apvts, a.paramId, formatForParam (a.paramId));
            addAndMakeVisible (*knob);
            registerWidget (i, *knob);
            photoKnobs.push_back (std::move (knob));
        }
        else if (a.kind == AviatorCockpit::AnchorKind::toggle)
        {
            if (a.paramId != nullptr && juce::String (a.paramId) == AviatorKeyz::ParamID::REVERSE)
            {
                auto t = std::make_unique<ReverseToggle> (apvts, AviatorKeyz::ParamID::REVERSE);
                addAndMakeVisible (*t);
                registerWidget (i, *t);
                photoToggles.push_back (std::move (t));
            }
            else if (a.action != nullptr)
            {
                auto b = std::make_unique<juce::TextButton> (a.label);
                b->setColour (juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
                b->onClick = [this, act = juce::String (a.action)] { handleAnchorAction (act.toRawUTF8()); };
                addAndMakeVisible (*b);
                registerWidget (i, *b);
                photoButtons.push_back (std::move (b));
            }
        }
        else if (a.kind == AviatorCockpit::AnchorKind::pushButton
                 || a.kind == AviatorCockpit::AnchorKind::guardedDome)
        {
            auto b = std::make_unique<juce::TextButton> (a.label);
            b->setColour (juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
            if (a.kind == AviatorCockpit::AnchorKind::guardedDome)
                b->setColour (juce::TextButton::textColourOffId, AviatorTokens::mfdAmber());

            const juce::String act = a.action != nullptr ? a.action : "";
            b->onClick = [this, act] { handleAnchorAction (act.toRawUTF8()); };
            addAndMakeVisible (*b);
            registerWidget (i, *b);
            photoButtons.push_back (std::move (b));
        }
        else if (a.kind == AviatorCockpit::AnchorKind::verticalLever && a.paramId != nullptr)
        {
            auto s = std::make_unique<juce::Slider> (juce::Slider::LinearVertical, juce::Slider::NoTextBox);
            s->setColour (juce::Slider::trackColourId, AviatorTokens::instrumentCyan().withAlpha (0.2f));
            s->setColour (juce::Slider::thumbColourId, AviatorTokens::instrumentCyan());
            leverAttachments.push_back (
                std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
                    apvts, a.paramId, *s));
            addAndMakeVisible (*s);
            registerWidget (i, *s);
            photoLevers.push_back (std::move (s));
        }
    }
}

void CockpitCrossworldPanel::handleAnchorAction (const char* action)
{
    if (action == nullptr)
        return;

    const juce::String a (action);
    auto& pm = processor.getPresetManager();

    if (a == "presetPrev")
        pm.loadAdjacentPreset (-1);
    else if (a == "presetNext")
        pm.loadAdjacentPreset (+1);
    else if (a == "library")
    {
        if (onLibraryRequested)
            onLibraryRequested();
    }
    else if (a == "panic")
    {
        if (auto* p = processor.getAPVTS().getParameter (AviatorKeyz::ParamID::REVERSE))
            p->setValueNotifyingHost (0.f);
    }
    else if (a == "reverbBypass")
    {
        if (auto* p = processor.getAPVTS().getParameter (AviatorKeyz::ParamID::REVERB_AMOUNT))
            p->setValueNotifyingHost (p->getValue() < 0.01f ? 0.25f : 0.f);
    }
    else if (a == "emergencyBurst")
    {
        if (auto* p = processor.getAPVTS().getParameter (AviatorKeyz::ParamID::SMEAR))
            p->setValueNotifyingHost (1.f);
    }

    refreshPresetUI();
}

void CockpitCrossworldPanel::buildPresetList()
{
    presetDisplayNames.clear();
    auto& pm = processor.getPresetManager();

    for (const auto& category : pm.getAllCategories())
        for (const auto& name : pm.getPresetsForCategory (category))
            presetDisplayNames.add (name + " · " + category);

    presetList.updateContent();
}

void CockpitCrossworldPanel::refreshPresetUI()
{
    auto& pm = processor.getPresetManager();
    const auto name = pm.getCurrentPresetName();
    const auto cat  = pm.getCurrentCategory();

    engineReadout.setText (cat.toUpperCase() + "\n" + name.toUpperCase(), juce::dontSendNotification);

    stripReadout.setText ("PATCH " + juce::String (pm.getCurrentPresetIndex() + 1).paddedLeft ('0', 2)
                          + "  ·  BPM 128  ·  VOICES 04",
                          juce::dontSendNotification);

    footer.setHudText ("FL " + juce::String (120 + (std::abs (name.hashCode()) % 280)));
    footer.setSampleRate (processor.getSampleRate() > 0 ? processor.getSampleRate() : 44100.0);
    footer.setBlockSize (processor.getBlockSize() > 0 ? processor.getBlockSize() : 256);

    buildPresetList();
    presetList.selectRow (pm.getCurrentPresetIndex());
    repaint();
}

void CockpitCrossworldPanel::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff030309));

    paintFxMeters (g, rightMfd.getBounds());
    paintAdsrCurve (g, radarAdsr.getBounds());
    paintLfoRadar (g, radarLfo.getBounds());
}

void CockpitCrossworldPanel::paintAdsrCurve (juce::Graphics& g, juce::Rectangle<int> bounds)
{
    if (bounds.isEmpty())
        return;

    const float atk = processor.getAPVTS().getRawParameterValue (AviatorKeyz::ParamID::ENV_ATTACK)->load();
    const float rel = processor.getAPVTS().getRawParameterValue (AviatorKeyz::ParamID::ENV_RELEASE)->load();

    juce::Path path;
    const auto r = bounds.toFloat().reduced (14.f);
    path.startNewSubPath (r.getX(), r.getBottom());
    path.lineTo (r.getX() + r.getWidth() * juce::jlimit (0.05f, 0.4f, atk), r.getY() + r.getHeight() * 0.2f);
    path.lineTo (r.getRight() - r.getWidth() * 0.15f, r.getY() + r.getHeight() * 0.2f);
    path.lineTo (r.getRight(), r.getBottom() - r.getHeight() * juce::jlimit (0.05f, 0.5f, rel * 0.3f));

    g.setColour (AviatorTokens::instrumentCyan().withAlpha (0.85f));
    g.strokePath (path, juce::PathStrokeType (1.5f));
}

void CockpitCrossworldPanel::paintLfoRadar (juce::Graphics& g, juce::Rectangle<int> bounds)
{
    if (bounds.isEmpty())
        return;

    const auto r = bounds.toFloat().reduced (12.f);
    const float cx = r.getCentreX();
    const float cy = r.getCentreY();
    const float rad = juce::jmin (r.getWidth(), r.getHeight()) * 0.45f;

    g.setColour (AviatorTokens::instrumentCyan().withAlpha (0.15f));
    for (float ring = 0.25f; ring <= 1.f; ring += 0.25f)
        g.drawEllipse (cx - rad * ring, cy - rad * ring, rad * ring * 2.f, rad * ring * 2.f, 1.f);

    const float angle = lfoSweep;
    juce::Path arm;
    arm.startNewSubPath (cx, cy);
    arm.lineTo (cx + std::cos (angle) * rad, cy + std::sin (angle) * rad);
    g.setColour (AviatorTokens::mfdAmber().withAlpha (0.9f));
    g.strokePath (arm, juce::PathStrokeType (1.5f));

    auto labelArea = r;
    g.setFont (AviatorTokens::hud (8.f));
    g.setColour (AviatorTokens::textMuted());
    g.drawText ("MOD · GLIDE/SMEAR", labelArea.removeFromBottom (12), juce::Justification::centred);
}

void CockpitCrossworldPanel::paintFxMeters (juce::Graphics& g, juce::Rectangle<int> bounds)
{
    if (bounds.isEmpty())
        return;

    const float rev = processor.getAPVTS().getRawParameterValue (AviatorKeyz::ParamID::REVERB_AMOUNT)->load();
    const float smear = processor.getAPVTS().getRawParameterValue (AviatorKeyz::ParamID::SMEAR)->load();
    const float width = processor.getAPVTS().getRawParameterValue (AviatorKeyz::ParamID::STEREO_WIDTH)->load();

    auto area = bounds.reduced (8);
    area.removeFromTop (14);
    const int barW = 6;
    const int gap = 4;
    const float vals[] = { rev, smear, width };
    const char* labels[] = { "RV", "SM", "WD" };

    for (int i = 0; i < 3; ++i)
    {
        auto col = area.removeFromLeft (barW);
        area.removeFromLeft (gap);
        const int h = (int) ((float) col.getHeight() * juce::jlimit (0.f, 1.f, vals[i]));
        g.setColour (AviatorTokens::instrumentCyan().withAlpha (0.2f));
        g.fillRect (col.withTrimmedTop (col.getHeight() - h));
        g.setColour (AviatorTokens::instrumentCyan());
        g.drawRect (col, 1);
        g.setFont (AviatorTokens::hud (7.f));
        g.drawText (labels[i], col.getX() - 2, col.getBottom() + 2, barW + 4, 10, juce::Justification::centred);
    }
}

void CockpitCrossworldPanel::timerCallback()
{
    lfoSweep += 0.08f;
    if (lfoSweep > juce::MathConstants<float>::twoPi)
        lfoSweep -= juce::MathConstants<float>::twoPi;
    repaint (radarLfo.getBounds());
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

    place (leftMfd);
    place (rightMfd);
    place (radarAdsr);
    place (radarLfo);
    place (autopilotStrip);
    place (overheadZone);
    place (throttleZone);

    auto mfd = leftMfd.getBounds().reduced (4);
    mfd.removeFromTop (12);
    searchBox.setBounds (mfd.removeFromTop (18));
    mfd.removeFromTop (2);
    presetList.setBounds (mfd.removeFromTop (mfd.getHeight() - 36));
    engineReadout.setBounds (mfd);

    stripReadout.setBounds (autopilotStrip.getBounds().reduced (2));

    for (size_t j = 0; j < anchoredWidgets.size(); ++j)
    {
        const auto& a = AviatorCockpit::getKnobAnchors()[anchoredIndices[j]];
        auto b = AviatorCockpit::anchorBounds (photoArea, a);

        if (a.kind == AviatorCockpit::AnchorKind::verticalLever)
        {
            auto leverBounds = b.withHeight (juce::jmax (48, b.getHeight() * 4));
            leverBounds.setY (b.getCentreY() - leverBounds.getHeight() / 2);
            b = leverBounds;
        }

        anchoredWidgets[j]->setBounds (b);
        anchoredWidgets[j]->toFront (false);
    }
}

void CockpitCrossworldPanel::resized()
{
    layoutZones();
}
