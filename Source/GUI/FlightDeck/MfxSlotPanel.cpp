#include "MfxSlotPanel.h"
#include "../../PluginProcessor.h"
#include "../../DSP/Mfx/MfxDescriptors.h"

namespace
{
constexpr int kHeaderH = 50;
constexpr int kGenH = 30;
constexpr int kAssignH = 58;
constexpr int kPad = 12;
} // namespace

// =============================================================================
//  HeaderButton — flat deck button (text, optional accent) with click handler.
// =============================================================================
class MfxSlotPanel::HeaderButton : public juce::Component
{
public:
    enum class Style { primary, ghost, selector };

    HeaderButton (const juce::String& text, Style st, std::function<void()> onClick)
        : label (text), style (st), handler (std::move (onClick))
    {
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
    }

    void setText (const juce::String& t, const juce::String& sub = {})
    {
        if (t != label || sub != subLabel) { label = t; subLabel = sub; repaint(); }
    }
    void setOn (bool o) { if (o != on) { on = o; repaint(); } }
    void setEnabledLook (bool e) { if (e != enabledLook) { enabledLook = e; repaint(); } }

    void mouseUp (const juce::MouseEvent& e) override
    {
        if (handler && getLocalBounds().contains (e.getPosition()))
            handler();
    }
    void mouseEnter (const juce::MouseEvent&) override { hover = true; repaint(); }
    void mouseExit (const juce::MouseEvent&) override { hover = false; repaint(); }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        if (style == Style::selector)
        {
            juce::ColourGradient grad (juce::Colour (0xff0b1a27), r.getX(), r.getY(), juce::Colour (0xff071320), r.getX(), r.getBottom(), false);
            g.setGradientFill (grad);
            g.fillRoundedRectangle (r, 3.0f);
            g.setColour (hover ? Aviation::goldDeep() : juce::Colour (0x38c9a35b));
            g.drawRoundedRectangle (r, 3.0f, 1.0f);
            auto inner = getLocalBounds().reduced (12, 0);
            g.setFont (Aviation::sans (17.0f, true));
            g.setColour (Aviation::cyanBright());
            g.drawText (label, inner.withTrimmedBottom (14), juce::Justification::centredLeft);
            g.setFont (Aviation::label (8.5f, 0.16f));
            g.setColour (Aviation::gold());
            g.drawText (subLabel, inner.withTrimmedTop (26), juce::Justification::centredLeft);
            g.setFont (Deck::mono (10.0f));
            g.setColour (Aviation::textSecondary());
            g.drawText (juce::String::fromUTF8 ("\xe2\x96\xbc"), inner, juce::Justification::centredRight);
            return;
        }

        const bool primary = style == Style::primary;
        if (primary || on)
        {
            juce::ColourGradient grad (on ? juce::Colour (0xff14293b) : juce::Colour (0xff0d1f2e), r.getX(), r.getY(),
                                       on ? juce::Colour (0xff0a1c2c) : juce::Colour (0xff071522), r.getX(), r.getBottom(), false);
            g.setGradientFill (grad);
            g.fillRoundedRectangle (r, 3.0f);
        }
        const auto border = on ? Aviation::cyan() : (primary ? (hover ? Aviation::cyan() : Aviation::goldDeep())
                                                            : (hover ? Aviation::cyanDim() : juce::Colour (0x29708596)));
        g.setColour (border.withAlpha (enabledLook ? 1.0f : 0.35f));
        g.drawRoundedRectangle (r, 3.0f, 1.0f);
        g.setFont (Aviation::label (9.0f, 0.14f));
        const auto text = on ? Aviation::cyanBright() : (primary ? (hover ? Aviation::cyanBright() : Aviation::goldBright())
                                                                 : (hover ? Aviation::cyan() : Aviation::textSecondary()));
        g.setColour (text.withAlpha (enabledLook ? 1.0f : 0.35f));
        g.drawText (label, getLocalBounds(), juce::Justification::centred);
    }

