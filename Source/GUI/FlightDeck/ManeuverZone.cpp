#include "ManeuverZone.h"
#include "../../PluginProcessor.h"
#include "../../State/StateSchema.h"

namespace
{
namespace P = AviatorKeyz::ParamID;
} // namespace

// =============================================================================
//  FlipLever — slot with rail + detents and a handle that slams between
//  FWD (right) and REV (left). Animated with a short overshoot.
// =============================================================================
class ManeuverZone::FlipLever : public juce::Component,
                                private juce::Timer
{
public:
    explicit FlipLever (juce::AudioProcessorValueTreeState& a)
        : apvts (a)
    {
        if (auto* p = apvts.getParameter (P::REVERSE))
        {
            attachment = std::make_unique<juce::ParameterAttachment> (*p, [this] (float v)
            {
                reversed = v > 0.5f;
                startTimerHz (60);
            });
            attachment->sendInitialUpdate();
            position = reversed ? 0.0f : 1.0f;
        }
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
    }

    ~FlipLever() override { stopTimer(); }

    void mouseDown (const juce::MouseEvent& e) override
    {
        if (attachment == nullptr)
            return;
        const bool momentaryMode = apvts.getRawParameterValue (P::FLIP_MODE)->load() > 0.5f;
        if (e.mods.isShiftDown() || momentaryMode)
        {
            momentary = true;
            attachment->setValueAsCompleteGesture (1.0f);
            return;
        }
        const bool wantRev = e.x < getWidth() / 2;
        attachment->setValueAsCompleteGesture (wantRev ? 1.0f : 0.0f);
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (attachment == nullptr || momentary)
            return;
        const bool wantRev = e.x < getWidth() / 2;
        if (wantRev != reversed)
            attachment->setValueAsCompleteGesture (wantRev ? 1.0f : 0.0f);
    }

    void mouseUp (const juce::MouseEvent&) override
    {
        if (momentary && attachment != nullptr)
        {
            momentary = false;
            attachment->setValueAsCompleteGesture (0.0f);
        }
    }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();

        // legend
        g.setFont (Aviation::label (9.0f, 0.26f));
        g.setColour (reversed ? Deck::warn() : juce::Colour (0xff2b4356));
        g.drawText (juce::String::fromUTF8 ("\xe2\x97\x80 REV"), r.toNearestInt().removeFromTop (14).withTrimmedLeft (4),
                    juce::Justification::centredLeft);
        g.setColour (reversed ? juce::Colour (0xff2b4356) : Deck::green());
        g.drawText (juce::String::fromUTF8 ("FWD \xe2\x96\xb6"), r.toNearestInt().removeFromTop (14).withTrimmedRight (4),
                    juce::Justification::centredRight);

        auto slot = r.withTrimmedTop (18.0f);
        juce::ColourGradient grad (juce::Colour (0xff020a11), slot.getX(), slot.getY(),
                                   juce::Colour (0xff061019), slot.getX(), slot.getBottom(), false);
        g.setGradientFill (grad);
        g.fillRoundedRectangle (slot, 9.0f);
        g.setColour (Deck::segBorder());
        g.drawRoundedRectangle (slot, 9.0f, 1.0f);
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.drawRoundedRectangle (slot.reduced (1.5f), 8.0f, 2.0f);

        // rail + detents
        const float railY = slot.getCentreY();
        g.setColour (juce::Colour (0xff16293a));
        g.fillRoundedRectangle (slot.getX() + 14.0f, railY - 1.5f, slot.getWidth() - 28.0f, 3.0f, 1.5f);
        g.setColour (Deck::padBorder());
        for (int i = 1; i <= 3; ++i)
        {
            const float x = slot.getX() + slot.getWidth() * (float) i / 4.0f;
            g.fillEllipse (x - 2.5f, railY - 2.5f, 5.0f, 5.0f);
        }

        // handle
        const float handleW = 120.0f;
        const float travel = slot.getWidth() - handleW - 8.0f;
        auto handle = juce::Rectangle<float> (slot.getX() + 4.0f + travel * position, slot.getY() + 4.0f,
                                              handleW, slot.getHeight() - 8.0f);
        g.setColour (juce::Colour (0xff02080e));
        g.fillRoundedRectangle (handle.translated (0.0f, 5.0f), 7.0f);
        g.setColour (juce::Colours::black.withAlpha (0.5f));
        g.fillRoundedRectangle (handle.translated (0.0f, 9.0f).expanded (2.0f), 8.0f);

        const bool rev = reversed;
        juce::ColourGradient hg (rev ? juce::Colour (0xff4a2b22) : juce::Colour (0xff2b3f52), handle.getCentreX(), handle.getY(),
                                 rev ? juce::Colour (0xff170b07) : juce::Colour (0xff0d1b27), handle.getCentreX(), handle.getBottom(), false);
        hg.addColour (0.42, rev ? juce::Colour (0xff2b1710) : juce::Colour (0xff16293a));
        g.setGradientFill (hg);
        g.fillRoundedRectangle (handle, 7.0f);
        if (rev)
        {
            g.setColour (Deck::warn().withAlpha (0.24f));
            g.drawRoundedRectangle (handle.expanded (4.0f), 10.0f, 6.0f);
        }
        g.setColour (rev ? juce::Colour (0xff7a3a28) : juce::Colour (0xff3a5670));
        g.drawRoundedRectangle (handle, 7.0f, 1.0f);
        g.setColour (juce::Colours::white.withAlpha (0.18f));
        g.fillRect (juce::Rectangle<float> (handle.getX() + 6.0f, handle.getY() + 1.0f, handle.getWidth() - 12.0f, 1.0f));

        // arrows + grips
        g.setFont (Deck::mono (15.0f));
        g.setColour (rev ? Deck::warnBright() : Aviation::cyanBright());
        g.drawText (rev ? juce::String::fromUTF8 ("\xe2\x97\x80\xe2\x97\x80") : juce::String::fromUTF8 ("\xe2\x96\xb6\xe2\x96\xb6"),
                    handle.toNearestInt().withTrimmedBottom (14), juce::Justification::centred);
        g.setColour (rev ? Deck::warn().withAlpha (0.25f) : juce::Colours::white.withAlpha (0.12f));
        for (int i = 0; i < 3; ++i)
            g.fillRoundedRectangle (handle.getCentreX() - 26.0f, handle.getBottom() - 14.0f + i * 4.0f, 52.0f, 2.0f, 1.0f);
    }

