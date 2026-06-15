#include "AdvancedPageContent.h"

namespace
{
namespace P = AviatorKeyz::ParamID;

constexpr int kTopRowH   = 420;
constexpr int kDividerH  = 2;

void styleModHdr (juce::Label& lbl)
{
    lbl.setFont (AviatorTokens::hudBold (8.f));
    lbl.setColour (juce::Label::textColourId, AdvancedWidgets::kGold());
    lbl.setJustificationType (juce::Justification::centredLeft);
}
} // namespace

AdvancedPageContent::AdvancedPageContent (juce::AudioProcessorValueTreeState& apvts)
    : apvtsRef (apvts)
{
    const juce::StringArray oscTypes { "Saw", "Sq", "Tri", "Sin", "Nse", "WT", "FM", "Chd" };

    osc1Wave = std::make_unique<OscWaveformDisplay> (apvts, P::OSC1_TYPE, P::OSC1_SHAPE, AdvancedWidgets::kCyanWave());
    osc2Wave = std::make_unique<OscWaveformDisplay> (apvts, P::OSC2_TYPE, P::OSC2_SHAPE, AdvancedWidgets::kCyanWave());
    osc1Types = std::make_unique<AdvancedWidgets::ChoiceToggleRow> (apvts, P::OSC1_TYPE, oscTypes, 9101);
    osc2Types = std::make_unique<AdvancedWidgets::ChoiceToggleRow> (apvts, P::OSC2_TYPE, oscTypes, 9102);
    addAndMakeVisible (*osc1Wave);
    addAndMakeVisible (*osc2Wave);
    addAndMakeVisible (*osc1Types);
    addAndMakeVisible (*osc2Types);

    addBox (P::OSC1_TUNE,  "TUNE",  Fmt::semitones);
    addBox (P::OSC1_FINE,  "FINE",  Fmt::cents);
    addBox (P::OSC1_SHAPE, "SHAPE", Fmt::percent);
    addBox (P::OSC1_LEVEL, "LEVEL", Fmt::percent);
    addBox (P::OSC1_PAN,   "PAN",   Fmt::pan);
    addBox (P::OSC2_TUNE,  "TUNE",  Fmt::semitones);
    addBox (P::OSC2_FINE,  "FINE",  Fmt::cents);
    addBox (P::OSC2_SHAPE, "SHAPE", Fmt::percent);
    addBox (P::OSC2_LEVEL, "LEVEL", Fmt::percent);
    addBox (P::OSC2_PAN,   "PAN",   Fmt::pan);
    addBox (P::SOURCE_BLEND, "BLEND", Fmt::percent);
    addBox (P::INPUT_GAIN, "GAIN", Fmt::decibels);
    addBox (P::VELOCITY_SENSITIVITY, "VEL", Fmt::percent);

    filterCurve = std::make_unique<FilterCurveGraph> (apvts,
                                                      P::FILTER_CUTOFF,
                                                      P::FILTER_RESONANCE,
                                                      P::FILTER_TYPE,
                                                      P::FILTER_ENABLED);
    addAndMakeVisible (*filterCurve);

    filterEnabled = std::make_unique<AdvancedWidgets::FlatToggle> (apvts,
                                                                   P::FILTER_ENABLED,
                                                                   "FILTER ON",
                                                                   "FILTER OFF");
    addAndMakeVisible (*filterEnabled);

    addBox (P::FILTER_DRIVE,     "DRIVE", Fmt::percent);
    addBox (P::ENV_FLT_AMOUNT,   "FLT",   Fmt::percent);
    addBox (P::ENV_ATTACK,       "ATK",   Fmt::ms);
    addBox (P::ENV_AMP_DECAY,    "DEC",   Fmt::ms);
    addBox (P::ENV_AMP_SUSTAIN,  "SUS",   Fmt::percent);
    addBox (P::ENV_RELEASE,      "REL",   Fmt::ms);
    addBox (P::GLIDE_TIME,       "GLIDE", Fmt::ms);

    lfo1Wave = std::make_unique<LfoWaveformDisplay> (apvts, P::LFO1_SHAPE, P::LFO1_PHASE);
    lfo2Wave = std::make_unique<LfoWaveformDisplay> (apvts, P::LFO2_SHAPE, P::LFO2_PHASE);
    lfo3Wave = std::make_unique<LfoWaveformDisplay> (apvts, P::LFO3_SHAPE, P::LFO3_PHASE);
    addAndMakeVisible (*lfo1Wave);
    addAndMakeVisible (*lfo2Wave);
    addAndMakeVisible (*lfo3Wave);

    addBox (P::LFO1_RATE,  "RATE",  Fmt::hz);
    addBox (P::LFO1_DEPTH, "DEPTH", Fmt::percent);
    addBox (P::LFO1_PHASE, "PHASE", Fmt::percent);
    addBox (P::LFO2_RATE,  "RATE",  Fmt::hz);
    addBox (P::LFO2_DEPTH, "DEPTH", Fmt::percent);
    addBox (P::LFO2_PHASE, "PHASE", Fmt::percent);
    addBox (P::LFO3_RATE,  "RATE",  Fmt::hz);
    addBox (P::LFO3_DEPTH, "DEPTH", Fmt::percent);
    addBox (P::LFO3_PHASE, "PHASE", Fmt::percent);

    const juce::StringArray lfoShapes { "Sine", "Square", "Tri", "Ramp Up", "Ramp Down", "Random" };
    for (auto* cb : { &lfo1Shape, &lfo2Shape, &lfo3Shape })
    {
        cb->addItemList (lfoShapes, 1);
        AdvancedWidgets::styleCombo (*cb);
        addAndMakeVisible (cb);
    }
    lfo1ShapeA = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (apvts, P::LFO1_SHAPE, lfo1Shape);
    lfo2ShapeA = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (apvts, P::LFO2_SHAPE, lfo2Shape);
    lfo3ShapeA = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (apvts, P::LFO3_SHAPE, lfo3Shape);

    lfo1Sync = std::make_unique<AdvancedWidgets::FlatToggle> (apvts, P::LFO1_SYNC, "SYNC ON", "SYNC OFF");
    lfo2Sync = std::make_unique<AdvancedWidgets::FlatToggle> (apvts, P::LFO2_SYNC, "SYNC ON", "SYNC OFF");
    lfo3Sync = std::make_unique<AdvancedWidgets::FlatToggle> (apvts, P::LFO3_SYNC, "SYNC ON", "SYNC OFF");
    addAndMakeVisible (*lfo1Sync);
    addAndMakeVisible (*lfo2Sync);
    addAndMakeVisible (*lfo3Sync);

    textureSection = std::make_unique<TextureSectionComponent> (apvts);
    addAndMakeVisible (*textureSection);

    performanceMacros = std::make_unique<PerformanceMacroStrip> (apvts);
    addAndMakeVisible (*performanceMacros);
    performanceMacros->setVisible (false);

    const auto sources = ModMatrix::sourceNames();
    const auto dests   = ModMatrix::destNames();
    for (int i = 0; i < (int) modRows.size(); ++i)
    {
        auto& row = modRows[(size_t) i];
        const auto& ids = ModRoutingHub::kRows[i];
        row.sourceBox.addItemList (sources, 1);
        row.destBox.addItemList (dests, 1);
        AdvancedWidgets::styleCombo (row.sourceBox);
        AdvancedWidgets::styleCombo (row.destBox);
        row.sourceLabel.setFont (AviatorTokens::hudBold (9.f));
        row.destLabel.setFont (AviatorTokens::hudBold (9.f));
        row.sourceLabel.setJustificationType (juce::Justification::centredLeft);
        row.destLabel.setJustificationType (juce::Justification::centredLeft);
        row.amountSlider = std::make_unique<ModAmountSlider> (apvts, ids.amount, AdvancedWidgets::kCyanWave());
        row.srcA = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (apvts, ids.source, row.sourceBox);
        row.dstA = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (apvts, ids.dest, row.destBox);
        row.onA  = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (apvts, ids.on, row.onBtn);
        row.onBtn.setClickingTogglesState (true);
        row.deleteBtn.onClick = [this, i]
        {
            ModRoutingHub::clearRow (apvtsRef, i);
            resized();
        };
        row.deleteBtn.setColour (juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
        row.deleteBtn.setColour (juce::TextButton::textColourOffId, AdvancedWidgets::kGold().withAlpha (0.75f));
        addAndMakeVisible (row.onBtn);
        addAndMakeVisible (row.deleteBtn);
        addAndMakeVisible (row.sourceBox);
        addAndMakeVisible (row.destBox);
        addAndMakeVisible (row.sourceLabel);
        addAndMakeVisible (row.destLabel);
        addAndMakeVisible (*row.amountSlider);
    }

    addModBtn.onClick = [this]
    {
        for (int r = 0; r < ModRoutingHub::kNumRows; ++r)
        {
            if (isModRowActive (r))
                continue;

            ModRoutingHub::assignRow (apvtsRef, r, 1, ModDest::smear, 0.35f, true);
            setSection (Section::matrix);
            resized();
            break;
        }
    };
    addAndMakeVisible (addModBtn);

    for (auto* lbl : { &modHdrOn, &modHdrRoute, &modHdrAmt })
    {
        styleModHdr (*lbl);
        addAndMakeVisible (lbl);
    }

    for (auto* tab : { &tabTexture, &tabMatrix, &tabFx, &tabUtility })
    {
        tab->setClickingTogglesState (true);
        tab->setRadioGroupId (93001);
        addAndMakeVisible (tab);
    }
    tabTexture.setToggleState (true, juce::dontSendNotification);
    tabTexture.onClick = [this] { setSection (Section::textureEngine); };
    tabMatrix.onClick  = [this] { setSection (Section::matrix); };
    tabFx.onClick      = [this] { setSection (Section::fxRouting); };
    tabUtility.onClick = [this] { setSection (Section::utility); };

    addBox (P::OUTPUT_GAIN,   "GAIN", Fmt::decibels);
    addBox (P::STEREO_WIDTH,  "WIDTH", Fmt::plain);
    outLimiter = std::make_unique<AdvancedWidgets::FlatToggle> (apvts, P::OUTPUT_LIMITER, "ON", "OFF");
    addAndMakeVisible (*outLimiter);

    addBox (P::VOICE_POLYPHONY, "POLY", Fmt::integer);
    voiceGlideBox = std::make_unique<BV> (apvts, P::GLIDE_TIME, "GLIDE", Fmt::ms, AdvancedWidgets::kGold());
    addAndMakeVisible (*voiceGlideBox);
    voiceMode = std::make_unique<AdvancedWidgets::ChoiceToggleRow> (apvts, P::VOICE_PLAY_MODE,
                                                                    juce::StringArray { "POLY", "MONO", "LEGATO" }, 9201);
    addAndMakeVisible (*voiceMode);

    phraseEnabled   = std::make_unique<AdvancedWidgets::FlatToggle> (apvts, P::PHRASE_ENABLED,   "ON", "ON");
    phraseTempoSync = std::make_unique<AdvancedWidgets::FlatToggle> (apvts, P::PHRASE_TEMPO_SYNC, "TEMPO", "TEMPO");
    phraseKeySync   = std::make_unique<AdvancedWidgets::FlatToggle> (apvts, P::PHRASE_KEY_SYNC,   "KEY", "KEY");
    phraseLoop      = std::make_unique<AdvancedWidgets::FlatToggle> (apvts, P::PHRASE_LOOP,       "LOOP", "LOOP");
    addAndMakeVisible (*phraseEnabled);
    addAndMakeVisible (*phraseTempoSync);
    addAndMakeVisible (*phraseKeySync);
    addAndMakeVisible (*phraseLoop);

    phraseOneShot.setClickingTogglesState (true);
    phraseOneShot.onClick = [this]
    {
        if (auto* p = apvtsRef.getParameter (P::PHRASE_LOOP))
            p->setValueNotifyingHost (phraseOneShot.getToggleState() ? 0.f : 1.f);
        AdvancedWidgets::stylePillButton (phraseOneShot, phraseOneShot.getToggleState());
    };
    addAndMakeVisible (phraseOneShot);

    addBox (P::PHRASE_START,  "START",  Fmt::percent);
    addBox (P::PHRASE_LENGTH, "LENGTH", Fmt::percent);
    addBox (P::PHRASE_PITCH,  "PITCH",  Fmt::semitones);

    fxReverbOn = std::make_unique<AdvancedWidgets::FxEnableButton> (apvts, P::FX_REVERB_ON,  "REVERB");
    fxDelayOn  = std::make_unique<AdvancedWidgets::FxEnableButton> (apvts, P::FX_DELAY_ON,   "DELAY");
    fxChorusOn = std::make_unique<AdvancedWidgets::FxEnableButton> (apvts, P::FX_CHORUS_ON,  "CHORUS");
    fxLofiOn   = std::make_unique<AdvancedWidgets::FxEnableButton> (apvts, P::FX_LOFI_ON,    "LO-FI");
    fxDistOn   = std::make_unique<AdvancedWidgets::FxEnableButton> (apvts, P::FX_DIST_ON,    "DIST");
    fxDelaySync = std::make_unique<AdvancedWidgets::FlatToggle> (apvts, P::FX_DELAY_SYNC, "SYNC ON", "SYNC OFF");
    addAndMakeVisible (*fxReverbOn);
    addAndMakeVisible (*fxDelayOn);
    addAndMakeVisible (*fxChorusOn);
    addAndMakeVisible (*fxLofiOn);
    addAndMakeVisible (*fxDistOn);
    addAndMakeVisible (*fxDelaySync);

    addBox (P::REVERB_AMOUNT,     "MIX",  Fmt::percent);
    addBox (P::REVERB_SIZE,       "SIZE", Fmt::percent);
    addBox (P::FX_REVERB_DAMP,    "DAMP", Fmt::percent);
    addBox (P::FX_DELAY_TIME,     "TIME", Fmt::plain);
    addBox (P::FX_DELAY_FEEDBACK, "FB",   Fmt::percent);
    addBox (P::FX_DELAY_MIX,      "MIX",  Fmt::percent);
    addBox (P::FX_CHORUS_RATE,    "RATE", Fmt::percent);
    addBox (P::FX_CHORUS_DEPTH,   "DPTH", Fmt::percent);
    addBox (P::FX_CHORUS_MIX,     "MIX",  Fmt::percent);
    addBox (P::FX_LOFI_AMOUNT,    "AMT",  Fmt::percent);
    addBox (P::FX_DIST_DRIVE,     "DRV",  Fmt::percent);

    fxEditsToggle = std::make_unique<AdvancedWidgets::FlatToggle> (apvts, P::FX_EDITS_ON, "FX ON", "FX OFF");
    addAndMakeVisible (*fxEditsToggle);

    const auto enableFilterOnUserEdit = [this]
    {
        if (auto* param = apvtsRef.getParameter (P::FILTER_ENABLED))
            param->setValueNotifyingHost (1.f);
    };

    for (const char* id : { P::FILTER_DRIVE, P::ENV_FLT_AMOUNT })
        if (auto* b = box (id))
            b->onUserAdjust = enableFilterOnUserEdit;

    setSection (Section::textureEngine);
    startTimerHz (12);
}

void AdvancedPageContent::addBox (const char* id, const juce::String& label, Fmt fmt)
{
    auto b = std::make_unique<BV> (apvtsRef, id, label, fmt, AdvancedWidgets::kGold());
    boxMap[id] = b.get();
    addAndMakeVisible (*b);
    allBoxes.push_back (std::move (b));
}

AdvancedPageContent::BV* AdvancedPageContent::box (const char* id) const
{
    const auto it = boxMap.find (id);
    return it != boxMap.end() ? it->second : nullptr;
}

void AdvancedPageContent::placeBox (BV* b, juce::Rectangle<int> r) const
{
    if (b != nullptr)
        b->setBounds (r);
}

void AdvancedPageContent::placeRow (juce::Rectangle<int> row,
                                    const std::vector<BV*>& boxes,
                                    int boxW,
                                    int gap) const
{
    int x = row.getX();
    for (auto* b : boxes)
    {
        placeBox (b, { x, row.getY(), boxW, row.getHeight() });
        x += boxW + gap;
    }
}

void AdvancedPageContent::styleModOnBtn (juce::TextButton& btn, bool on) const
{
    btn.setButtonText (on ? juce::String (juce::CharPointer_UTF8 ("●")) : "○");
    AdvancedWidgets::stylePillButton (btn, on);
}

void AdvancedPageContent::selectModSource (int sourceIndex)
{
    if (sourceIndex < 1 || sourceIndex > 3)
        return;

    selectedModSourceIndex = sourceIndex;
    resized();
}

int AdvancedPageContent::modChoiceIndex (const char* paramId) const
{
    if (auto* p = dynamic_cast<juce::AudioParameterChoice*> (apvtsRef.getParameter (paramId)))
        return p->getIndex();
    return 0;
}

bool AdvancedPageContent::isModRowActive (int rowIndex) const
{
    if (! juce::isPositiveAndBelow (rowIndex, ModRoutingHub::kNumRows))
        return false;

    const auto& ids = ModRoutingHub::kRows[rowIndex];
    const int destIdx = modChoiceIndex (ids.dest);
    return destIdx > 0;
}

void AdvancedPageContent::setSection (Section section)
{
    activeSection = section;

    if (section == Section::matrix)
    {
        for (int i = 0; i < (int) modRows.size(); ++i)
        {
            if (! isModRowActive (i))
                continue;

            const int srcIdx = modChoiceIndex (ModRoutingHub::kRows[i].source);
            if (srcIdx >= 1 && srcIdx <= 3)
            {
                selectedModSourceIndex = srcIdx;
                break;
            }
        }
    }

    styleSectionTab (tabTexture, section == Section::textureEngine);
    styleSectionTab (tabMatrix,  section == Section::matrix);
    styleSectionTab (tabFx,      section == Section::fxRouting);
    styleSectionTab (tabUtility, section == Section::utility);
    tabTexture.setToggleState (section == Section::textureEngine, juce::dontSendNotification);
    tabMatrix.setToggleState (section == Section::matrix, juce::dontSendNotification);
    tabFx.setToggleState (section == Section::fxRouting, juce::dontSendNotification);
    tabUtility.setToggleState (section == Section::utility, juce::dontSendNotification);
    resized();
}

void AdvancedPageContent::styleSectionTab (juce::TextButton& btn, bool active) const
{
    btn.setColour (juce::TextButton::buttonColourId,
                   active ? AviatorTokens::instrumentCyan().withAlpha (0.22f) : juce::Colour (0xff0d1f33));
    btn.setColour (juce::TextButton::textColourOffId,
                   active ? AviatorTokens::instrumentCyan() : AviatorTokens::textMuted());
}

void AdvancedPageContent::layoutModulatorFocus (juce::Rectangle<int> focusArea,
                                                const std::function<int (int)>& rh)
{
    struct LfoUi
    {
        LfoWaveformDisplay* wave;
        juce::ComboBox* shape;
        AdvancedWidgets::FlatToggle* sync;
        const char* rateId;
        const char* depthId;
        const char* phaseId;
        const char* title;
    };

    const LfoUi lfos[] {
        { lfo1Wave.get(), &lfo1Shape, lfo1Sync.get(), P::LFO1_RATE, P::LFO1_DEPTH, P::LFO1_PHASE, "LFO 1" },
        { lfo2Wave.get(), &lfo2Shape, lfo2Sync.get(), P::LFO2_RATE, P::LFO2_DEPTH, P::LFO2_PHASE, "LFO 2" },
        { lfo3Wave.get(), &lfo3Shape, lfo3Sync.get(), P::LFO3_RATE, P::LFO3_DEPTH, P::LFO3_PHASE, "LFO 3" },
    };

    lfo1Wave->setVisible (false);
    lfo2Wave->setVisible (false);
    lfo3Wave->setVisible (false);
    lfo1Shape.setVisible (false);
    lfo2Shape.setVisible (false);
    lfo3Shape.setVisible (false);
    lfo1Sync->setVisible (false);
    lfo2Sync->setVisible (false);
    lfo3Sync->setVisible (false);
    for (const char* id : { P::LFO1_RATE, P::LFO1_DEPTH, P::LFO1_PHASE, P::LFO2_RATE, P::LFO2_DEPTH, P::LFO2_PHASE,
                            P::LFO3_RATE, P::LFO3_DEPTH, P::LFO3_PHASE })
        placeBox (box (id), {});

    const int idx = juce::jlimit (0, 2, selectedModSourceIndex - 1);
    const auto& ui = lfos[idx];
    regions.push_back ({ "ACTIVE MODULATOR — " + juce::String (ui.title), focusArea });

    auto b = focusArea.reduced (8, 0).withTrimmedTop (rh (14));
    ui.wave->setVisible (true);
    ui.wave->setBounds (b.removeFromTop (rh (190)));
    b.removeFromTop (rh (8));

    const int bw = juce::jmax (rh (56), (b.getWidth() - rh (8)) / 3);
    auto paramRow = b.removeFromTop (rh (56));
    placeBox (box (ui.rateId),  paramRow.removeFromLeft (bw));
    paramRow.removeFromLeft (rh (4));
    placeBox (box (ui.depthId), paramRow.removeFromLeft (bw));
    paramRow.removeFromLeft (rh (4));
    placeBox (box (ui.phaseId), paramRow.removeFromLeft (bw));
    b.removeFromTop (rh (8));

    ui.shape->setVisible (true);
    ui.sync->setVisible (true);
    ui.shape->setBounds (b.removeFromTop (rh (22)));
    b.removeFromTop (rh (4));
    ui.sync->setBounds (b.removeFromTop (rh (18)));
}

void AdvancedPageContent::layoutModMatrixZone (juce::Rectangle<int> modArea, int rowHpx)
{
    auto b = modArea.reduced (8, 0).withTrimmedTop (18);
    auto hdr = b.removeFromTop (rowHpx);
    const int deleteW = rowHpx;
    modHdrOn.setBounds (hdr.removeFromLeft (rowHpx));
    modHdrRoute.setBounds (hdr.removeFromLeft (hdr.getWidth() - rowHpx * 2 - deleteW));
    modHdrAmt.setBounds (hdr.removeFromLeft (rowHpx * 2));
    hdr.removeFromLeft (deleteW);

    const auto sources = ModMatrix::sourceNames();
    const auto dests   = ModMatrix::destNames();

    for (int i = 0; i < (int) modRows.size(); ++i)
    {
        auto& row = modRows[(size_t) i];
        const bool active = isModRowActive (i);

        if (! active)
        {
            row.rowBounds = {};
            row.onBtn.setBounds ({});
            row.deleteBtn.setBounds ({});
            row.sourceBox.setBounds ({});
            row.destBox.setBounds ({});
            row.sourceLabel.setBounds ({});
            row.destLabel.setBounds ({});
            if (row.amountSlider != nullptr)
                row.amountSlider->setBounds ({});
            continue;
        }

        auto r = b.removeFromTop (rowHpx + 8);
        row.rowBounds = r;

        const float hoverAlpha = hoveredModRowIndex == i ? 1.f : 0.28f;
        row.deleteBtn.setAlpha (hoverAlpha);
        row.onBtn.setAlpha (hoverAlpha);

        row.deleteBtn.setBounds (r.removeFromRight (deleteW).withSizeKeepingCentre (deleteW - 4, rowHpx - 4));
        row.onBtn.setBounds (r.removeFromLeft (rowHpx).withSizeKeepingCentre (rowHpx - 4, rowHpx - 4));

        const int srcIdx = modChoiceIndex (ModRoutingHub::kRows[i].source);
        const int dstIdx = modChoiceIndex (ModRoutingHub::kRows[i].dest);
        const juce::Colour srcColour = ModRoutingHub::colourForSource (srcIdx);
        row.sourceLabel.setText (sources[juce::jlimit (0, sources.size() - 1, srcIdx)],
                                 juce::dontSendNotification);
        row.sourceLabel.setColour (juce::Label::textColourId,
                                   selectedModSourceIndex == srcIdx ? AdvancedWidgets::kGold() : srcColour);
        row.destLabel.setText (dests[juce::jlimit (0, dests.size() - 1, dstIdx)], juce::dontSendNotification);
        row.destLabel.setColour (juce::Label::textColourId, AviatorTokens::textPrimary());

        row.sourceLabel.setBounds (r.removeFromLeft (rowHpx * 2));
        if (row.amountSlider != nullptr)
        {
            row.amountSlider->setAccentColour (srcColour);
            row.amountSlider->setBounds (r.removeFromLeft (r.getWidth() - rowHpx * 2));
        }
        row.destLabel.setBounds (r);

        row.sourceBox.setBounds ({});
        row.destBox.setBounds ({});
    }

    addModBtn.setBounds (b.removeFromTop (rowHpx + 6).withSizeKeepingCentre (juce::jmin (220, modArea.getWidth() - 16), rowHpx));
    AdvancedWidgets::stylePillButton (addModBtn, false);
}

void AdvancedPageContent::paint (juce::Graphics& g)
{
    g.fillAll (AdvancedWidgets::kNavyBg());

    const float sy = (float) getHeight() / (float) kDesignHeight;
    const auto rh  = [sy] (int px) { return juce::roundToInt ((float) px * sy); };

    if (activeSection == Section::textureEngine)
    {
        const int divY1 = rh (kTopRowH);
        AdvancedWidgets::paintDivider (g, { 0, divY1, getWidth(), rh (kDividerH) });
    }

    if (activeSection == Section::matrix)
    {
        for (int i = 0; i < (int) modRows.size(); ++i)
        {
            if (hoveredModRowIndex != i || ! isModRowActive (i))
                continue;

            const auto& rb = modRows[(size_t) i].rowBounds;
            if (rb.isEmpty())
                continue;

            g.setColour (AviatorTokens::instrumentCyan().withAlpha (0.07f));
            g.fillRect (rb);
        }
    }

    for (const auto& r : regions)
    {
        if (r.title.isNotEmpty())
        {
            auto header = r.bounds.withHeight (AviatorTokens::scaledFor (*this, 14));
            AdvancedWidgets::paintSectionHeader (g, header, r.title);
        }
    }
}

void AdvancedPageContent::resized()
{
    regions.clear();

    const float sy = (float) getHeight() / (float) kDesignHeight;
    const auto rh  = [sy] (int px) { return juce::jmax (1, juce::roundToInt ((float) px * sy)); };

    auto bounds = getLocalBounds();
    const int tabH = rh (28);
    auto tabRow = bounds.removeFromBottom (tabH);
    const int tabW = (tabRow.getWidth() - rh (12)) / 4;
    tabTexture.setBounds (tabRow.removeFromLeft (tabW));
    tabRow.removeFromLeft (rh (4));
    tabMatrix.setBounds (tabRow.removeFromLeft (tabW));
    tabRow.removeFromLeft (rh (4));
    tabFx.setBounds (tabRow.removeFromLeft (tabW));
    tabRow.removeFromLeft (rh (4));
    tabUtility.setBounds (tabRow);

    const int leftW  = juce::roundToInt ((float) bounds.getWidth() * 0.48f);
    const int rightW = bounds.getWidth() - leftW;
    const int rowHpx = rh (24);

    const bool showEngines = activeSection == Section::textureEngine;
    const bool showMatrix  = activeSection == Section::matrix;
    const bool showFx      = activeSection == Section::fxRouting;
    const bool showUtility = activeSection == Section::utility;

    auto hideOsc = [&]
    {
        osc1Wave->setBounds ({}); osc2Wave->setBounds ({}); osc1Types->setBounds ({}); osc2Types->setBounds ({});
        for (const char* id : { P::OSC1_TUNE, P::OSC1_FINE, P::OSC1_SHAPE, P::OSC1_LEVEL, P::OSC1_PAN,
                                P::OSC2_TUNE, P::OSC2_FINE, P::OSC2_SHAPE, P::OSC2_LEVEL, P::OSC2_PAN,
                                P::SOURCE_BLEND, P::INPUT_GAIN })
            placeBox (box (id), {});
    };
    auto hideFilter = [&]
    {
        if (filterCurve != nullptr)
        {
            filterCurve->setBounds ({});
            filterCurve->setVisible (false);
        }
        for (const char* id : { P::FILTER_DRIVE, P::ENV_FLT_AMOUNT,
                                P::ENV_ATTACK, P::ENV_AMP_DECAY, P::ENV_AMP_SUSTAIN, P::ENV_RELEASE, P::GLIDE_TIME,
                                P::VELOCITY_SENSITIVITY })
            placeBox (box (id), {});
        if (filterEnabled != nullptr)
            filterEnabled->setBounds ({});
    };
    auto hideLfoDetail = [&]
    {
        lfo1Wave->setBounds ({}); lfo2Wave->setBounds ({}); lfo3Wave->setBounds ({});
        lfo1Shape.setBounds ({}); lfo2Shape.setBounds ({}); lfo3Shape.setBounds ({});
        lfo1Sync->setBounds ({}); lfo2Sync->setBounds ({}); lfo3Sync->setBounds ({});
        for (const char* id : { P::LFO1_RATE, P::LFO1_DEPTH, P::LFO1_PHASE, P::LFO2_RATE, P::LFO2_DEPTH, P::LFO2_PHASE,
                                P::LFO3_RATE, P::LFO3_DEPTH, P::LFO3_PHASE })
            placeBox (box (id), {});
    };
    auto hideTexture = [&]
    {
        if (textureSection != nullptr)
            textureSection->setBounds ({});
    };
    auto hideMod = [&]
    {
        modHdrOn.setBounds ({}); modHdrRoute.setBounds ({}); modHdrAmt.setBounds ({});
        addModBtn.setBounds ({});
        if (performanceMacros != nullptr)
        {
            performanceMacros->setBounds ({});
            performanceMacros->setVisible (false);
        }
        for (auto& row : modRows)
        {
            row.rowBounds = {};
            row.onBtn.setBounds ({}); row.deleteBtn.setBounds ({});
            row.sourceBox.setBounds ({}); row.destBox.setBounds ({});
            row.sourceLabel.setBounds ({}); row.destLabel.setBounds ({});
            if (row.amountSlider != nullptr) row.amountSlider->setBounds ({});
        }
    };
    auto hideUtility = [&]
    {
        placeBox (box (P::OUTPUT_GAIN), {}); placeBox (box (P::STEREO_WIDTH), {});
        if (outLimiter != nullptr) outLimiter->setBounds ({});
        placeBox (box (P::VOICE_POLYPHONY), {});
        if (voiceGlideBox != nullptr) voiceGlideBox->setBounds ({});
        if (voiceMode != nullptr) voiceMode->setBounds ({});
        if (phraseEnabled != nullptr) phraseEnabled->setBounds ({});
        if (phraseTempoSync != nullptr) phraseTempoSync->setBounds ({});
        if (phraseKeySync != nullptr) phraseKeySync->setBounds ({});
        if (phraseLoop != nullptr) phraseLoop->setBounds ({});
        phraseOneShot.setBounds ({});
        for (const char* id : { P::PHRASE_START, P::PHRASE_LENGTH, P::PHRASE_PITCH })
            placeBox (box (id), {});
    };
    auto hideFx = [&]
    {
        if (fxReverbOn != nullptr) fxReverbOn->setBounds ({});
        if (fxDelayOn != nullptr) fxDelayOn->setBounds ({});
        if (fxChorusOn != nullptr) fxChorusOn->setBounds ({});
        if (fxLofiOn != nullptr) fxLofiOn->setBounds ({});
        if (fxDistOn != nullptr) fxDistOn->setBounds ({});
        if (fxDelaySync != nullptr) fxDelaySync->setBounds ({});
        if (fxEditsToggle != nullptr) fxEditsToggle->setBounds ({});
        for (const char* id : { P::REVERB_AMOUNT, P::REVERB_SIZE, P::FX_REVERB_DAMP, P::FX_DELAY_TIME, P::FX_DELAY_FEEDBACK,
                                P::FX_DELAY_MIX, P::FX_CHORUS_RATE, P::FX_CHORUS_DEPTH, P::FX_CHORUS_MIX,
                                P::FX_LOFI_AMOUNT, P::FX_DIST_DRIVE })
            placeBox (box (id), {});
    };

    hideOsc(); hideFilter(); hideLfoDetail(); hideTexture(); hideMod(); hideUtility(); hideFx();

    if (showEngines)
    {
        const int topH = bounds.getHeight();
        const int oscH = rh (180);
        const int blendH = rh (60);
        const int filterH = rh (150);
        const int lfoH = rh (90);
        int y = bounds.getY();

        auto osc1Area = juce::Rectangle<int> (bounds.getX(), y, leftW, oscH);
        regions.push_back ({ "OSC 1", osc1Area });
        {
            auto b = osc1Area.reduced (8, 0).withTrimmedTop (14);
            osc1Wave->setBounds (b.removeFromTop (rh (80)));
            b.removeFromTop (2);
            osc1Types->setBounds (b.removeFromTop (rh (18)));
            b.removeFromTop (2);
            const int bw = juce::jmin (rh (50), (b.getWidth() - rh (8)) / 5);
            placeRow (b.removeFromTop (rh (36)), { box (P::OSC1_TUNE), box (P::OSC1_FINE), box (P::OSC1_SHAPE),
                           box (P::OSC1_LEVEL), box (P::OSC1_PAN) }, bw, rh (2));
        }
        y += oscH;

        auto osc2Area = juce::Rectangle<int> (bounds.getX(), y, leftW, oscH);
        regions.push_back ({ "OSC 2", osc2Area });
        {
            auto b = osc2Area.reduced (8, 0).withTrimmedTop (14);
            osc2Wave->setBounds (b.removeFromTop (rh (80)));
            b.removeFromTop (2);
            osc2Types->setBounds (b.removeFromTop (rh (18)));
            b.removeFromTop (2);
            const int bw = juce::jmin (rh (50), (b.getWidth() - rh (8)) / 5);
            placeRow (b.removeFromTop (rh (36)), { box (P::OSC2_TUNE), box (P::OSC2_FINE), box (P::OSC2_SHAPE),
                           box (P::OSC2_LEVEL), box (P::OSC2_PAN) }, bw, rh (2));
        }
        y += oscH;

        auto blendArea = juce::Rectangle<int> (bounds.getX(), y, leftW, blendH);
        regions.push_back ({ "SOURCE BLEND", blendArea });
        {
            auto b = blendArea.reduced (8, 0).withTrimmedTop (14);
            const int bw = juce::jmin (rh (48), (b.getWidth() - rh (2)) / 2);
            placeRow (b.removeFromTop (rh (36)), { box (P::SOURCE_BLEND), box (P::INPUT_GAIN), box (P::VELOCITY_SENSITIVITY) }, bw, rh (2));
        }

        int ry = bounds.getY();
        auto filterArea = juce::Rectangle<int> (bounds.getX() + leftW, ry, rightW, filterH);
        regions.push_back ({ "FILTER + AMP", filterArea });
        {
            filterCurve->setVisible (true);
            auto b = filterArea.reduced (8, 0).withTrimmedTop (14);
            if (filterEnabled != nullptr)
                filterEnabled->setBounds (b.removeFromTop (rh (18)));
            b.removeFromTop (rh (2));
            filterCurve->setBounds (b.removeFromTop (rh (72)));
            b.removeFromTop (rh (4));
            const int bw = juce::jmin (rh (48), (b.getWidth() - rh (2)) / 2);
            placeRow (b.removeFromTop (rh (36)), { box (P::FILTER_DRIVE), box (P::ENV_FLT_AMOUNT) }, bw, rh (2));
            b.removeFromTop (rh (4));
            placeRow (b.removeFromTop (rh (36)), { box (P::ENV_ATTACK), box (P::ENV_AMP_DECAY), box (P::ENV_AMP_SUSTAIN),
                          box (P::ENV_RELEASE), box (P::GLIDE_TIME) }, bw, rh (2));
        }
        ry += filterH;

        const auto layoutLfo = [&] (int index, LfoWaveformDisplay* wave, juce::ComboBox& shape,
                                     AdvancedWidgets::FlatToggle* sync,
                                     const char* rateId, const char* depthId, const char* phaseId)
        {
            auto lfoArea = juce::Rectangle<int> (bounds.getX() + leftW, ry, rightW, lfoH);
            regions.push_back ({ "LFO " + juce::String (index), lfoArea });
            auto b = lfoArea.reduced (8, 0).withTrimmedTop (14);
            const int waveW = juce::jmin (rh (200), b.getWidth() / 3);
            wave->setBounds (b.removeFromLeft (waveW));
            b.removeFromLeft (rh (4));
            const int bw = rh (44);
            auto ctrl = b;
            placeBox (box (rateId),  ctrl.removeFromLeft (bw).withHeight (rh (36)));
            ctrl.removeFromLeft (rh (2));
            placeBox (box (depthId), ctrl.removeFromLeft (bw).withHeight (rh (36)));
            ctrl.removeFromLeft (rh (2));
            placeBox (box (phaseId), ctrl.removeFromLeft (bw).withHeight (rh (36)));
            ctrl.removeFromLeft (rh (4));
            shape.setBounds (ctrl.removeFromLeft (rh (72)).withHeight (rh (20)));
            ctrl.removeFromLeft (rh (2));
            sync->setBounds (ctrl.removeFromLeft (rh (62)).withHeight (rh (18)));
            ry += lfoH;
        };

        layoutLfo (1, lfo1Wave.get(), lfo1Shape, lfo1Sync.get(), P::LFO1_RATE, P::LFO1_DEPTH, P::LFO1_PHASE);
        layoutLfo (2, lfo2Wave.get(), lfo2Shape, lfo2Sync.get(), P::LFO2_RATE, P::LFO2_DEPTH, P::LFO2_PHASE);
        layoutLfo (3, lfo3Wave.get(), lfo3Shape, lfo3Sync.get(), P::LFO3_RATE, P::LFO3_DEPTH, P::LFO3_PHASE);

        const int texTop = bounds.getY() + rh (kTopRowH) + rh (kDividerH);
        auto texArea = juce::Rectangle<int> (bounds.getX(), texTop, bounds.getWidth(), bounds.getBottom() - texTop);
        regions.push_back ({ "TEXTURE", texArea });
        if (textureSection != nullptr)
            textureSection->setBounds (texArea.reduced (4, 2));

        juce::ignoreUnused (topH);
    }
    else if (showMatrix)
    {
        if (performanceMacros != nullptr)
            performanceMacros->setVisible (true);

        const int macroH = rh (168);
        auto macroArea = bounds.removeFromBottom (macroH);
        performanceMacros->setBounds (macroArea);
        bounds.removeFromBottom (rh (6));

        const int focusW = juce::roundToInt ((float) bounds.getWidth() * 0.42f);
        auto focusCol = bounds.removeFromLeft (focusW);
        layoutModulatorFocus (focusCol, rh);

        auto modArea = bounds.reduced (8, 4);
        regions.push_back ({ "MOD MATRIX", modArea });
        layoutModMatrixZone (modArea, rowHpx);
    }
    else if (showFx)
    {
        auto fxArea = bounds.reduced (12);
        regions.push_back ({ "FX", fxArea });
        const int editsToggleW = rh (56);
        auto fxRow = fxArea.reduced (8, 8).withTrimmedTop (14);
        fxEditsToggle->setBounds (fxRow.removeFromRight (editsToggleW).withHeight (rh (18)));

        const int blockW = fxRow.getWidth() / 5;
        const int bw = juce::jmin (rh (52), blockW / 4);
        const int boxH = rh (36);

        const auto layoutFxBlock = [&] (juce::Rectangle<int> block,
                                         AdvancedWidgets::FxEnableButton* onBtn,
                                         const std::vector<BV*>& boxes,
                                         AdvancedWidgets::FlatToggle* syncBtn = nullptr)
        {
            onBtn->setBounds (block.removeFromTop (rh (22)));
            block.removeFromTop (rh (6));
            auto params = block;
            for (auto* bx : boxes)
            {
                placeBox (bx, params.removeFromLeft (bw).withHeight (boxH));
                params.removeFromLeft (rh (2));
            }
            if (syncBtn != nullptr)
                syncBtn->setBounds (params.removeFromLeft (rh (52)).withHeight (rh (16)));
        };

        layoutFxBlock (fxRow.removeFromLeft (blockW), fxReverbOn.get(),
                       { box (P::REVERB_AMOUNT), box (P::REVERB_SIZE), box (P::FX_REVERB_DAMP) });
        layoutFxBlock (fxRow.removeFromLeft (blockW), fxDelayOn.get(),
                       { box (P::FX_DELAY_TIME), box (P::FX_DELAY_FEEDBACK), box (P::FX_DELAY_MIX) },
                       fxDelaySync.get());
        layoutFxBlock (fxRow.removeFromLeft (blockW), fxChorusOn.get(),
                       { box (P::FX_CHORUS_RATE), box (P::FX_CHORUS_DEPTH), box (P::FX_CHORUS_MIX) });
        layoutFxBlock (fxRow.removeFromLeft (blockW), fxLofiOn.get(), { box (P::FX_LOFI_AMOUNT) });
        layoutFxBlock (fxRow, fxDistOn.get(), { box (P::FX_DIST_DRIVE) });
    }
    else if (showUtility)
    {
        auto panel = bounds.reduced (24);
        const int bw = rh (52);
        const int blockH = rh (90);

        auto outArea = panel.removeFromTop (blockH);
        regions.push_back ({ "OUTPUT", outArea });
        {
            auto b = outArea.reduced (8, 0).withTrimmedTop (14);
            auto row = b.removeFromTop (rh (36));
            placeBox (box (P::OUTPUT_GAIN), row.removeFromLeft (bw));
            row.removeFromLeft (rh (4));
            placeBox (box (P::STEREO_WIDTH), row.removeFromLeft (bw));
            row.removeFromLeft (rh (8));
            outLimiter->setBounds (row.removeFromLeft (rh (52)).withHeight (rh (18)));
        }

        auto voiceArea = panel.removeFromTop (blockH);
        regions.push_back ({ "VOICE", voiceArea });
        {
            auto b = voiceArea.reduced (8, 0).withTrimmedTop (14);
            auto row = b.removeFromTop (rh (36));
            placeBox (box (P::VOICE_POLYPHONY), row.removeFromLeft (bw));
            row.removeFromLeft (rh (4));
            placeBox (voiceGlideBox.get(), row.removeFromLeft (bw));
            row.removeFromLeft (rh (8));
            voiceMode->setBounds (row.withHeight (rh (18)));
        }

        auto phraseArea = panel;
        regions.push_back ({ "PHRASES", phraseArea });
        {
            auto b = phraseArea.reduced (8, 0).withTrimmedTop (14);
            auto row = b.removeFromTop (rh (36));
            placeBox (box (P::PHRASE_START), row.removeFromLeft (bw));
            row.removeFromLeft (rh (4));
            placeBox (box (P::PHRASE_LENGTH), row.removeFromLeft (bw));
            row.removeFromLeft (rh (4));
            placeBox (box (P::PHRASE_PITCH), row.removeFromLeft (bw));
            row.removeFromLeft (rh (8));
            phraseEnabled->setBounds (row.removeFromLeft (rh (44)).withHeight (rh (18)));
            row.removeFromLeft (rh (4));
            phraseTempoSync->setBounds (row.removeFromLeft (rh (56)).withHeight (rh (18)));
            row.removeFromLeft (rh (4));
            phraseKeySync->setBounds (row.removeFromLeft (rh (44)).withHeight (rh (18)));
            row.removeFromLeft (rh (4));
            phraseLoop->setBounds (row.removeFromLeft (rh (44)).withHeight (rh (18)));
            row.removeFromLeft (rh (4));
            phraseOneShot.setBounds (row.removeFromLeft (rh (64)).withHeight (rh (18)));
        }
    }
}

void AdvancedPageContent::mouseMove (const juce::MouseEvent& e)
{
    if (activeSection != Section::matrix)
        return;

    int nextHover = -1;
    for (int i = 0; i < (int) modRows.size(); ++i)
    {
        if (isModRowActive (i) && modRows[(size_t) i].rowBounds.contains (e.getPosition()))
        {
            nextHover = i;
            break;
        }
    }

    if (nextHover != hoveredModRowIndex)
    {
        hoveredModRowIndex = nextHover;

        for (int i = 0; i < (int) modRows.size(); ++i)
        {
            if (! isModRowActive (i))
                continue;

            const float alpha = hoveredModRowIndex == i ? 1.f : 0.28f;
            modRows[(size_t) i].onBtn.setAlpha (alpha);
            modRows[(size_t) i].deleteBtn.setAlpha (alpha);
        }

        repaint();
    }
}

void AdvancedPageContent::mouseDown (const juce::MouseEvent& e)
{
    if (activeSection != Section::matrix)
        return;

    for (int i = 0; i < (int) modRows.size(); ++i)
    {
        if (! isModRowActive (i))
            continue;

        auto& row = modRows[(size_t) i];
        if (! row.rowBounds.contains (e.getPosition()))
            continue;

        if (row.onBtn.getBounds().contains (e.getPosition())
            || row.deleteBtn.getBounds().contains (e.getPosition())
            || (row.amountSlider != nullptr && row.amountSlider->getBounds().contains (e.getPosition())))
            return;

        const int srcIdx = modChoiceIndex (ModRoutingHub::kRows[i].source);
        if (srcIdx >= 1 && srcIdx <= 3)
            selectModSource (srcIdx);
        return;
    }
}

void AdvancedPageContent::timerCallback()
{
    if (textureSection != nullptr)
        textureSection->syncVisualizerFromParams();

    for (int i = 0; i < (int) modRows.size(); ++i)
    {
        const bool on = apvtsRef.getRawParameterValue (ModRoutingHub::kRows[i].on)->load() > 0.5f;
        styleModOnBtn (modRows[(size_t) i].onBtn, on);
    }

    if (phraseOneShot.isVisible())
    {
        const bool loopOn = apvtsRef.getRawParameterValue (P::PHRASE_LOOP)->load() > 0.5f;
        phraseOneShot.setToggleState (! loopOn, juce::dontSendNotification);
        AdvancedWidgets::stylePillButton (phraseOneShot, ! loopOn);
    }
}
