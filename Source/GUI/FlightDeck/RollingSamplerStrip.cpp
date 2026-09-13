#include "RollingSamplerStrip.h"
#include "../../PluginProcessor.h"
#include "../../State/SampleAnalysis.h"
#include "../../State/StateSchema.h"
#include "../Cockpit/PresetDisplayUtils.h"

namespace
{
namespace P = AviatorKeyz::ParamID;

constexpr int kLiveW    = 62;   // on/off pill
constexpr int kLiveH    = 18;
constexpr int kBtnH     = 18;
constexpr int kCargoW   = 82;
constexpr int kDragW    = 74;
constexpr int kReadoutW = 300;
constexpr int kDurW     = 62;   // selected-length readout, right of the wave
constexpr int kGap      = 8;
constexpr float kHandleHit = 6.f;
} // namespace

RollingSamplerStrip::RollingSamplerStrip (AviatorKeyzProcessor& p)
    : processorRef (p)
{
    startTimerHz (30); // the window scrolls, so this repaints continuously
    timerCallback();
}

RollingSamplerStrip::~RollingSamplerStrip()
{
    stopTimer();
}

// -----------------------------------------------------------------------------
//  layout — one row, everything vertically centred
// -----------------------------------------------------------------------------
juce::Rectangle<int> RollingSamplerStrip::liveBounds() const
{
    return { 0, (getHeight() - kLiveH) / 2, kLiveW, kLiveH };
}

juce::Rectangle<int> RollingSamplerStrip::readoutBounds() const
{
    return { getWidth() - kReadoutW, 0, kReadoutW, getHeight() };
}

juce::Rectangle<int> RollingSamplerStrip::dragBounds() const
{
    return { readoutBounds().getX() - kGap - kDragW, (getHeight() - kBtnH) / 2, kDragW, kBtnH };
}

juce::Rectangle<int> RollingSamplerStrip::cargoBounds() const
{
    return { dragBounds().getX() - 6 - kCargoW, (getHeight() - kBtnH) / 2, kCargoW, kBtnH };
}

juce::Rectangle<int> RollingSamplerStrip::waveBounds() const
{
    const int x = liveBounds().getRight() + 12;
    const int right = cargoBounds().getX() - kGap - kDurW;
    return { x, 5, juce::jmax (40, right - x), getHeight() - 10 };
}

void RollingSamplerStrip::resized()
{
    const int cols = juce::jmax (1, waveBounds().getWidth());
    if ((int) columns.size() != cols)
        columns.assign ((size_t) cols, RollingSampler::Bucket {});
}

// -----------------------------------------------------------------------------
//  frame <-> pixel
// -----------------------------------------------------------------------------
float RollingSamplerStrip::xForFrame (int64_t frame) const
{
    const auto w = waveBounds();
    const double t = (double) (frame - viewOldest) / (double) juce::jmax ((int64_t) 1, windowFrames);
    return (float) w.getX() + (float) juce::jlimit (0.0, 1.0, t) * (float) w.getWidth();
}

int64_t RollingSamplerStrip::frameForX (int x) const
{
    const auto w = waveBounds();
    const double t = juce::jlimit (0.0, 1.0, (double) (x - w.getX()) / (double) juce::jmax (1, w.getWidth()));
    return viewOldest + (int64_t) (t * (double) windowFrames);
}

bool RollingSamplerStrip::regionIsAging() const noexcept
{
    return hasRegion() && regionStart < processorRef.getRollingOldestFrame();
}

double RollingSamplerStrip::regionSeconds() const
{
    return (double) (regionEnd - regionStart) / juce::jmax (1.0, processorRef.getRollingSampleRate());
}

