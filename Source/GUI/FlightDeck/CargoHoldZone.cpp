#include "CargoHoldZone.h"
#include "../../DSP/Performance/PerformanceTypes.h"
#include "../../DSP/Performance/SliceGrid.h"
#include "../../PluginProcessor.h"
#include "../../State/SampleAnalysis.h"
#include "../../State/StateSchema.h"

namespace
{
namespace P = AviatorKeyz::ParamID;

juce::String timeText (double seconds)
{
    const int mins = (int) (seconds / 60.0);
    const double rem = seconds - mins * 60.0;
    return juce::String::formatted ("%02d:%05.2f", mins, rem);
}

// Anything registerBasicFormats() decodes on both platforms: PCM, FLAC/Ogg
// (JUCE), MP3 (JUCE_USE_MP3AUDIOFORMAT) and the AAC family (CoreAudio / WMF).
// Compressed cargo matters because the rolling sampler's bounces come back
// through this same drop path.
bool isAcceptedAudioFile (const juce::String& path)
{
    static const juce::StringArray kExts { ".wav", ".aif", ".aiff", ".flac", ".ogg", ".mp3", ".m4a", ".aac" };
    return kExts.contains (juce::File (path).getFileExtension().toLowerCase());
}
} // namespace

// =============================================================================
//  DropZone — dashed target; click to browse.
// =============================================================================
class CargoHoldZone::DropZone : public juce::Component
{
public:
    explicit DropZone (std::function<void()> onClick) : clickHandler (std::move (onClick))
    {
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
    }

    void setHighlighted (bool h) { highlighted = h; repaint(); }
    void setMessage (const juce::String& m, bool isError) { message = m; error = isError; repaint(); }

    void mouseUp (const juce::MouseEvent& e) override
    {
        if (clickHandler && getLocalBounds().contains (e.getPosition()))
            clickHandler();
    }
    void mouseEnter (const juce::MouseEvent&) override { hover = true; repaint(); }
    void mouseExit (const juce::MouseEvent&) override { hover = false; repaint(); }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat().reduced (1.0f);
        g.setColour (juce::Colour (0xff06111b));
        g.fillRoundedRectangle (r, 8.0f);

        juce::Path border;
        border.addRoundedRectangle (r, 8.0f);
        const float dashes[] = { 5.0f, 4.0f };
        juce::PathStrokeType stroke (1.5f);
        juce::Path dashed;
        stroke.createDashedStroke (dashed, border, dashes, 2);
        const auto accent = error ? Deck::warn() : (highlighted || hover ? Aviation::gold() : juce::Colour (0xff2a4258));
        g.setColour (accent);
        g.fillPath (dashed);

        const auto textCol = error ? Deck::warnBright() : (highlighted || hover ? Aviation::goldBright() : Aviation::textSecondary());
        g.setFont (Aviation::sans (22.0f));
        g.setColour (textCol);
        g.drawText (juce::String::fromUTF8 ("\xe2\xac\x87"), r.toNearestInt().withTrimmedBottom (54), juce::Justification::centred);
        g.setFont (Aviation::label (9.0f, 0.20f));
        g.drawText (error ? "CARGO REFUSED" : "DROP CARGO", r.toNearestInt().withTrimmedTop (60).withHeight (14), juce::Justification::centred);

        g.setFont (Deck::mono (7.5f));
        g.setColour (error ? Deck::warnBright() : Aviation::textDim());
        const juce::String body = message.isNotEmpty() ? message
                                                       : "DRAG A PHRASE OR LOOP\nUP TO 60 SECONDS\nOR CLICK TO BROWSE";
        g.drawFittedText (body, r.toNearestInt().withTrimmedTop (78).reduced (8, 0), juce::Justification::centredTop, 4);
    }

private:
    std::function<void()> clickHandler;
    juce::String message;
    bool highlighted { false };
    bool hover { false };
    bool error { false };
};

// =============================================================================
//  WaveBox — waveform with trim handles bound to src_start / src_end.
// =============================================================================
class CargoHoldZone::WaveBox : public juce::Component
{
public:
    WaveBox (AviatorKeyzProcessor& p, juce::AudioProcessorValueTreeState& a)
        : processor (p), apvts (a)
    {
        startParam = apvts.getParameter (P::SRC_START);
        endParam = apvts.getParameter (P::SRC_END);
        loopStartParam = apvts.getParameter (P::SRC_LOOP_START);
        loopEndParam = apvts.getParameter (P::SRC_LOOP_END);
        xfadeParam = apvts.getParameter (P::SLICE_XFADE);
        for (int i = 0; i < P::SLICE_CUT_COUNT; ++i)
            cutParams[(size_t) i] = apvts.getParameter (P::sliceCutParamId (i));
    }

    /** Double-click a slice cut to reset it; elsewhere in LOOP, re-detect loop points. */
    void mouseDoubleClick (const juce::MouseEvent& e) override
    {
        const int cut = cutAt (e.x);
        if (cut >= 0)
        {
            if (auto* p = cutParams[(size_t) cut])
                p->setValueNotifyingHost (p->convertTo0to1 (0.f));
            repaint();
            return;
        }
        if (loopActive())
        {
            processor.autoDetectLoopPoints();
            repaint();
        }
    }