private:
    juce::String label, subLabel;
    Style style;
    std::function<void()> handler;
    bool hover { false }, on { false }, enabledLook { true };
};

// =============================================================================
//  ParamColumn — one generic slot: id, name, vertical track, value, lock.
// =============================================================================
class MfxSlotPanel::ParamColumn : public juce::Component
{
public:
    ParamColumn (AviatorKeyzProcessor& p, int slotIndex, int paramIndex)
        : processor (p), slot (slotIndex), index (paramIndex)
    {
        param = p.getAPVTS().getParameter (Mfx::paramId (slot, index));
        if (param != nullptr)
        {
            attachment = std::make_unique<juce::ParameterAttachment> (*param, [this] (float v) { norm = v; repaint(); });
            attachment->sendInitialUpdate();
        }
    }

    void setSpec (const Mfx::ParamSpec* s, bool ids) { spec = s; showId = ids; repaint(); }
    void setLive (float realValue, bool useLive) { live = realValue; liveActive = useLive; }

    bool locked() const { return (processor.getMfxLocks (slot) >> index) & 1u; }

    void mouseDown (const juce::MouseEvent& e) override
    {
        if (spec == nullptr || ! spec->used() || attachment == nullptr)
            return;
        if (lockArea().contains (e.getPosition()))
        {
            processor.setMfxLocks (slot, processor.getMfxLocks (slot) ^ (1u << index));
            repaint();
            return;
        }
        if (e.getNumberOfClicks() >= 2)
        {
            attachment->setValueAsCompleteGesture (spec->normalise (spec->def));
            return;
        }
        dragStartNorm = norm;
        attachment->beginGesture();
        dragging = true;
        if (trackArea().contains (e.getPosition()))
        {
            const float n = 1.0f - (float) (e.y - trackArea().getY()) / (float) trackArea().getHeight();
            attachment->setValueAsPartOfGesture (juce::jlimit (0.0f, 1.0f, n));
            dragStartNorm = juce::jlimit (0.0f, 1.0f, n);
        }
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (! dragging || attachment == nullptr)
            return;
        const float fine = (e.mods.isShiftDown() || e.mods.isCommandDown()) ? 0.25f : 1.0f;
        const float delta = -(float) e.getDistanceFromDragStartY() / (float) juce::jmax (40, trackArea().getHeight()) * fine;
        attachment->setValueAsPartOfGesture (juce::jlimit (0.0f, 1.0f, dragStartNorm + delta));
    }

    void mouseUp (const juce::MouseEvent&) override
    {
        if (dragging && attachment != nullptr)
            attachment->endGesture();
        dragging = false;
    }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        const bool used = spec != nullptr && spec->used();
        const bool isLocked = used && locked();
        const float alpha = used ? 1.0f : 0.3f;

        juce::ColourGradient grad (juce::Colour (0x9e0a1823), r.getX(), r.getY(), juce::Colour (0x4c050d16), r.getX(), r.getBottom(), false);
        g.setGradientFill (grad);
        g.fillRoundedRectangle (r, 3.0f);
        g.setColour (isLocked ? Aviation::gold() : juce::Colour (0x29708596));
        g.drawRoundedRectangle (r, 3.0f, 1.0f);
        if (isLocked)
        {
            g.setColour (Aviation::gold().withAlpha (0.10f));
            g.fillRoundedRectangle (r, 3.0f);
        }

        // id + name
        g.setFont (Deck::mono (7.5f));
        g.setColour (Aviation::textDim().withAlpha (alpha));
        if (showId)
            g.drawText (juce::String::formatted ("p%02d", index + 1), 0, 4, getWidth(), 10, juce::Justification::centred);
        g.setFont (Aviation::label (8.5f, 0.06f));
        g.setColour (Aviation::textPrimary().withAlpha (alpha));
        g.drawFittedText (used ? juce::String (spec->label).toUpperCase() : juce::String::fromUTF8 ("\xe2\x80\x94"),
                          2, 14, getWidth() - 4, 20, juce::Justification::centred, 2, 0.8f);