// -----------------------------------------------------------------------------
//  state
// -----------------------------------------------------------------------------
void RollingSamplerStrip::timerCallback()
{
    const double sr = juce::jmax (1.0, processorRef.getRollingSampleRate());
    windowFrames = juce::jmax ((int64_t) 1, (int64_t) std::llround (processorRef.getRollingWindowSeconds() * sr));
    viewNow = processorRef.getRollingNowFrame();
    // Always show a full 30 s frame of reference, so audio visibly rolls in from
    // the right rather than stretching to fill the strip.
    viewOldest = viewNow - windowFrames;

    if (hasRegion() && regionEnd <= processorRef.getRollingOldestFrame())
    {
        regionStart = regionEnd = 0;
        showStatus ("REGION ROLLED OUT", true);
    }

    const float level = processorRef.getRollingSamplerLevel();
    meterLevel = level > meterLevel ? level : meterLevel * 0.8f + level * 0.2f;
    heldSeconds = processorRef.getRollingSamplerSeconds();

    if (statusText.isNotEmpty() && juce::Time::getMillisecondCounter() > statusUntilMs)
        statusText.clear();

    const int cols = juce::jmax (1, waveBounds().getWidth());
    if ((int) columns.size() != cols)
        columns.assign ((size_t) cols, RollingSampler::Bucket {});
    processorRef.readRollingEnvelope (columns.data(), cols, viewOldest, viewNow);

    // SRC / KEY / BPM / MODE — this row has always carried it.
    auto& pm = processorRef.getPresetManager();
    const auto& info = processorRef.getUserSampleInfo();
    static const char* modeNames[] = { "ONE-SHOT", "PHRASE", "CHROMATIC", "STRETCH", "SLICE-PHRASE" };
    const int mode = juce::jlimit (0, 4, (int) processorRef.getAPVTS().getRawParameterValue (P::SRC_PLAYBACK_MODE)->load());

    juce::String src = info.loaded ? info.name.toUpperCase()
                                   : PresetDisplayUtils::shortenDisplayName (pm.getCurrentPresetName()).toUpperCase();
    if (src.length() > 22)
        src = src.substring (0, 21) + juce::String::fromUTF8 ("\xe2\x80\xa6");
    const juce::String key = info.loaded ? SampleAnalysis::keyName (info.keyPitchClass, info.keyMinor).toUpperCase()
                                         : Deck::noteName (pm.getCurrentRootNote());

    readoutLine1 = "SRC " + src + juce::String::fromUTF8 (" \xc2\xb7 KEY ") + key + juce::String::fromUTF8 (" \xc2\xb7 ")
                   + juce::String (juce::roundToInt (processorRef.getLastKnownHostBpm())) + " BPM";
    readoutLine2 = juce::String ("MODE ") + modeNames[mode] + juce::String::fromUTF8 (" \xc2\xb7 VOICES ")
                   + juce::String::formatted ("%02d", processorRef.getActiveVoiceCount());

    repaint();
}

void RollingSamplerStrip::showStatus (const juce::String& text, bool isError)
{
    statusText = text;
    statusIsError = isError;
    statusUntilMs = juce::Time::getMillisecondCounter() + 3500;
}

void RollingSamplerStrip::toggleLive()
{
    const bool nowLive = ! processorRef.isRollingSamplerArmed();
    processorRef.setRollingSamplerArmed (nowLive);
    if (! nowLive)
    {
        processorRef.clearRollingSampler();
        regionStart = regionEnd = 0;
    }
    showStatus (nowLive ? "LIVE" : "OFF", false);
}

void RollingSamplerStrip::selectAll()
{
    regionStart = juce::jmax (viewOldest, processorRef.getRollingOldestFrame());
    regionEnd = viewNow;
    if (! hasRegion())
        showStatus ("NOTHING CAPTURED YET", true);
}

void RollingSamplerStrip::sendToCargo()
{
    if (! hasRegion())
        selectAll();          // no marked region: the whole window is the obvious intent
    if (! hasRegion())
        return;

    juce::String error;
    if (! processorRef.loadRollingRange (regionStart, regionEnd, error))
    {
        showStatus (error.toUpperCase(), true);
        return;
    }

    showStatus (juce::String (regionSeconds(), 2) + "S TO CARGO", false);
    if (onRegionLoaded)
        onRegionLoaded();
}

void RollingSamplerStrip::beginExternalDrag()
{
    if (dragStarted)
        return;
    if (! hasRegion())
        selectAll();
    if (! hasRegion())
        return;

    juce::File file;
    juce::String error;
    if (! processorRef.exportRollingRange (regionStart, regionEnd, file, error))
    {
        showStatus (error.toUpperCase(), true);
        return;
    }

    dragStarted = true;
    showStatus ("DRAGGING WAV", false);
    // WAV is what every DAW accepts on a file drop; the file stays in the
    // capture folder so the DAW can reference it after the drop.
    juce::DragAndDropContainer::performExternalDragDropOfFiles ({ file.getFullPathName() }, false, this);
}

