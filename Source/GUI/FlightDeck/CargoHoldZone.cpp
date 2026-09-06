#include "CargoHoldZone.h"
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

bool isAcceptedAudioFile (const juce::String& path)
{
    const auto ext = juce::File (path).getFileExtension().toLowerCase();
    return ext == ".wav" || ext == ".aif" || ext == ".aiff";
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
    }

    void mouseMove (const juce::MouseEvent& e) override
    {
        const int h = handleAt (e.x);
        setMouseCursor (h >= 0 ? juce::MouseCursor::LeftRightResizeCursor : juce::MouseCursor::NormalCursor);
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        dragging = handleAt (e.x);
        if (dragging < 0)
        {
            // click sets the nearer handle
            const float n = normAt (e.x);
            dragging = std::abs (n - startNorm()) < std::abs (n - endNorm()) ? 0 : 1;
        }
        if (auto* p = dragging == 0 ? startParam : endParam)
            p->beginChangeGesture();
        applyDrag (e.x);
    }

    void mouseDrag (const juce::MouseEvent& e) override { applyDrag (e.x); }

    void mouseUp (const juce::MouseEvent&) override
    {
        if (dragging >= 0)
            if (auto* p = dragging == 0 ? startParam : endParam)
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

        // SLICE mode: 16 pads across the trimmed window (C1 = pad 1)
        if ((int) apvts.getRawParameterValue (P::SRC_PLAYBACK_MODE)->load() == 4)
        {
            const float x0 = xForNorm (s), x1 = xForNorm (e);
            g.setFont (Deck::mono (7.0f));
            for (int i = 0; i < 16; ++i)
            {
                const float xa = x0 + (x1 - x0) * (float) i / 16.0f;
                g.setColour (Aviation::gold().withAlpha (i == 0 ? 0.0f : 0.35f));
                g.drawLine (xa, area.getY(), xa, area.getBottom(), 0.8f);
                g.setColour (Aviation::gold().withAlpha (0.55f));
                g.drawText (juce::String (i + 1), (int) xa + 2, (int) area.getBottom() - 10, 18, 9, juce::Justification::centredLeft);
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

private:
    juce::Rectangle<float> waveArea() const { return getLocalBounds().toFloat().reduced (2.0f).withTrimmedTop (30.0f).withTrimmedBottom (4.0f); }
    float xForNorm (float n) const { auto a = waveArea(); return a.getX() + juce::jlimit (0.0f, 1.0f, n) * (float) juce::jmax (1, (int) peaks.size() - 1) * 3.0f; }
    float normAt (int x) const
    {
        auto a = waveArea();
        const float span = (float) juce::jmax (1, (int) peaks.size() - 1) * 3.0f;
        return juce::jlimit (0.0f, 1.0f, ((float) x - a.getX()) / span);
    }

    int handleAt (int x) const
    {
        if (std::abs ((float) x - xForNorm (startNorm())) < 8.0f) return 0;
        if (std::abs ((float) x - xForNorm (endNorm())) < 8.0f) return 1;
        return -1;
    }

    void applyDrag (int x)
    {
        if (dragging < 0)
            return;
        const float n = normAt (x);
        if (dragging == 0 && startParam != nullptr)
            startParam->setValueNotifyingHost (startParam->convertTo0to1 (juce::jmin (n, endNorm() - 0.01f)));
        else if (dragging == 1 && endParam != nullptr)
            endParam->setValueNotifyingHost (endParam->convertTo0to1 (juce::jmax (n, startNorm() + 0.01f)));
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
            lp->setValueNotifyingHost (lp->convertTo0to1 ((float) ((cur + 1) % 3)));
        }
    };
    syncChip = std::make_unique<DeckChip> ("SYNC", "HOST");
    syncChip->onClick = [this]
    {
        if (auto* sp = apvts.getParameter (P::SRC_BPM_SYNC))
            sp->setValueNotifyingHost (sp->getValue() > 0.5f ? 0.0f : 1.0f);
    };
    rootChip = std::make_unique<DeckChip> ("ROOT", "C4");
    rootChip->onDragTicks = [this] (int ticks)
    {
        if (auto* rp = apvts.getParameter (P::SRC_ROOT_NOTE))
        {
            const int cur = juce::roundToInt (rp->convertFrom0to1 (rp->getValue()));
            rp->setValueNotifyingHost (rp->convertTo0to1 ((float) juce::jlimit (0, 127, cur + ticks)));
        }
    };
    speedChip = std::make_unique<DeckChip> ("SPEED", juce::String::fromUTF8 ("\xc3\x97" "1.00"));
    speedChip->onDragTicks = [this] (int ticks)
    {
        if (auto* sp = apvts.getParameter (P::SRC_SPEED))
        {
            const float cur = sp->convertFrom0to1 (sp->getValue());
            const float next = juce::jlimit (0.25f, 4.f, cur * std::pow (1.05f, (float) ticks));
            sp->setValueNotifyingHost (sp->convertTo0to1 (next));
        }
    };
    speedChip->onClick = [this]
    {
        if (auto* sp = apvts.getParameter (P::SRC_SPEED))
            sp->setValueNotifyingHost (sp->convertTo0to1 (1.f)); // click = back to 1x
    };
    trimChip = std::make_unique<DeckChip> ("TRIM", juce::String::fromUTF8 ("0.00s \xe2\x80\x93 0.00s"));
    feedChip = std::make_unique<DeckChip> ("FEEDS ARP + FLIP + ATMOSPHERE", juce::String::fromUTF8 ("\xe2\x96\xb2"), Aviation::textSecondary());
    for (auto* c : { loopChip.get(), syncChip.get(), rootChip.get(), speedChip.get(), trimChip.get(), feedChip.get() })
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

    rootChip->setValue (Deck::noteName ((int) apvts.getRawParameterValue (P::SRC_ROOT_NOTE)->load()), Deck::green());

    const int mode = (int) apvts.getRawParameterValue (P::SRC_PLAYBACK_MODE)->load();
    const float speed = apvts.getRawParameterValue (P::SRC_SPEED)->load();
    speedChip->setLabel (mode == 3 ? "STRETCH" : "SPEED");
    speedChip->setValue (juce::String::fromUTF8 ("\xc3\x97") + juce::String (speed, 2),
                         std::abs (speed - 1.f) < 0.005f ? Aviation::textSecondary() : Deck::green());

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
    speedChip->setBounds (ctrl.removeFromLeft (108).withHeight (DeckChip::kH).withY (ctrl.getY() + 1));
    ctrl.removeFromLeft (8);
    trimChip->setBounds (ctrl.removeFromLeft (150).withHeight (DeckChip::kH).withY (ctrl.getY() + 1));
    feedChip->setBounds (ctrl.removeFromRight (feedChip->preferredWidth() + 8).withHeight (DeckChip::kH).withY (ctrl.getY() + 1));
}

void CargoHoldZone::paint (juce::Graphics& g)
{
    Deck::paintZone (g, getLocalBounds(), "CARGO HOLD",
                     juce::String::fromUTF8 ("USER SAMPLE \xc2\xb7 WAV / AIFF \xc2\xb7 MAX 60s \xc2\xb7 AUTO BPM + KEY DETECT"));
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
    showError ("ONLY WAV / AIFF ACCEPTED");
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

void CargoHoldZone::showError (const juce::String& message)
{
    errorText = message;
    errorUntilMs = juce::Time::getMillisecondCounter() + 5000;
    dropZone->setMessage (message, true);
}

void CargoHoldZone::browse()
{
    chooser = std::make_unique<juce::FileChooser> ("Load cargo (WAV / AIFF, max 60 s)",
                                                   juce::File::getSpecialLocation (juce::File::userMusicDirectory),
                                                   "*.wav;*.aif;*.aiff");
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