        // track
        auto track = trackArea().toFloat();
        g.setColour (juce::Colour (0xff01050a));
        g.fillRoundedRectangle (track, 2.0f);
        g.setColour (juce::Colour (0x33708596));
        g.drawRoundedRectangle (track, 2.0f, 1.0f);
        if (used)
        {
            const float fillH = track.getHeight() * juce::jlimit (0.0f, 1.0f, norm);
            juce::Rectangle<float> fill (track.getX() + 1.0f, track.getBottom() - fillH, track.getWidth() - 2.0f, fillH);
            const bool cautious = spec->cautious;
            juce::ColourGradient fg (cautious ? juce::Colour (0xfff5a623) : Aviation::cyan(), fill.getX(), fill.getY(),
                                     cautious ? juce::Colour (0xffc47f12) : juce::Colour (0xff1c7fae), fill.getX(), fill.getBottom(), false);
            g.setGradientFill (fg);
            g.fillRect (fill);

            if (liveActive)
            {
                const float ln = spec->normalise (live);
                const float y = track.getBottom() - track.getHeight() * ln;
                g.setColour (Aviation::goldBright().withAlpha (0.9f));
                g.fillRect (juce::Rectangle<float> (track.getX() - 2.0f, y - 1.0f, track.getWidth() + 4.0f, 2.0f));
            }
        }

        // value
        g.setFont (Deck::mono (8.5f));
        g.setColour (Aviation::cyan().withAlpha (alpha));
        const float shown = used ? (liveActive ? live : spec->denormalise (norm)) : 0.0f;
        g.drawText (used ? Mfx::valueText (*spec, shown) : juce::String(), 0, trackArea().getBottom() + 4, getWidth(), 12,
                    juce::Justification::centred);

        // lock
        if (used)
        {
            auto la = lockArea().toFloat();
            g.setColour (isLocked ? Aviation::gold() : Aviation::textDim());
            g.drawRoundedRectangle (la.reduced (2.0f).withTrimmedTop (4.0f), 1.5f, 1.0f);
            juce::Path shackle;
            shackle.addCentredArc (la.getCentreX(), la.getY() + 5.0f, 2.5f, 3.5f, 0.0f,
                                   -juce::MathConstants<float>::halfPi, juce::MathConstants<float>::halfPi, true);
            if (! isLocked)
                shackle.applyTransform (juce::AffineTransform::translation (2.0f, -1.0f));
            g.strokePath (shackle, juce::PathStrokeType (1.0f));
        }
    }

private:
    juce::Rectangle<int> trackArea() const { return { getWidth() / 2 - 7, 36, 14, getHeight() - 36 - 34 }; }
    juce::Rectangle<int> lockArea() const { return { getWidth() / 2 - 8, getHeight() - 16, 16, 14 }; }

    AviatorKeyzProcessor& processor;
    int slot, index;
    juce::RangedAudioParameter* param { nullptr };
    std::unique_ptr<juce::ParameterAttachment> attachment;
    const Mfx::ParamSpec* spec { nullptr };
    float norm { 0.f };
    float live { 0.f };
    bool liveActive { false };
    bool showId { false };
    bool dragging { false };
    float dragStartNorm { 0.f };
};

// =============================================================================
//  AssignCell — source selector + sens knob for one ASSIGN row.
// =============================================================================
class MfxSlotPanel::AssignCell : public juce::Component
{
public:
    AssignCell (AviatorKeyzProcessor& p, int slotIndex, int assignIndex, MenuLookAndFeel& lnf)
        : processor (p), slot (slotIndex), index (assignIndex), menuLnf (lnf)
    {
        auto& apvts = p.getAPVTS();
        srcParam = apvts.getParameter (Mfx::assignSourceId (slot, index));
        if (srcParam != nullptr)
        {
            srcAttachment = std::make_unique<juce::ParameterAttachment> (*srcParam, [this] (float v) { source = juce::roundToInt (v); repaint(); });
            srcAttachment->sendInitialUpdate();
        }
        sens = std::make_unique<DeckKnob> (apvts, Mfx::assignAmountId (slot, index), "SENS", 30, DeckKnob::Format::percent);
        addAndMakeVisible (*sens);
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
    }