    void mouseMove (const juce::MouseEvent& e) override
    {
        const int h = handleAt (e.x, e.y);
        setMouseCursor (h >= 0 ? juce::MouseCursor::LeftRightResizeCursor : juce::MouseCursor::NormalCursor);
        const int cut = cutAt (e.x);
        if (cut != hoverCut)
        {
            hoverCut = cut;
            repaint();
        }
    }

    void mouseExit (const juce::MouseEvent&) override
    {
        if (hoverCut >= 0)
        {
            hoverCut = -1;
            repaint();
        }
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        dragging = handleAt (e.x, e.y);
        if (dragging == kFadeInHandle || dragging == kFadeOutHandle)
        {
            // Relative drag: grabbing the tab must not jump the value.
            dragStartX = (float) e.x;
            dragStartMs = xfadeMs();
            if (auto* fp = paramFor (dragging))
                fp->beginChangeGesture();
            repaint();
            return;
        }
        if (dragging < 0)
        {
            // click sets the nearer handle
            const float n = normAt (e.x);
            dragging = std::abs (n - startNorm()) < std::abs (n - endNorm()) ? 0 : 1;
        }
        if (auto* p = paramFor (dragging))
            p->beginChangeGesture();
        applyDrag (e.x);
    }

    void mouseDrag (const juce::MouseEvent& e) override { applyDrag (e.x); }

    void mouseUp (const juce::MouseEvent&) override
    {
        if (dragging >= 0)
            if (auto* p = paramFor (dragging))
                p->endChangeGesture();
        dragging = -1;
    }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        Deck::paintScreen (g, r);
        rebuildPeaksIfNeeded();

        auto area = waveArea();
        const float mid = area.getCentreY();
        g.setColour (Deck::screenLine());
        g.drawHorizontalLine ((int) mid, area.getX(), area.getRight());

        const float s = startNorm(), e = endNorm();
        const int cols = (int) peaks.size();
        for (int i = 0; i < cols; ++i)
        {
            const float n = (float) i / (float) juce::jmax (1, cols - 1);
            const bool inside = n >= s && n <= e;
            const float h = peaks[(size_t) i] * area.getHeight() * 0.48f;
            const float x = area.getX() + (float) i * 3.0f;
            g.setColour (inside ? Aviation::cyan().withAlpha (0.75f) : Aviation::textDim().withAlpha (0.45f));
            g.drawLine (x, mid - h, x, mid + h, 1.0f);
        }

        // playhead
        const float ph = processor.getPlayheadNorm();
        const float px = xForNorm (ph);
        g.setColour (Aviation::goldBright().withAlpha (0.9f));
        g.drawLine (px, area.getY(), px, area.getBottom(), 1.5f);

        // SLICE mode: SLICE_DIV pads across the trimmed window (C1 = pad 1).
        // Cuts are drawn where SliceGrid actually places them, offsets included,
        // and each interior cut can be dragged.
        if (sliceActive())
        {
            const auto sl = sliceSettings();
            const float x0 = xForNorm (s), x1 = xForNorm (e);
            g.setFont (Deck::mono (7.0f));
            for (int i = 0; i < sl.divisions; ++i)
            {
                int fa = 0, fb = 0;
                SliceGrid::sliceBounds (sl, 0, kCutScale, i, fa, fb);
                const float xa = x0 + (x1 - x0) * (float) fa / (float) kCutScale;
                if (i > 0)
                {
                    const bool hot = (hoverCut == i - 1 || dragging == kCutHandleBase + i - 1);
                    g.setColour (Aviation::gold().withAlpha (hot ? 0.95f : 0.35f));
                    g.drawLine (xa, area.getY(), xa, area.getBottom(), hot ? 1.6f : 0.8f);
                }
                g.setColour (Aviation::gold().withAlpha (0.55f));
                g.drawText (juce::String (i + 1), (int) xa + 2, (int) area.getBottom() - 10, 18, 9,
                            juce::Justification::centredLeft);
            }
        }

        // trim handles
        auto drawHandle = [&] (float norm, bool left)
        {
            const float x = xForNorm (norm);
            g.setColour (Aviation::gold());
            g.drawLine (x, r.getY() + 4.0f, x, r.getBottom() - 4.0f, 2.0f);
            g.fillRect (juce::Rectangle<float> (left ? x : x - 7.0f, r.getY() + 4.0f, 7.0f, 10.0f));
        };
        drawHandle (s, true);
        drawHandle (e, false);
        g.setColour (juce::Colours::black.withAlpha (0.35f));
        g.fillRect (juce::Rectangle<float> (area.getX(), r.getY() + 1.0f, xForNorm (s) - area.getX(), r.getHeight() - 2.0f));
        g.fillRect (juce::Rectangle<float> (xForNorm (e), r.getY() + 1.0f, area.getRight() - xForNorm (e), r.getHeight() - 2.0f));