private:
    void timerCallback() override
    {
        const float target = reversed ? 0.0f : 1.0f;
        // critically-damped-ish spring with a hint of overshoot
        velocity += (target - position) * 0.45f;
        velocity *= 0.55f;
        position += velocity;
        if (std::abs (target - position) < 0.002f && std::abs (velocity) < 0.002f)
        {
            position = target;
            velocity = 0.0f;
            stopTimer();
        }
        repaint();
    }

    juce::AudioProcessorValueTreeState& apvts;
    std::unique_ptr<juce::ParameterAttachment> attachment;
    bool reversed { false };
    bool momentary { false };
    float position { 1.0f };
    float velocity { 0.0f };
};

// =============================================================================
//  MirrorStrip — waveform of the loaded sound with the live playhead. Mirrors
//  horizontally while reversed so the strip reads the way the sample plays.
// =============================================================================
class ManeuverZone::MirrorStrip : public juce::Component
{
public:
    explicit MirrorStrip (AviatorKeyzProcessor& p) : processor (p) { setInterceptsMouseClicks (false, false); }

    void setReversed (bool r) { reversed = r; }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        Deck::paintScreen (g, r);
        rebuildPeaksIfNeeded();

        auto area = r.reduced (2.0f, 4.0f);
        const float mid = area.getCentreY();
        const int cols = (int) peaks.size();
        g.setColour (Aviation::cyan().withAlpha (0.6f));
        for (int i = 0; i < cols; ++i)
        {
            const int col = reversed ? cols - 1 - i : i;
            const float h = peaks[(size_t) col] * area.getHeight() * 0.5f;
            const float x = area.getX() + (float) i * 3.0f;
            g.drawLine (x, mid - h, x, mid + h, 1.0f);
        }

        float ph = processor.getPlayheadNorm();
        if (reversed) ph = 1.0f - ph;
        const float px = area.getX() + ph * (float) cols * 3.0f;
        g.setColour (Aviation::goldBright().withAlpha (0.3f));
        g.fillRect (juce::Rectangle<float> (px - 3.0f, r.getY(), 6.0f, r.getHeight()));
        g.setColour (Aviation::goldBright());
        g.fillRect (juce::Rectangle<float> (px - 1.0f, r.getY(), 2.0f, r.getHeight()));
    }