    void setTargetName (const juce::String& t) { if (t != target) { target = t; repaint(); } }

    void resized() override
    {
        sens->setBounds (getWidth() - 46, 2, 44, getHeight() - 2);
    }

    void mouseUp (const juce::MouseEvent& e) override
    {
        if (e.x > getWidth() - 48 || srcAttachment == nullptr)
            return;
        juce::PopupMenu m;
        m.setLookAndFeel (&menuLnf);
        const auto names = Mfx::modSourceNames();
        for (int i = 0; i < names.size(); ++i)
            m.addItem (i + 1, names[i], true, i == source);
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this),
                         [safe = juce::Component::SafePointer<AssignCell> (this)] (int result)
                         {
                             if (safe != nullptr && result > 0 && safe->srcAttachment != nullptr)
                                 safe->srcAttachment->setValueAsCompleteGesture ((float) (result - 1));
                         });
    }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        g.setColour (juce::Colour (0x800a1823));
        g.fillRoundedRectangle (r, 3.0f);
        g.setColour (source > 0 ? Aviation::cyanDim() : juce::Colour (0x29708596));
        g.drawRoundedRectangle (r, 3.0f, 1.0f);

        g.setFont (Aviation::label (8.0f, 0.16f));
        g.setColour (Aviation::gold());
        const bool narrow = getWidth() < 150;
        const juce::String cap = (narrow ? "A" : "ASSIGN ") + juce::String (index + 1)
                                 + (target.isNotEmpty() ? " " + juce::String::fromUTF8 ("\xe2\x86\x92") + " " + target.toUpperCase() : juce::String());
        g.drawFittedText (cap, 10, 5, getWidth() - 58, 12, juce::Justification::centredLeft, 1, 0.7f);

        juce::Rectangle<int> sel (10, 22, getWidth() - 66, 20);
        g.setColour (juce::Colour (0xff050d16));
        g.fillRoundedRectangle (sel.toFloat(), 2.0f);
        g.setColour (juce::Colour (0x29708596));
        g.drawRoundedRectangle (sel.toFloat(), 2.0f, 1.0f);
        g.setFont (Aviation::body (11.0f));
        g.setColour (source > 0 ? Aviation::textPrimary() : Aviation::textSecondary());
        g.drawText (Mfx::modSourceName (static_cast<Mfx::ModSource> (juce::jlimit (0, (int) Mfx::ModSource::count - 1, source))),
                    sel.reduced (6, 0), juce::Justification::centredLeft);
        g.setFont (Deck::mono (8.0f));
        g.setColour (Aviation::textDim());
        g.drawText (juce::String::fromUTF8 ("\xe2\x96\xbc"), sel.reduced (6, 0), juce::Justification::centredRight);
    }

private:
    AviatorKeyzProcessor& processor;
    int slot, index;
    MenuLookAndFeel& menuLnf;
    juce::RangedAudioParameter* srcParam { nullptr };
    std::unique_ptr<juce::ParameterAttachment> srcAttachment;
    std::unique_ptr<DeckKnob> sens;
    int source { 0 };
    juce::String target;
};