        // CHOP FADE corners: grab either top corner and drag inward, the way an
        // audio clip fades in a DAW. The fade applies to every chop / slice edge,
        // so a clicky chop can be smoothed by hand. The tab stays grabbable at 0.
        {
            const float fw = fadeWidthPx();
            auto drawFade = [&] (float xEdge, bool left, int handle)
            {
                const float dir = left ? 1.0f : -1.0f;
                if (fw > 0.5f)
                {
                    juce::Path ramp;
                    ramp.startNewSubPath (xEdge, area.getBottom());
                    ramp.lineTo (xEdge + dir * fw, area.getY());
                    ramp.lineTo (xEdge, area.getY());
                    ramp.closeSubPath();
                    g.setColour (Aviation::gold().withAlpha (0.16f));
                    g.fillPath (ramp);
                    g.setColour (Aviation::gold().withAlpha (0.75f));
                    g.drawLine (xEdge, area.getBottom(), xEdge + dir * fw, area.getY(), 1.2f);
                }
                const float tabX = xEdge + dir * juce::jmax (fw, 10.0f);
                g.setColour (dragging == handle ? Aviation::goldBright() : Aviation::gold().withAlpha (0.8f));
                g.fillRect (juce::Rectangle<float> (tabX - 3.0f, area.getY(), 6.0f, 6.0f));
            };
            drawFade (xForNorm (s), true, kFadeInHandle);
            drawFade (xForNorm (e), false, kFadeOutHandle);

            if (dragging == kFadeInHandle || dragging == kFadeOutHandle)
            {
                g.setFont (Deck::mono (8.0f));
                g.setColour (Aviation::goldBright());
                g.drawText ("CHOP FADE " + juce::String (xfadeMs(), 1) + " ms",
                            (int) area.getX() + 6, (int) area.getY() + 9, 150, 10,
                            juce::Justification::centredLeft);
            }
        }

        // sustain loop (LOOP mode): cyan region + bottom-anchored handles, so
        // they read apart from the gold trim handles at the top
        if (loopActive())
        {
            const float lx0 = xForNorm (loopStartNorm()), lx1 = xForNorm (loopEndNorm());
            g.setColour (Aviation::cyan().withAlpha (0.10f));
            g.fillRect (juce::Rectangle<float> (lx0, area.getY(), juce::jmax (0.0f, lx1 - lx0), area.getHeight()));
            auto drawLoopHandle = [&] (float x, bool left)
            {
                g.setColour (Aviation::cyan());
                g.drawLine (x, area.getY(), x, r.getBottom() - 4.0f, 1.5f);
                g.fillRect (juce::Rectangle<float> (left ? x : x - 7.0f, r.getBottom() - 14.0f, 7.0f, 10.0f));
            };
            drawLoopHandle (lx0, true);
            drawLoopHandle (lx1, false);
            g.setFont (Deck::mono (7.0f));
            g.setColour (Aviation::cyan().withAlpha (0.8f));
            g.drawText ("LOOP", (int) lx0 + 4, (int) area.getY() + 2, 30, 9, juce::Justification::centredLeft);
        }

        // readouts
        const auto& info = processor.getUserSampleInfo();
        g.setFont (Deck::mono (9.0f));
        g.setColour (Aviation::goldBright());
        const juce::String name = info.loaded ? info.name : processor.getPresetManager().getCurrentPresetName();
        g.drawText ((info.loaded ? "" : juce::String::fromUTF8 ("FACTORY \xc2\xb7 ")) + name, 10, 5, getWidth() / 2, 12, juce::Justification::centredLeft);

        g.setColour (Aviation::cyan());
        const double secs = totalSeconds();
        juce::String line1 = timeText (secs) + " / 60s MAX";
        juce::String line2;
        if (info.loaded)
        {
            line2 = "DETECTED ";
            line2 << (info.detectedBpm > 1.f ? juce::String (juce::roundToInt (info.detectedBpm)) + " BPM" : juce::String ("-- BPM"))
                  << juce::String::fromUTF8 (" \xc2\xb7 ") << SampleAnalysis::keyName (info.keyPitchClass, info.keyMinor).toUpperCase();
        }
        else
        {
            line2 = "ROOT " + Deck::noteName (processor.getPresetManager().getCurrentRootNote())
                    + juce::String::fromUTF8 (" \xc2\xb7 ") + juce::String (juce::roundToInt (processor.getPresetManager().getCurrentOriginalBpm())) + " BPM";
        }
        g.drawText (line1, getWidth() / 2, 5, getWidth() / 2 - 10, 12, juce::Justification::centredRight);
        g.drawText (line2, getWidth() / 2, 18, getWidth() / 2 - 10, 12, juce::Justification::centredRight);
    }

    double totalSeconds() const
    {
        const auto& info = processor.getUserSampleInfo();
        if (info.loaded)
            return info.seconds;
        const int frames = processor.getFactoryWaveformFrames();
        const double rate = 44100.0;
        return frames > 0 ? frames / rate : 0.0;
    }

    float startNorm() const { return apvts.getRawParameterValue (P::SRC_START)->load(); }
    float endNorm() const { return apvts.getRawParameterValue (P::SRC_END)->load(); }
    float loopStartNorm() const { return apvts.getRawParameterValue (P::SRC_LOOP_START)->load(); }
    float loopEndNorm() const { return apvts.getRawParameterValue (P::SRC_LOOP_END)->load(); }
    bool loopActive() const { return (int) apvts.getRawParameterValue (P::SRC_LOOP_MODE)->load() == 1; }
    bool sliceActive() const { return (int) apvts.getRawParameterValue (P::SRC_PLAYBACK_MODE)->load() == 4; }
    float xfadeMs() const { return apvts.getRawParameterValue (P::SLICE_XFADE)->load(); }

    /** Width of the CHOP FADE ramp in pixels, drawn to scale with the waveform. */
    float fadeWidthPx() const
    {
        const double secs = totalSeconds();
        if (secs <= 0.0)
            return 0.f;
        const float span = (float) juce::jmax (1, (int) peaks.size() - 1) * 3.0f;
        return juce::jmax (0.f, xfadeMs() * 0.001f * span / (float) secs);
    }

    SliceSettings sliceSettings() const
    {
        SliceSettings sl;
        sl.divisions = SliceGrid::divisionsForChoice ((int) apvts.getRawParameterValue (P::SLICE_DIV)->load());
        for (int i = 0; i < P::SLICE_CUT_COUNT; ++i)
            sl.cutOffsets[(size_t) i] = apvts.getRawParameterValue (P::sliceCutParamId (i))->load();
        return sl;
    }