private:
    void rebuildPeaksIfNeeded()
    {
        const float* data = processor.getFactoryWaveformData();
        const int frames = processor.getFactoryWaveformFrames();
        const int cols = juce::jmax (1, (getWidth() - 4) / 3);
        if (data == cachedData && frames == cachedFrames && (int) peaks.size() == cols)
            return;

        cachedData = data;
        cachedFrames = frames;
        peaks.assign ((size_t) cols, 0.0f);
        if (data == nullptr || frames <= 0)
            return;

        for (int c = 0; c < cols; ++c)
        {
            const int a = (int) ((juce::int64) c * frames / cols);
            const int b = juce::jmax (a + 1, (int) ((juce::int64) (c + 1) * frames / cols));
            float peak = 0.0f;
            const int step = juce::jmax (1, (b - a) / 256);
            for (int i = a; i < b && i < frames; i += step)
                peak = juce::jmax (peak, std::abs (data[i]));
            peaks[(size_t) c] = juce::jlimit (0.02f, 1.0f, peak);
        }
    }

    AviatorKeyzProcessor& processor;
    const float* cachedData { nullptr };
    int cachedFrames { -1 };
    std::vector<float> peaks;
    bool reversed { false };
};

// =============================================================================
ManeuverZone::ManeuverZone (AviatorKeyzProcessor& p)
    : processorRef (p), apvts (p.getAPVTS())
{
    lever = std::make_unique<FlipLever> (apvts);
    addAndMakeVisible (*lever);
    mirror = std::make_unique<MirrorStrip> (processorRef);
    addAndMakeVisible (*mirror);

    windowSeg = std::make_unique<DeckSegment> (apvts, P::FLIP_WINDOW,
        std::vector<DeckSegment::Option> { { "PHRASE", 0 }, { "SLICE", 1 }, { "BEAT", 2 } }, "FLIP WINDOW");
    snapSeg = std::make_unique<DeckSegment> (apvts, P::FLIP_SNAP,
        std::vector<DeckSegment::Option> { { "OFF", 0 }, { "1/4", 1 }, { "1/8", 2 }, { "1/16", 3 } }, "SNAP");
    modeSeg = std::make_unique<DeckSegment> (apvts, P::FLIP_MODE,
        std::vector<DeckSegment::Option> { { "LATCH", 0 }, { "MOMENT", 1 } }, "MODE");
    addAndMakeVisible (*windowSeg);
    addAndMakeVisible (*snapSeg);
    addAndMakeVisible (*modeSeg);

    triggerChip = std::make_unique<DeckChip> ("TRIG", "OFF");
    triggerChip->onClick = [this]
    {
        if (auto* on = apvts.getParameter (P::FLIP_TRIGGER_ON))
            on->setValueNotifyingHost (on->getValue() > 0.5f ? 0.0f : 1.0f);
    };
    triggerChip->onDragTicks = [this] (int ticks)
    {
        if (auto* np = apvts.getParameter (P::FLIP_TRIGGER_NOTE))
        {
            const int cur = juce::roundToInt (np->convertFrom0to1 (np->getValue()));
            np->setValueNotifyingHost (np->convertTo0to1 ((float) juce::jlimit (0, 127, cur + ticks)));
        }
    };
    presetChip = std::make_unique<DeckChip> ("PRESETS", juce::String::fromUTF8 ("\xe2\x96\xbc"), Aviation::goldBright());
    presetChip->onClick = [this] { openPresets(); };
    addAndMakeVisible (*triggerChip);
    addAndMakeVisible (*presetChip);

    if (auto* rp = apvts.getParameter (P::REVERSE))
    {
        reverseAttachment = std::make_unique<juce::ParameterAttachment> (*rp, [this] (float v)
        {
            reversed = v > 0.5f;
            mirror->setReversed (reversed);
            repaint();
        });
        reverseAttachment->sendInitialUpdate();
    }

    startTimerHz (30);
}

ManeuverZone::~ManeuverZone()
{
    stopTimer();
}

void ManeuverZone::timerCallback()
{
    mirror->repaint();
    refreshChips();
}

void ManeuverZone::refreshChips()
{
    const bool on = apvts.getRawParameterValue (P::FLIP_TRIGGER_ON)->load() > 0.5f;
    const int note = (int) apvts.getRawParameterValue (P::FLIP_TRIGGER_NOTE)->load();
    triggerChip->setValue (on ? "MIDI " + Deck::noteName (note) : "OFF " + Deck::noteName (note),
                           on ? Deck::green() : Aviation::textSecondary());
}