// =============================================================================
MfxSlotPanel::MfxSlotPanel (AviatorKeyzProcessor& p, int slotIndex)
    : processorRef (p), apvts (p.getAPVTS()), slot (slotIndex)
{
    rng.setSeedRandomly();

    powerPad = std::make_unique<DeckPad> (apvts, Mfx::onId (slot), slot == 0 ? "MFX A" : "MFX B", "POWER", Aviation::cyan());
    addAndMakeVisible (*powerPad);

    prevBtn = std::make_unique<HeaderButton> (juce::String::fromUTF8 ("\xe2\x96\xb2"), HeaderButton::Style::ghost, [this] { stepEffect (-1); });
    nextBtn = std::make_unique<HeaderButton> (juce::String::fromUTF8 ("\xe2\x96\xbc"), HeaderButton::Style::ghost, [this] { stepEffect (+1); });
    selectorBtn = std::make_unique<HeaderButton> ("", HeaderButton::Style::selector, [this] { openEffectMenu(); });
    presetBtn = std::make_unique<HeaderButton> ("PRESET", HeaderButton::Style::ghost, [this] { openPresetMenu(); });
    rerollBtn = std::make_unique<HeaderButton> (juce::String::fromUTF8 ("\xe2\x9a\x84 REROLL"), HeaderButton::Style::primary, [this] { reroll(); });
    surpriseBtn = std::make_unique<HeaderButton> (juce::String::fromUTF8 ("\xe2\x9c\xa6 SURPRISE ME"), HeaderButton::Style::primary, [this] { surprise(); });
    undoBtn = std::make_unique<HeaderButton> ("UNDO", HeaderButton::Style::ghost, [this] { undo(); });
    idsBtn = std::make_unique<HeaderButton> ("IDs", HeaderButton::Style::ghost, [this] { showIds = ! showIds; idsBtn->setOn (showIds); rebuildColumns(); });
    for (auto* b : { prevBtn.get(), nextBtn.get(), selectorBtn.get(), presetBtn.get(), rerollBtn.get(), surpriseBtn.get(), undoBtn.get(), idsBtn.get() })
        addAndMakeVisible (*b);

    sendKnob = std::make_unique<DeckKnob> (apvts, Mfx::sendId (slot), "REV SEND", 36, DeckKnob::Format::percent);
    levelKnob = std::make_unique<DeckKnob> (apvts, Mfx::levelId (slot), "LEVEL", 36, DeckKnob::Format::decibels, true);
    addAndMakeVisible (*sendKnob);
    addAndMakeVisible (*levelKnob);

    amountSlider.setRange (0.05, 1.0, 0.01);
    amountSlider.setValue (0.55, juce::dontSendNotification);
    amountSlider.setColour (juce::Slider::trackColourId, Aviation::cyan().withAlpha (0.7f));
    amountSlider.setColour (juce::Slider::backgroundColourId, juce::Colour (0xff0a1823));
    amountSlider.setColour (juce::Slider::thumbColourId, Aviation::cyanBright());
    amountSlider.onValueChange = [this] { repaint (generatorArea()); };
    addAndMakeVisible (amountSlider);

    for (int i = 0; i < Mfx::kParamsPerSlot; ++i)
    {
        auto col = std::make_unique<ParamColumn> (processorRef, slot, i);
        addAndMakeVisible (*col);
        columns.push_back (std::move (col));
    }
    for (int a = 0; a < Mfx::kNumAssigns; ++a)
    {
        auto cell = std::make_unique<AssignCell> (processorRef, slot, a, menuLookAndFeel);
        addAndMakeVisible (*cell);
        assigns.push_back (std::move (cell));
    }

    if (auto* ep = apvts.getParameter (Mfx::effectId (slot)))
    {
        effectAttachment = std::make_unique<juce::ParameterAttachment> (*ep, [this] (float v)
        {
            currentEffect = juce::jlimit (0, (int) Mfx::Effect::count - 1, juce::roundToInt (v));
            effectChanged();
        });
        effectAttachment->sendInitialUpdate();
    }

    startTimerHz (20);
}

MfxSlotPanel::~MfxSlotPanel()
{
    stopTimer();
}

void MfxSlotPanel::effectChanged()
{
    const auto& d = Mfx::descriptor (currentEffect);
    selectorBtn->setText (d.name, Mfx::categoryName (d.category));
    for (int a = 0; a < Mfx::kNumAssigns; ++a)
    {
        const int t = d.assignTargets[(size_t) a];
        assigns[(size_t) a]->setTargetName (t >= 0 && d.params[(size_t) t].used() ? d.params[(size_t) t].label : juce::String());
    }
    rebuildColumns();
    repaint();
}