private:
    juce::Rectangle<float> waveArea() const { return getLocalBounds().toFloat().reduced (2.0f).withTrimmedTop (30.0f).withTrimmedBottom (4.0f); }
    float xForNorm (float n) const { auto a = waveArea(); return a.getX() + juce::jlimit (0.0f, 1.0f, n) * (float) juce::jmax (1, (int) peaks.size() - 1) * 3.0f; }
    float normAt (int x) const
    {
        auto a = waveArea();
        const float span = (float) juce::jmax (1, (int) peaks.size() - 1) * 3.0f;
        return juce::jlimit (0.0f, 1.0f, ((float) x - a.getX()) / span);
    }

    // handles: 0 trim start, 1 trim end, 2 loop start, 3 loop end,
    //          kCutHandleBase + n = interior slice cut n,
    //          kFadeInHandle / kFadeOutHandle = the CHOP FADE corners
    static constexpr int kCutHandleBase = 4;
    static constexpr int kCutScale = 1 << 20; // fixed grid SliceGrid maps cuts onto
    static constexpr int kFadeInHandle  = kCutHandleBase + P::SLICE_CUT_COUNT;
    static constexpr int kFadeOutHandle = kCutHandleBase + P::SLICE_CUT_COUNT + 1;

    juce::RangedAudioParameter* paramFor (int handle) const
    {
        switch (handle)
        {
            case 0:  return startParam;
            case 1:  return endParam;
            case 2:  return loopStartParam;
            case 3:  return loopEndParam;
            case kFadeInHandle:
            case kFadeOutHandle: return xfadeParam;
            default: break;
        }
        const int cut = handle - kCutHandleBase;
        return juce::isPositiveAndBelow (cut, P::SLICE_CUT_COUNT) ? cutParams[(size_t) cut] : nullptr;
    }

    /** x of interior cut `cut` (0-based) with its current offset applied. */
    float xForCut (const SliceSettings& sl, int cut) const
    {
        int fa = 0, fb = 0;
        SliceGrid::sliceBounds (sl, 0, kCutScale, cut + 1, fa, fb);
        const float x0 = xForNorm (startNorm()), x1 = xForNorm (endNorm());
        return x0 + (x1 - x0) * (float) fa / (float) kCutScale;
    }

    int cutAt (int x) const
    {
        if (! sliceActive())
            return -1;
        const auto sl = sliceSettings();
        for (int i = 0; i < sl.divisions - 1 && i < P::SLICE_CUT_COUNT; ++i)
            if (std::abs ((float) x - xForCut (sl, i)) < 6.0f)
                return i;
        return -1;
    }

    int handleAt (int x, int y) const
    {
        // The fade corners own the top strip inside the waveform so they never
        // fight the full-height trim lines underneath them.
        const auto a = waveArea();
        if ((float) y < a.getY() + 14.0f)
        {
            const float fw = juce::jmax (fadeWidthPx(), 10.0f);
            if (std::abs ((float) x - (xForNorm (startNorm()) + fw)) < 7.0f) return kFadeInHandle;
            if (std::abs ((float) x - (xForNorm (endNorm()) - fw)) < 7.0f) return kFadeOutHandle;
        }
        if (loopActive())
        {
            if (std::abs ((float) x - xForNorm (loopStartNorm())) < 8.0f) return 2;
            if (std::abs ((float) x - xForNorm (loopEndNorm())) < 8.0f) return 3;
        }
        if (std::abs ((float) x - xForNorm (startNorm())) < 8.0f) return 0;
        if (std::abs ((float) x - xForNorm (endNorm())) < 8.0f) return 1;
        const int cut = cutAt (x);
        return cut >= 0 ? kCutHandleBase + cut : -1;
    }

    void applyDrag (int x)
    {
        if (dragging < 0)
            return;
        if (dragging == kFadeInHandle || dragging == kFadeOutHandle)
        {
            if (xfadeParam == nullptr)
                return;
            // Drag inward to lengthen. A fixed 0.5 ms per pixel keeps the gesture
            // usable on a 6-second sample, where 50 ms is only a few pixels wide.
            const float sign = dragging == kFadeInHandle ? 1.f : -1.f;
            const float ms = juce::jlimit (0.f, 50.f, dragStartMs + sign * ((float) x - dragStartX) * 0.5f);
            xfadeParam->setValueNotifyingHost (xfadeParam->convertTo0to1 (ms));
            repaint();
            return;
        }
        const float n = normAt (x);
        auto* p = paramFor (dragging);
        if (p == nullptr)
            return;
        float value = n;
        switch (dragging)
        {
            case 0: value = juce::jmin (n, endNorm() - 0.01f); break;
            case 1: value = juce::jmax (n, startNorm() + 0.01f); break;
            case 2: value = juce::jlimit (startNorm(), loopEndNorm() - 0.005f, n); break;   // loop stays inside the trim
            case 3: value = juce::jlimit (loopStartNorm() + 0.005f, endNorm(), n); break;
            default:
            {
                // Slice cut: express the drag as an offset in +/- half a slice
                // from the cut's equal-division home position.
                const int cut = dragging - kCutHandleBase;
                const auto sl = sliceSettings();
                const float span = juce::jmax (0.001f, endNorm() - startNorm());
                const float sliceLen = span / (float) juce::jmax (1, sl.divisions);
                const float home = startNorm() + sliceLen * (float) (cut + 1);
                value = juce::jlimit (-1.f, 1.f, (n - home) / (sliceLen * 0.5f));
                break;
            }
        }
        p->setValueNotifyingHost (p->convertTo0to1 (value));
        repaint();
    }

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
    juce::AudioProcessorValueTreeState& apvts;
    juce::RangedAudioParameter* startParam { nullptr };
    juce::RangedAudioParameter* endParam { nullptr };
    juce::RangedAudioParameter* loopStartParam { nullptr };
    juce::RangedAudioParameter* loopEndParam { nullptr };
    juce::RangedAudioParameter* xfadeParam { nullptr };
    float dragStartX { 0.f }, dragStartMs { 0.f };
    std::array<juce::RangedAudioParameter*, 15> cutParams {};
    int hoverCut { -1 };
    const float* cachedData { nullptr };
    int cachedFrames { -1 };
    std::vector<float> peaks;
    int dragging { -1 };
};