// -----------------------------------------------------------------------------
//  mouse
// -----------------------------------------------------------------------------
void RollingSamplerStrip::mouseMove (const juce::MouseEvent& e)
{
    int item = -1;
    if (liveBounds().contains (e.getPosition()))       item = 0;
    else if (cargoBounds().contains (e.getPosition())) item = 1;
    else if (dragBounds().contains (e.getPosition()))  item = 2;

    auto cursor = juce::MouseCursor::NormalCursor;
    if (item >= 0)
    {
        cursor = juce::MouseCursor::PointingHandCursor;
    }
    else if (waveBounds().contains (e.getPosition()))
    {
        const float x = (float) e.x;
        if (hasRegion() && (std::abs (x - xForFrame (regionStart)) < kHandleHit
                            || std::abs (x - xForFrame (regionEnd)) < kHandleHit))
            cursor = juce::MouseCursor::LeftRightResizeCursor;
        else if (hasRegion() && x > xForFrame (regionStart) && x < xForFrame (regionEnd))
            cursor = juce::MouseCursor::DraggingHandCursor; // drag the region out
        else
            cursor = juce::MouseCursor::IBeamCursor;        // drag to mark
    }
    setMouseCursor (cursor);

    if (item != hoverItem)
    {
        hoverItem = item;
        repaint();
    }
}

void RollingSamplerStrip::mouseExit (const juce::MouseEvent&)
{
    if (hoverItem >= 0)
    {
        hoverItem = -1;
        repaint();
    }
}

void RollingSamplerStrip::mouseDown (const juce::MouseEvent& e)
{
    dragStarted = false;
    grab = Grab::none;

    if (liveBounds().contains (e.getPosition()))  { toggleLive();  return; }
    if (cargoBounds().contains (e.getPosition())) { sendToCargo(); return; }
    if (dragBounds().contains (e.getPosition()))  { grab = Grab::dragOut; return; }

    if (! waveBounds().contains (e.getPosition()))
        return;

    if (hasRegion())
    {
        const float x = (float) e.x;
        if (std::abs (x - xForFrame (regionStart)) < kHandleHit) { grab = Grab::moveStart; return; }
        if (std::abs (x - xForFrame (regionEnd)) < kHandleHit)   { grab = Grab::moveEnd;   return; }
        if (x > xForFrame (regionStart) && x < xForFrame (regionEnd))
        {
            grab = Grab::dragOut; // inside the region: becomes a file drag on move
            return;
        }
    }

    grab = Grab::newRegion;
    regionStart = regionEnd = frameForX (e.x);
}

void RollingSamplerStrip::mouseDrag (const juce::MouseEvent& e)
{
    if (grab == Grab::dragOut)
    {
        if (e.getDistanceFromDragStart() > 6)
            beginExternalDrag();
        return;
    }

    const int64_t f = frameForX (e.x);
    switch (grab)
    {
        case Grab::newRegion:
        {
            const int64_t anchor = regionStart;
            regionStart = juce::jmin (anchor, f);
            regionEnd = juce::jmax (anchor, f);
            break;
        }
        case Grab::moveStart: regionStart = juce::jmin (f, regionEnd - 1); break;
        case Grab::moveEnd:   regionEnd = juce::jmax (f, regionStart + 1); break;
        default: return;
    }
    repaint();
}

void RollingSamplerStrip::mouseUp (const juce::MouseEvent&)
{
    if (grab == Grab::newRegion && regionEnd - regionStart < windowFrames / 400)
        regionStart = regionEnd = 0; // a click, not a drag: clear the region
    grab = Grab::none;
    dragStarted = false;
    repaint();
}

void RollingSamplerStrip::mouseDoubleClick (const juce::MouseEvent& e)
{
    if (waveBounds().contains (e.getPosition()))
        selectAll();
}