void MfxSlotPanel::rebuildColumns()
{
    const auto& d = Mfx::descriptor (currentEffect);
    for (int i = 0; i < Mfx::kParamsPerSlot; ++i)
        columns[(size_t) i]->setSpec (&d.params[(size_t) i], showIds);
}

void MfxSlotPanel::timerCallback()
{
    const bool on = apvts.getRawParameterValue (Mfx::onId (slot))->load() > 0.5f;
    bool anyAssign = false;
    for (int a = 0; a < Mfx::kNumAssigns; ++a)
        if (apvts.getRawParameterValue (Mfx::assignSourceId (slot, a))->load() > 0.5f)
            anyAssign = true;
    const auto& d = Mfx::descriptor (currentEffect);
    for (int i = 0; i < Mfx::kParamsPerSlot; ++i)
    {
        columns[(size_t) i]->setLive (processorRef.getMfxRack().getLiveValue (slot, i), on && anyAssign);
        if (on && anyAssign && d.params[(size_t) i].used())
            columns[(size_t) i]->repaint();
    }
    undoBtn->setEnabledLook (processorRef.canUndoMfx (slot));
    repaint (headerArea().removeFromLeft (0)); // no-op keeps compiler quiet about unused
    repaint (juce::Rectangle<int> (getWidth() - 120, 0, 120, Deck::kZoneHeaderH)); // slot meter in the zone tag
}

void MfxSlotPanel::stepEffect (int delta)
{
    const int count = (int) Mfx::Effect::count;
    const int next = ((currentEffect + delta) % count + count) % count;
    processorRef.setMfxEffect (slot, static_cast<Mfx::Effect> (next));
}

void MfxSlotPanel::openEffectMenu()
{
    juce::PopupMenu menu;
    menu.setLookAndFeel (&menuLookAndFeel);
    // categorised: one submenu per category, in registry order
    std::vector<int> cats;
    for (int e = 0; e < (int) Mfx::Effect::count; ++e)
    {
        const int c = (int) Mfx::descriptor (e).category;
        if (std::find (cats.begin(), cats.end(), c) == cats.end())
            cats.push_back (c);
    }
    for (int c : cats)
    {
        juce::PopupMenu sub;
        for (int e = 0; e < (int) Mfx::Effect::count; ++e)
        {
            const auto& d = Mfx::descriptor (e);
            if ((int) d.category != c) continue;
            sub.addItem (e + 1, d.name, true, e == currentEffect);
        }
        menu.addSubMenu (Mfx::categoryName (static_cast<Mfx::Category> (c)), sub, true, nullptr,
                         (int) Mfx::descriptor (currentEffect).category == c);
    }
    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (selectorBtn.get()),
                        [safe = juce::Component::SafePointer<MfxSlotPanel> (this)] (int result)
                        {
                            if (safe != nullptr && result > 0)
                                safe->processorRef.setMfxEffect (safe->slot, static_cast<Mfx::Effect> (result - 1));
                        });
}

void MfxSlotPanel::openPresetMenu()
{
    juce::PopupMenu menu;
    menu.setLookAndFeel (&menuLookAndFeel);
    const auto& d = Mfx::descriptor (currentEffect);
    menu.addSectionHeader (juce::String (d.name).toUpperCase() + " PRESETS");
    for (int i = 0; i < Mfx::kMaxPresets; ++i)
        if (d.presets[(size_t) i].name != nullptr)
            menu.addItem (i + 1, d.presets[(size_t) i].name);
    menu.addSeparator();
    menu.addItem (100, "Reset to defaults");
    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (presetBtn.get()),
                        [safe = juce::Component::SafePointer<MfxSlotPanel> (this)] (int result)
                        {
                            if (safe == nullptr || result <= 0) return;
                            const auto effect = static_cast<Mfx::Effect> (safe->currentEffect);
                            safe->processorRef.setMfxEffect (safe->slot, effect, result == 100 ? -1 : result - 1);
                        });
}