// =============================================================================
CargoHoldZone::CargoHoldZone (AviatorKeyzProcessor& p)
    : processorRef (p), apvts (p.getAPVTS())
{
    dropZone = std::make_unique<DropZone> ([this] { browse(); });
    addAndMakeVisible (*dropZone);

    waveBox = std::make_unique<WaveBox> (processorRef, apvts);
    addAndMakeVisible (*waveBox);

    modeSeg = std::make_unique<DeckSegment> (apvts, P::SRC_PLAYBACK_MODE,
        std::vector<DeckSegment::Option> { { "PHRASE", 1 }, { "CHROMATIC", 2 }, { "SLICE", 4 }, { "STRETCH", 3 } });
    addAndMakeVisible (*modeSeg);

    loopChip = std::make_unique<DeckChip> ("LOOP", "ON");
    loopChip->onClick = [this]
    {
        if (auto* lp = apvts.getParameter (P::SRC_LOOP_MODE))
        {
            const int cur = juce::roundToInt (lp->convertFrom0to1 (lp->getValue()));
            const int next = (cur + 1) % 3;
            lp->setValueNotifyingHost (lp->convertTo0to1 ((float) next));

            // Entering LOOP with untouched loop points: pick a sustain loop so a
            // short one-shot holds instead of cutting off. Handles stay draggable.
            if (next == 1
                && apvts.getRawParameterValue (P::SRC_LOOP_START)->load() < 0.001f
                && apvts.getRawParameterValue (P::SRC_LOOP_END)->load() > 0.999f)
                processorRef.autoDetectLoopPoints();
        }
    };
    syncChip = std::make_unique<DeckChip> ("SYNC", "HOST");
    syncChip->onClick = [this]
    {
        if (auto* sp = apvts.getParameter (P::SRC_BPM_SYNC))
            sp->setValueNotifyingHost (sp->getValue() > 0.5f ? 0.0f : 1.0f);
    };
    // ROOT = the key that plays the sample at its recorded pitch. Dragging
    // re-roots the keyboard; a click goes back to the sample's own root.
    rootChip = std::make_unique<DeckChip> ("ROOT", "C4");
    rootChip->onDragTicks = [this] (int ticks) { setRootShift (processorRef.getPresetManager().getRootShift() + ticks); };
    rootChip->onClick = [this] { setRootShift (0); };
    speedChip = std::make_unique<DeckChip> ("SPEED", juce::String::fromUTF8 ("\xc3\x97" "1.00"));
    speedChip->onDragTicks = [this] (int ticks)
    {
        if (auto* sp = apvts.getParameter (P::SRC_SPEED))
        {
            const float cur = sp->convertFrom0to1 (sp->getValue());
            // Locked: whole 0.25 steps (x0.5, x0.75, x1 ...); unlocked: fine 5 % steps.
            const float next = speedLocked() ? snapSpeedRatio (cur + kSpeedSnapStep * (float) ticks)
                                             : juce::jlimit (0.25f, 4.f, cur * std::pow (1.05f, (float) ticks));
            sp->setValueNotifyingHost (sp->convertTo0to1 (next));
        }
    };
    speedChip->onClick = [this]
    {
        if (auto* sp = apvts.getParameter (P::SRC_SPEED))
            sp->setValueNotifyingHost (sp->convertTo0to1 (1.f)); // click = back to 1x
    };
    speedChip->setLock (true, true);
    speedChip->onLockClick = [this]
    {
        auto* lock = apvts.getParameter (P::SRC_SPEED_SNAP);
        auto* sp = apvts.getParameter (P::SRC_SPEED);
        if (lock == nullptr || sp == nullptr)
            return;
        const bool locking = ! speedLocked();
        lock->setValueNotifyingHost (locking ? 1.f : 0.f);
        // land the stored value on the grid too, so the readout matches what plays
        if (locking)
            sp->setValueNotifyingHost (sp->convertTo0to1 (snapSpeedRatio (sp->convertFrom0to1 (sp->getValue()))));
    };
    sliceChip = std::make_unique<DeckChip> ("SLICES", "16");
    sliceChip->onClick = [this]
    {
        if (auto* sp = apvts.getParameter (P::SLICE_DIV))
        {
            const int cur = juce::roundToInt (sp->convertFrom0to1 (sp->getValue()));
            sp->setValueNotifyingHost (
                sp->convertTo0to1 ((float) ((cur + 1) % SliceGrid::kNumDivisionChoices)));
        }
    };
    sliceRndChip = std::make_unique<DeckChip> ("RND", "0%");
    sliceRndChip->onDragTicks = [this] (int ticks)
    {
        if (auto* rp = apvts.getParameter (P::SLICE_RANDOM))
        {
            const float cur = rp->convertFrom0to1 (rp->getValue());
            rp->setValueNotifyingHost (rp->convertTo0to1 (juce::jlimit (0.f, 1.f, cur + 0.05f * (float) ticks)));
        }
    };
    sliceRndChip->onClick = [this]
    {
        if (auto* rp = apvts.getParameter (P::SLICE_RANDOM))
            rp->setValueNotifyingHost (0.f);
    };
    // CHOP FADE: also draggable straight off the waveform corners.
    fadeChip = std::make_unique<DeckChip> ("FADE", "2.0ms");
    fadeChip->onDragTicks = [this] (int ticks)
    {
        if (auto* fp = apvts.getParameter (P::SLICE_XFADE))
        {
            const float cur = fp->convertFrom0to1 (fp->getValue());
            fp->setValueNotifyingHost (fp->convertTo0to1 (juce::jlimit (0.f, 50.f, cur + 0.5f * (float) ticks)));
        }
    };
    fadeChip->onClick = [this]
    {
        if (auto* fp = apvts.getParameter (P::SLICE_XFADE))
            fp->setValueNotifyingHost (fp->convertTo0to1 (2.f));
    };
    trimChip = std::make_unique<DeckChip> ("TRIM", juce::String::fromUTF8 ("0.00s \xe2\x80\x93 0.00s"));
    feedChip = std::make_unique<DeckChip> ("FEEDS ARP + FLIP + ATMOSPHERE", juce::String::fromUTF8 ("\xe2\x96\xb2"), Aviation::textSecondary());
    for (auto* c : { loopChip.get(), syncChip.get(), rootChip.get(), speedChip.get(),
                     sliceChip.get(), sliceRndChip.get(), fadeChip.get(), trimChip.get(), feedChip.get() })
        addAndMakeVisible (*c);

    startTimerHz (20);
    refreshFromProcessor();
}