void ManeuverZone::openPresets()
{
    // window / snap / mode combos that land musically
    struct FlipPreset { const char* name; int window, snap, mode; };
    static const FlipPreset presets[] = {
        { "Phrase Slam   (phrase, 1/8, latch)",     0, 2, 0 },
        { "Beat Roll     (beat, 1/4, latch)",       2, 1, 0 },
        { "Slice Stutter (slice, 1/16, momentary)", 1, 3, 1 },
        { "Free Scrub    (phrase, off, momentary)", 0, 0, 1 },
        { "Half-Bar Flip (beat, 1/8, momentary)",   2, 2, 1 },
    };
    juce::PopupMenu m;
    m.setLookAndFeel (&menuLookAndFeel);
    m.addSectionHeader ("FLIP PRESETS");
    for (int i = 0; i < (int) std::size (presets); ++i)
        m.addItem (i + 1, presets[i].name);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (presetChip.get()),
                     [safe = juce::Component::SafePointer<ManeuverZone> (this)] (int result)
                     {
                         if (safe == nullptr || result <= 0) return;
                         const auto& pr = presets[(size_t) (result - 1)];
                         auto set = [&] (const char* id, float v)
                         {
                             if (auto* p = safe->apvts.getParameter (id))
                                 p->setValueNotifyingHost (p->convertTo0to1 (v));
                         };
                         set (P::FLIP_WINDOW, (float) pr.window);
                         set (P::FLIP_SNAP, (float) pr.snap);
                         set (P::FLIP_MODE, (float) pr.mode);
                     });
}

void ManeuverZone::resized()
{
    auto body = getLocalBounds().withTrimmedTop (Deck::kZoneHeaderH).reduced (14, 10);

    lever->setBounds (body.removeFromLeft (318).withHeight (82).withY (body.getY() + 4));
    body.removeFromLeft (16);

    auto opts = body.removeFromRight (250);
    body.removeFromRight (12);

    const int segH = DeckSegment::kSegH + DeckSegment::kCaptionH + 2;
    windowSeg->setBounds (opts.getX(), opts.getY() + 2, windowSeg->preferredWidth (8), segH);
    snapSeg->setBounds (opts.getX(), windowSeg->getBottom() + 4, snapSeg->preferredWidth (7), segH);
    modeSeg->setBounds (windowSeg->getRight() + 8, opts.getY() + 2, modeSeg->preferredWidth (7), segH);
    triggerChip->setBounds (snapSeg->getRight() + 8, snapSeg->getY() + 1, 92, DeckChip::kH);
    presetChip->setBounds (snapSeg->getRight() + 8, triggerChip->getBottom() + 4, 92, DeckChip::kH);

    // state block: title + sub drawn in paint; mirror strip below
    mirror->setBounds (body.getX(), body.getY() + 44, body.getWidth(), 44);
}

void ManeuverZone::paint (juce::Graphics& g)
{
    Deck::paintZone (g, getLocalBounds(), juce::String::fromUTF8 ("MANEUVER \xc2\xb7 SAMPLE FLIP"),
                     juce::String::fromUTF8 ("SLAM THE LEVER \xc2\xb7 \xe2\x87\xa7 = MOMENTARY \xc2\xb7 TRIG = MIDI NOTE"));

    auto body = getLocalBounds().withTrimmedTop (Deck::kZoneHeaderH).reduced (14, 10);
    body.removeFromLeft (318 + 16);
    body.removeFromRight (250 + 12);

    g.setFont (Aviation::label (19.0f, 0.30f));
    g.setColour (reversed ? Deck::warn() : Deck::green());
    g.drawText (reversed ? "REVERSED" : "FORWARD", body.getX(), body.getY() + 2, body.getWidth(), 22,
                juce::Justification::centredLeft);

    static const char* snapNames[] = { "OFF", "1/4", "1/8", "1/16" };
    static const char* windowNames[] = { "PHRASE", "SLICE", "BEAT" };
    const int snap = juce::jlimit (0, 3, (int) apvts.getRawParameterValue (P::FLIP_SNAP)->load());
    const int window = juce::jlimit (0, 2, (int) apvts.getRawParameterValue (P::FLIP_WINDOW)->load());

    juce::String sub;
    if (processorRef.isFlipPending())
        sub = juce::String::fromUTF8 ("ARMED \xc2\xb7 FLIPS ON THE NEXT ") + juce::String (snapNames[snap]);
    else if (reversed)
        sub = juce::String::fromUTF8 ("PLAYHEAD \xe2\x86\x90  \xc2\xb7  SNAPPED TO ") + snapNames[snap] + juce::String::fromUTF8 (" \xc2\xb7 ") + windowNames[window] + " WINDOW";
    else
        sub = juce::String::fromUTF8 ("PLAYHEAD \xe2\x86\x92  \xc2\xb7  SLAM THE LEVER TO FLIP MID-PHRASE");

    g.setFont (Deck::mono (8.5f));
    g.setColour (Aviation::textDim());
    g.drawText (sub, body.getX(), body.getY() + 26, body.getWidth(), 12, juce::Justification::centredLeft);
}