void MfxSlotPanel::reroll()
{
    processorRef.pushMfxHistory (slot);
    auto snap = processorRef.captureMfx (slot);
    snap.values = Mfx::reroll (static_cast<Mfx::Effect> (currentEffect), snap.values,
                               processorRef.getMfxLocks (slot), (float) amountSlider.getValue(), rng);
    processorRef.applyMfx (slot, snap);
}

void MfxSlotPanel::surprise()
{
    processorRef.pushMfxHistory (slot);
    int next = currentEffect;
    if ((int) Mfx::Effect::count > 1)
        while (next == currentEffect)
            next = rng.nextInt ((int) Mfx::Effect::count);
    processorRef.setMfxLocks (slot, 0);
    auto snap = processorRef.captureMfx (slot);
    snap.effect = next;
    snap.values = Mfx::reroll (static_cast<Mfx::Effect> (next), Mfx::defaultsNormalised (static_cast<Mfx::Effect> (next)), 0, 1.0f, rng);
    processorRef.applyMfx (slot, snap);
    if (auto* on = apvts.getParameter (Mfx::onId (slot)))
        on->setValueNotifyingHost (1.0f);
}

void MfxSlotPanel::undo()
{
    processorRef.undoMfx (slot);
}

// -----------------------------------------------------------------------------
juce::Rectangle<int> MfxSlotPanel::headerArea() const
{
    return { kPad, Deck::kZoneHeaderH + 8, getWidth() - kPad * 2, kHeaderH };
}

juce::Rectangle<int> MfxSlotPanel::generatorArea() const
{
    return { kPad, headerArea().getBottom() + 8, getWidth() - kPad * 2, kGenH };
}

juce::Rectangle<int> MfxSlotPanel::assignsArea() const
{
    return { kPad, getHeight() - kPad - kAssignH, getWidth() - kPad * 2, kAssignH };
}

juce::Rectangle<int> MfxSlotPanel::bankArea() const
{
    return { kPad, generatorArea().getBottom() + 8, getWidth() - kPad * 2,
             assignsArea().getY() - 8 - (generatorArea().getBottom() + 8) };
}

void MfxSlotPanel::resized()
{
    auto h = headerArea();
    powerPad->setBounds (h.removeFromLeft (72).withHeight (kHeaderH));
    h.removeFromLeft (8);
    auto steps = h.removeFromLeft (26);
    prevBtn->setBounds (steps.removeFromTop (kHeaderH / 2 - 2));
    nextBtn->setBounds (steps.removeFromBottom (kHeaderH / 2 - 2));
    h.removeFromLeft (8);
    levelKnob->setBounds (h.removeFromRight (DeckKnob::preferredWidth (36)).withHeight (DeckKnob::preferredHeight (36)).withY (h.getY() - 4));
    h.removeFromRight (2);
    sendKnob->setBounds (h.removeFromRight (DeckKnob::preferredWidth (36)).withHeight (DeckKnob::preferredHeight (36)).withY (h.getY() - 4));
    h.removeFromRight (10);
    presetBtn->setBounds (h.removeFromRight (64).withSizeKeepingCentre (64, 26));
    h.removeFromRight (8);
    selectorBtn->setBounds (h.withSizeKeepingCentre (h.getWidth(), 44));

    auto gen = generatorArea();
    rerollBtn->setBounds (gen.removeFromLeft (92).withHeight (26).withY (gen.getY() + 2));
    gen.removeFromLeft (8);
    surpriseBtn->setBounds (gen.removeFromLeft (wide() ? 118 : 108).withHeight (26).withY (gen.getY() + 2));
    gen.removeFromLeft (14);
    // AMOUNT label painted; slider + readout
    gen.removeFromLeft (52);
    amountSlider.setBounds (gen.removeFromLeft (wide() ? 104 : 80).withHeight (22).withY (gen.getY() + 4));
    gen.removeFromLeft (44);
    idsBtn->setBounds (gen.removeFromRight (44).withHeight (26).withY (gen.getY() + 2));
    gen.removeFromRight (8);
    undoBtn->setBounds (gen.removeFromRight (56).withHeight (26).withY (gen.getY() + 2));

    auto bank = bankArea();
    const int perRow = wide() ? 16 : 8;
    const int rows = Mfx::kParamsPerSlot / perRow;
    const int gap = 6;
    const int colW = (bank.getWidth() - gap * (perRow - 1)) / perRow;
    const int rowH = (bank.getHeight() - gap * (rows - 1)) / rows;
    for (int i = 0; i < Mfx::kParamsPerSlot; ++i)
    {
        const int r = i / perRow, c = i % perRow;
        columns[(size_t) i]->setBounds (bank.getX() + c * (colW + gap), bank.getY() + r * (rowH + gap), colW, rowH);
    }

    auto asg = assignsArea();
    const int cellW = (asg.getWidth() - gap * 3) / 4;
    for (int a = 0; a < Mfx::kNumAssigns; ++a)
        assigns[(size_t) a]->setBounds (asg.getX() + a * (cellW + gap), asg.getY(), cellW, asg.getHeight());
}