CargoHoldZone::~CargoHoldZone()
{
    stopTimer();
}

void CargoHoldZone::timerCallback()
{
    waveBox->repaint();
    refreshFromProcessor();

    juce::String hint;
    for (auto* chip : { rootChip.get(), speedChip.get() })
        if (chip->isMouseOverOrDragging())
            hint = chip->getHint();
    if (hint != activeHint)
    {
        activeHint = hint;
        repaint (getLocalBounds().removeFromTop (Deck::kZoneHeaderH));
    }

    if (errorText.isNotEmpty() && juce::Time::getMillisecondCounter() > errorUntilMs)
    {
        errorText.clear();
        dropZone->setMessage ({}, false);
    }
}

void CargoHoldZone::refreshFromProcessor()
{
    static const char* loopNames[] = { "ONE SHOT", "ON", "GATE" };
    const int loop = juce::jlimit (0, 2, (int) apvts.getRawParameterValue (P::SRC_LOOP_MODE)->load());
    loopChip->setValue (loopNames[loop], loop == 1 ? Deck::green() : Aviation::textSecondary());

    const bool sync = apvts.getRawParameterValue (P::SRC_BPM_SYNC)->load() > 0.5f;
    syncChip->setValue (sync ? "HOST" : "OFF", sync ? Deck::green() : Aviation::textSecondary());

    const int mode = (int) apvts.getRawParameterValue (P::SRC_PLAYBACK_MODE)->load();
    const auto dot = juce::String::fromUTF8 (" \xc2\xb7 ");

    // ROOT: green while it steers pitch, gold once re-rooted, dim when the
    // current mode ignores it (SLICE pads, keytrack off).
    {
        const int root = juce::roundToInt (apvts.getRawParameterValue (P::SRC_ROOT_NOTE)->load());
        const int ownRoot = processorRef.getPresetManager().getCurrentRootNote();
        const bool keytrack = apvts.getRawParameterValue (P::SRC_KEYTRACK)->load() > 0.5f;
        const bool rootPlays = keytrack && mode != 4;
        const int shift = root - ownRoot;
        const auto name = Deck::noteName (root);

        rootChip->setValue (name, ! rootPlays ? Aviation::textSecondary()
                                              : (shift != 0 ? Aviation::goldBright() : Deck::green()));

        juce::String hint = "ROOT " + name + dot;
        if (mode == 4)
            hint << "NOT USED IN SLICE" << dot << "KEYS PICK SLICES AT THEIR RECORDED PITCH";
        else if (! keytrack)
            hint << "NOT USED WHILE KEYTRACK IS OFF" << dot << "EVERY KEY PLAYS THE RECORDED PITCH";
        else if (shift == 0)
            hint << "THE KEY THAT PLAYS THE SAMPLE AT ITS RECORDED PITCH" << dot << "DRAG IF YOUR KEYS SOUND OUT OF TUNE";
        else
            hint << "MOVED FROM THE SAMPLE'S OWN " << Deck::noteName (ownRoot) << dot << "KEYS PLAY "
                 << std::abs (shift) << (std::abs (shift) == 1 ? " SEMITONE " : " SEMITONES ")
                 << (shift > 0 ? "LOWER" : "HIGHER") << dot << "CLICK TO RESET";
        rootChip->setHint (hint);
    }

    // SPEED: the lock plays 0.25 steps. STRETCH keeps pitch; other modes resample.
    {
        const float speed = apvts.getRawParameterValue (P::SRC_SPEED)->load();
        const bool locked = speedLocked();
        const bool stretch = mode == 3;
        const auto value = juce::String::fromUTF8 ("\xc3\x97") + juce::String (speed, 2);
        speedChip->setLabel (stretch ? "STRETCH" : "SPEED");
        speedChip->setValue (value, std::abs (speed - 1.f) < 0.005f ? Aviation::textSecondary() : Deck::green());
        speedChip->setLock (true, locked);
        speedChip->setHint ((stretch ? "STRETCH " : "SPEED ") + value + dot
                            + (stretch ? "CHANGES TEMPO, KEEPS PITCH" : "SLOWER PLAYS LOWER (x0.50 = ONE OCTAVE DOWN)")
                            + dot + "DRAG TO CHANGE" + dot + "CLICK = x1" + dot
                            + (locked ? "LOCK ON: 0.25 STEPS" : "LOCK OFF: FREE"));
    }

    const bool sliceMode = mode == 4;
    const int divs = SliceGrid::divisionsForChoice ((int) apvts.getRawParameterValue (P::SLICE_DIV)->load());
    const float sliceRnd = apvts.getRawParameterValue (P::SLICE_RANDOM)->load();
    sliceChip->setValue (juce::String (divs), sliceMode ? Deck::green() : Aviation::textSecondary());
    fadeChip->setValue (juce::String (apvts.getRawParameterValue (P::SLICE_XFADE)->load(), 1) + "ms",
                        apvts.getRawParameterValue (P::SLICE_XFADE)->load() > 0.05f ? Deck::green()
                                                                                    : Aviation::textSecondary());
    sliceRndChip->setValue (juce::String (juce::roundToInt (sliceRnd * 100.f)) + "%",
                            sliceMode && sliceRnd > 0.005f ? Deck::green() : Aviation::textSecondary());

    const double total = waveBox->totalSeconds();
    trimChip->setValue (juce::String (waveBox->startNorm() * total, 2) + juce::String::fromUTF8 ("s \xe2\x80\x93 ")
                        + juce::String (waveBox->endNorm() * total, 2) + "s", Deck::green());
}