// -----------------------------------------------------------------------------
//  paint
// -----------------------------------------------------------------------------
void RollingSamplerStrip::paint (juce::Graphics& g)
{
    const bool live = processorRef.isRollingSamplerArmed();
    const auto accent = Deck::green();

    // --- on/off pill ----------------------------------------------------------
    //  A slim outline with a small status LED — it sits above the whole deck and
    //  shouldn't compete with it.
    {
        auto r = liveBounds().toFloat();
        const bool hot = hoverItem == 0;
        const float radius = r.getHeight() * 0.5f;

        g.setColour (live ? accent.withAlpha (0.10f) : juce::Colour (0xff0a1119));
        g.fillRoundedRectangle (r, radius);
        g.setColour (live ? accent.withAlpha (hot ? 0.75f : 0.55f)
                          : Aviation::textDim().withAlpha (hot ? 0.65f : 0.32f));
        g.drawRoundedRectangle (r.reduced (0.5f), radius, 1.0f);

        const float cx = r.getX() + 11.0f, cy = r.getCentreY();
        if (live)
        {
            g.setColour (accent.withAlpha (0.18f));
            g.fillEllipse (cx - 5.5f, cy - 5.5f, 11.0f, 11.0f);
            g.setColour (accent);
            g.fillEllipse (cx - 2.5f, cy - 2.5f, 5.0f, 5.0f);
        }
        else
        {
            g.setColour (Aviation::textDim().withAlpha (0.5f));
            g.drawEllipse (cx - 2.5f, cy - 2.5f, 5.0f, 5.0f, 1.0f);
        }

        g.setFont (Aviation::label (8.0f, 0.14f));
        g.setColour (live ? accent : Aviation::textDim().withAlpha (hot ? 1.0f : 0.65f));
        g.drawText ("LIVE", liveBounds().withTrimmedLeft (19), juce::Justification::centredLeft);
    }

    // --- inline live window ---------------------------------------------------
    auto w = waveBounds();
    Deck::paintScreen (g, w.toFloat());

    const float mid = (float) w.getCentreY();
    const float half = (float) w.getHeight() * 0.44f;

    // 5-second ticks for a sense of scale
    g.setColour (Deck::screenLine());
    for (int s = 5; s < 30; s += 5)
        g.drawVerticalLine (w.getX() + juce::roundToInt ((float) w.getWidth() * (float) s / 30.f),
                            (float) w.getY() + 2.f, (float) w.getBottom() - 2.f);

    if (hasRegion())
    {
        const float rx0 = xForFrame (regionStart), rx1 = xForFrame (regionEnd);
        g.setColour ((regionIsAging() ? Deck::warn() : Aviation::gold()).withAlpha (0.16f));
        g.fillRect (juce::Rectangle<float> (rx0, (float) w.getY(), juce::jmax (1.f, rx1 - rx0), (float) w.getHeight()));
    }

    const int cols = juce::jmin ((int) columns.size(), w.getWidth());
    for (int i = 0; i < cols; ++i)
    {
        const auto& b = columns[(size_t) i];
        if (std::abs (b.max) < 1.0e-5f && std::abs (b.min) < 1.0e-5f)
            continue;
        const float x = (float) (w.getX() + i);
        const bool inRegion = hasRegion() && x >= xForFrame (regionStart) && x <= xForFrame (regionEnd);
        const float top = mid - juce::jlimit (0.f, 1.f, b.max) * half;
        const float bot = mid - juce::jlimit (-1.f, 0.f, b.min) * half;
        g.setColour (inRegion ? Aviation::goldBright().withAlpha (0.95f)
                              : Aviation::cyan().withAlpha (live ? 0.7f : 0.28f));
        g.drawLine (x, top, x, juce::jmax (bot, top + 1.f), 1.0f);
    }

    if (hasRegion())
    {
        const float rx0 = xForFrame (regionStart), rx1 = xForFrame (regionEnd);
        g.setColour (regionIsAging() ? Deck::warn() : Aviation::goldBright());
        g.drawLine (rx0, (float) w.getY(), rx0, (float) w.getBottom(), 1.5f);
        g.drawLine (rx1, (float) w.getY(), rx1, (float) w.getBottom(), 1.5f);
    }

    // "now" edge — audio enters here
    g.setColour (live ? accent : Aviation::textDim().withAlpha (0.4f));
    g.drawLine ((float) w.getRight() - 1.f, (float) w.getY(), (float) w.getRight() - 1.f, (float) w.getBottom(), 1.5f);

    if (! live)
    {
        g.setFont (Deck::mono (8.5f));
        g.setColour (Aviation::textDim().withAlpha (0.75f));
        g.drawText (juce::String::fromUTF8 ("LIVE SAMPLER OFF \xc2\xb7 SWITCH ON TO HOLD THE LAST 30s"),
                    w, juce::Justification::centred);
    }
    else if (statusText.isNotEmpty())
    {
        g.setFont (Deck::mono (8.0f));
        g.setColour (statusIsError ? Deck::warn() : accent);
        g.drawText (statusText, w.getX() + 6, w.getY() + 1, 200, 10, juce::Justification::centredLeft);
    }

    // --- selected length ------------------------------------------------------
    {
        const juce::Rectangle<int> dur { w.getRight() + 6, 0, kDurW, getHeight() };
        g.setFont (Deck::mono (8.5f));
        if (hasRegion())
        {
            g.setColour (regionIsAging() ? Deck::warn() : Aviation::goldBright());
            g.drawText (juce::String (regionSeconds(), 2) + "s", dur, juce::Justification::centredLeft);
        }
        else
        {
            g.setColour (Aviation::textDim().withAlpha (0.6f));
            g.drawText (juce::String (heldSeconds, 0) + "/30s", dur, juce::Justification::centredLeft);
        }
    }

    // --- actions --------------------------------------------------------------
    auto button = [&] (juce::Rectangle<int> r, const juce::String& text, bool hot, juce::Colour col)
    {
        auto rf = r.toFloat();
        g.setColour (live ? juce::Colour (0xff0e1a26) : juce::Colour (0xff0a1119));
        g.fillRoundedRectangle (rf, 3.0f);
        g.setColour (live ? col.withAlpha (hot ? 0.85f : 0.45f) : Deck::segBorder());
        g.drawRoundedRectangle (rf, 3.0f, 1.0f);
        g.setFont (Aviation::label (8.0f, 0.14f));
        g.setColour (live ? col : Aviation::textDim().withAlpha (0.45f));
        g.drawText (text, r, juce::Justification::centred);
    };
    button (cargoBounds(), "TO CARGO", hoverItem == 1, Aviation::goldBright());
    button (dragBounds(),  "DRAG WAV", hoverItem == 2, Aviation::goldBright());

    // --- SRC / KEY / BPM / MODE readout --------------------------------------
    auto drawSysLine = [&] (const juce::String& line, juce::Rectangle<int> area)
    {
        auto tokens = juce::StringArray::fromTokens (line, false);
        const auto f = Deck::mono (9.0f);
        struct Piece { juce::String text; bool value; };
        std::vector<Piece> pieces;
        static const juce::StringArray labels { "SRC", "KEY", "MODE", "VOICES", juce::String::fromUTF8 ("\xc2\xb7") };
        bool nextIsValue = false;
        for (const auto& t : tokens)
        {
            const bool isLabel = labels.contains (t);
            pieces.push_back ({ t + " ", ! isLabel && (nextIsValue || t.containsOnly ("0123456789") || t == "BPM") });
            nextIsValue = isLabel && t != juce::String::fromUTF8 ("\xc2\xb7");
        }
        float total = 0.0f;
        for (const auto& pc : pieces)
            total += juce::GlyphArrangement::getStringWidth (f, pc.text);
        float x = (float) area.getRight() - total;
        for (const auto& pc : pieces)
        {
            g.setFont (f);
            g.setColour (pc.value ? Aviation::cyan() : Aviation::textDim());
            const float pw = juce::GlyphArrangement::getStringWidth (f, pc.text);
            g.drawText (pc.text, (int) x, area.getY(), (int) pw + 2, area.getHeight(), juce::Justification::centredLeft);
            x += pw;
        }
    };
    const auto ro = readoutBounds();
    drawSysLine (readoutLine1, ro.withTrimmedTop (4).withHeight (14));
    drawSysLine (readoutLine2, ro.withTrimmedTop (20).withHeight (14));
}