void MfxSlotPanel::paint (juce::Graphics& g)
{
    const juce::String title = slot == 0 ? juce::String::fromUTF8 ("MFX \xc2\xb7 SLOT A") : juce::String::fromUTF8 ("MFX \xc2\xb7 SLOT B");
    const juce::String tag = slot == 0 ? juce::String::fromUTF8 ("SIXTEEN GENERIC SLOTS \xc2\xb7 RELABEL PER EFFECT \xc2\xb7 A \xe2\x86\x92 B")
                                       : juce::String::fromUTF8 ("SERIES AFTER SLOT A \xc2\xb7 SHARED REVERB SEND");
    Deck::paintZone (g, getLocalBounds(), title, tag);

    // slot activity LED in the zone header
    const float level = juce::jlimit (0.0f, 1.0f, processorRef.getMfxRack().getSlotLevel (slot) * 2.5f);
    juce::Rectangle<float> led ((float) getWidth() - 14.0f - (float) juce::GlyphArrangement::getStringWidth (Deck::mono (8.5f), tag) - 16.0f,
                                (float) Deck::kZoneHeaderH * 0.5f - 3.0f, 6.0f, 6.0f);
    g.setColour (Deck::green().withAlpha (0.2f + 0.8f * level));
    g.fillEllipse (led);

    // generator bar strip
    auto gen = generatorArea().toFloat();
    g.setColour (juce::Colour (0x8c02070c));
    g.fillRoundedRectangle (gen.expanded (4.0f, 2.0f), 3.0f);

    // AMOUNT caption + readout
    g.setFont (Aviation::label (8.5f, 0.14f));
    g.setColour (Aviation::textSecondary());
    const int amtX = surpriseBtn->getRight() + 14;
    g.drawText ("AMOUNT", amtX, generatorArea().getY(), 50, kGenH, juce::Justification::centredLeft);
    g.setFont (Deck::mono (9.0f));
    g.setColour (Aviation::cyan());
    g.drawText (juce::String (juce::roundToInt (amountSlider.getValue() * 100.0)) + "%",
                amountSlider.getRight() + 6, generatorArea().getY(), 40, kGenH, juce::Justification::centredLeft);

    // bank head
    const auto& d = Mfx::descriptor (currentEffect);
    g.setFont (Aviation::label (8.5f, 0.14f));
    g.setColour (Aviation::gold());
    g.drawText ("PARAMETER BANK", bankArea().getX(), bankArea().getY() - 8, 120, 10, juce::Justification::centredLeft);
    g.setFont (Deck::mono (8.0f));
    g.setColour (Aviation::textDim());
    g.drawText (juce::String (d.numUsedParams()) + " OF 16 SLOTS USED",
                bankArea().getX() + 120, bankArea().getY() - 8, bankArea().getWidth() - 120, 10, juce::Justification::centredLeft);
}