void CargoHoldZone::resized()
{
    auto body = getLocalBounds().withTrimmedTop (Deck::kZoneHeaderH).reduced (14, 12);
    dropZone->setBounds (body.removeFromLeft (200));
    body.removeFromLeft (14);

    auto ctrl = body.removeFromBottom (DeckSegment::kSegH);
    body.removeFromBottom (8);
    waveBox->setBounds (body);

    modeSeg->setBounds (ctrl.removeFromLeft (modeSeg->preferredWidth (13)).withHeight (DeckSegment::kSegH));
    ctrl.removeFromLeft (8);
    auto placeChip = [&] (DeckChip& chip)
    {
        chip.setBounds (ctrl.removeFromLeft (chip.preferredWidth() + 6).withHeight (DeckChip::kH).withY (ctrl.getY() + 1));
        ctrl.removeFromLeft (8);
    };
    placeChip (*loopChip);
    placeChip (*syncChip);
    placeChip (*rootChip);
    speedChip->setBounds (ctrl.removeFromLeft (124).withHeight (DeckChip::kH).withY (ctrl.getY() + 1));
    ctrl.removeFromLeft (8);
    placeChip (*sliceChip);
    placeChip (*sliceRndChip);
    placeChip (*fadeChip);
    trimChip->setBounds (ctrl.removeFromLeft (150).withHeight (DeckChip::kH).withY (ctrl.getY() + 1));
    feedChip->setBounds (ctrl.removeFromRight (feedChip->preferredWidth() + 8).withHeight (DeckChip::kH).withY (ctrl.getY() + 1));
}

void CargoHoldZone::paint (juce::Graphics& g)
{
    Deck::paintZone (g, getLocalBounds(), "CARGO HOLD",
                     activeHint.isNotEmpty() ? juce::String()
                                             : juce::String::fromUTF8 ("USER SAMPLE \xc2\xb7 WAV / AIFF / MP3 / FLAC \xc2\xb7 MAX 60s \xc2\xb7 AUTO BPM + KEY DETECT"));
    if (activeHint.isNotEmpty())
    {
        // plain-English help for the hovered control, where the tag usually sits
        g.setFont (Deck::mono (8.5f));
        g.setColour (Aviation::cyan());
        g.drawText (activeHint, getLocalBounds().removeFromTop (Deck::kZoneHeaderH).reduced (14, 0),
                    juce::Justification::centredRight);
    }
    if (dragOver)
    {
        g.setColour (Aviation::gold().withAlpha (0.6f));
        g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (1.5f), 8.0f, 2.0f);
    }
}

bool CargoHoldZone::isInterestedInFileDrag (const juce::StringArray& files)
{
    for (const auto& f : files)
        if (isAcceptedAudioFile (f))
            return true;
    return false;
}

void CargoHoldZone::fileDragEnter (const juce::StringArray&, int, int)
{
    dragOver = true;
    dropZone->setHighlighted (true);
    repaint();
}

void CargoHoldZone::fileDragExit (const juce::StringArray&)
{
    dragOver = false;
    dropZone->setHighlighted (false);
    repaint();
}

void CargoHoldZone::filesDropped (const juce::StringArray& files, int, int)
{
    dragOver = false;
    dropZone->setHighlighted (false);
    repaint();
    for (const auto& f : files)
    {
        if (isAcceptedAudioFile (f))
        {
            loadFile (juce::File (f));
            return;
        }
    }
    showError ("UNSUPPORTED AUDIO FORMAT");
}

void CargoHoldZone::loadFile (const juce::File& file)
{
    juce::String error;
    if (! processorRef.loadUserSample (file, error))
    {
        showError (error.toUpperCase());
        return;
    }
    errorText.clear();
    dropZone->setMessage ("LOADED\n" + file.getFileName().toUpperCase(), false);
    errorUntilMs = juce::Time::getMillisecondCounter() + 4000;
    errorText = "loaded"; // reuse the timeout to clear the message
    refreshFromProcessor();
}

void CargoHoldZone::setRootShift (int semitones)
{
    auto& pm = processorRef.getPresetManager();
    pm.setRootShift (semitones);
    if (auto* rp = apvts.getParameter (P::SRC_ROOT_NOTE))
        rp->setValueNotifyingHost (rp->convertTo0to1 ((float) pm.getEffectiveRootNote()));
    refreshFromProcessor();
}

bool CargoHoldZone::speedLocked() const
{
    return apvts.getRawParameterValue (P::SRC_SPEED_SNAP)->load() > 0.5f;
}

void CargoHoldZone::showError (const juce::String& message)
{
    errorText = message;
    errorUntilMs = juce::Time::getMillisecondCounter() + 5000;
    dropZone->setMessage (message, true);
}

void CargoHoldZone::browse()
{
    chooser = std::make_unique<juce::FileChooser> ("Load cargo (WAV / AIFF / MP3 / FLAC, max 60 s)",
                                                   juce::File::getSpecialLocation (juce::File::userMusicDirectory),
                                                   "*.wav;*.aif;*.aiff;*.flac;*.ogg;*.mp3;*.m4a;*.aac");
    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                          [safe = juce::Component::SafePointer<CargoHoldZone> (this)] (const juce::FileChooser& fc)
                          {
                              if (safe == nullptr)
                                  return;
                              const auto result = fc.getResult();
                              if (result.existsAsFile())
                                  safe->loadFile (result);
                          });
}
