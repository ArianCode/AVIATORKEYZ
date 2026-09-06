#include "CenterDashboard.h"
#include "AviationIcons.h"
#include "AviationTheme.h"
#include "../../State/StateSchema.h"
#include "../../DSP/Mfx/MfxDescriptors.h"

namespace
{
constexpr int kMiniStripH  = 24;
constexpr int kTitleH      = 30;
constexpr int kDisplayRowH = 112;
constexpr int kStripGap    = 8;
constexpr int kStripH      = 58;

juce::String tuneHzText (juce::AudioProcessorValueTreeState& apvts)
{
    double semis = 0.0;
    if (auto* p = apvts.getRawParameterValue (AviatorKeyz::ParamID::SRC_TUNE))
        semis = (double) p->load();
    const double hz = 440.0 * std::pow (2.0, semis / 12.0);
    return juce::String (hz, 2) + " Hz";
}
} // namespace

// =============================================================================
//  LimiterCell — output limiter toggle presented as a small output dial.
//  Shows the limiter ceiling (-0.5 dB, see OutputLimiter::prepare) when
//  engaged, OFF otherwise. Click toggles output_limiter.
// =============================================================================
class CenterDashboard::LimiterCell : public juce::Component
{
public:
    explicit LimiterCell (juce::AudioProcessorValueTreeState& apvts)
        : apvtsRef (apvts)
    {
        button.setWantsKeyboardFocus (false);
        button.setAlpha (0.0f);
        addAndMakeVisible (button);
        attachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
            apvtsRef, AviatorKeyz::ParamID::OUTPUT_LIMITER, button);
        button.onStateChange = [this] { repaint(); };
        button.onClick = [this] { repaint(); };
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
    }

    void resized() override { button.setBounds (getLocalBounds()); }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        const bool on = button.getToggleState();

        g.setFont (Aviation::label (9.5f, 0.14f));
        g.setColour (Aviation::gold().withAlpha (0.92f));
        g.drawText ("LIMITER", r.toNearestInt().removeFromTop (14), juce::Justification::centred);

        // output dial
        const float cx = r.getCentreX();
        const float cy = r.getCentreY() + 6.0f;
        const float radius = 24.0f;
        constexpr float a0 = juce::MathConstants<float>::pi * 1.25f;
        constexpr float a1 = juce::MathConstants<float>::pi * 2.75f;

        juce::Path track;
        track.addCentredArc (cx, cy, radius, radius, 0.0f, a0, a1, true);
        g.setColour (juce::Colour (0xff17262f));
        g.strokePath (track, juce::PathStrokeType (2.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        if (on)
        {
            juce::Path arc;
            arc.addCentredArc (cx, cy, radius, radius, 0.0f, a0, a1 - 0.12f, true);
            g.setColour (Aviation::cyan().withAlpha (0.9f));
            g.strokePath (arc, juce::PathStrokeType (2.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        g.setFont (Aviation::value (12.5f));
        g.setColour (on ? juce::Colours::white : Aviation::textSecondary());
        g.drawText (on ? juce::String ("-0.5 dB") : juce::String ("OFF"),
                    (int) (cx - 40.0f), (int) (cy - 8.0f), 80, 16, juce::Justification::centred);

        g.setFont (Aviation::label (8.5f, 0.12f));
        g.setColour (Aviation::textSecondary());
        g.drawText ("OUTPUT", r.toNearestInt().removeFromBottom (12), juce::Justification::centred);
    }

private:
    juce::AudioProcessorValueTreeState& apvtsRef;
    juce::ToggleButton button;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> attachment;
};

// =============================================================================
//  ReverseCell — glass cell that mirrors the Flight Deck flip lever on the
//  MAIN page: FWD / REV readout with a direction arrow. Click toggles `reverse`.
// =============================================================================
class CenterDashboard::ReverseCell : public juce::Component
{
public:
    explicit ReverseCell (juce::AudioProcessorValueTreeState& apvts)
        : apvtsRef (apvts)
    {
        button.setWantsKeyboardFocus (false);
        button.setAlpha (0.0f);
        addAndMakeVisible (button);
        attachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
            apvtsRef, AviatorKeyz::ParamID::REVERSE, button);
        button.onStateChange = [this] { repaint(); };
        button.onClick = [this] { repaint(); };
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
    }

    void resized() override { button.setBounds (getLocalBounds()); }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        const bool rev = button.getToggleState();
        const bool hover = button.isOver() || button.isDown();

        Aviation::fillGlassScreen (g, r, 5.0f, hover ? 0.5f : 0.28f);

        // state bar along the bottom edge: cyan forward, warm red reversed
        const auto barColour = rev ? juce::Colour (0xffff6a4d) : Aviation::cyan();
        g.setColour (barColour.withAlpha (0.65f));
        g.fillRect (juce::Rectangle<float> (r.getX() + 4.0f, r.getBottom() - 4.0f, r.getWidth() - 8.0f, 2.0f));

        g.setFont (Aviation::label (9.5f, 0.12f));
        g.setColour (Aviation::gold().withAlpha (0.92f));
        g.drawText ("REVERSE", 0, 7, getWidth(), 12, juce::Justification::centred);

        g.setFont (Aviation::value (13.0f));
        g.setColour (rev ? juce::Colour (0xffffb3a2) : Aviation::cyanBright());
        g.drawText (rev ? juce::String::fromUTF8 ("\xe2\x97\x80\xe2\x97\x80 REV")
                        : juce::String::fromUTF8 ("FWD \xe2\x96\xb6\xe2\x96\xb6"),
                    0, 21, getWidth(), getHeight() - 26, juce::Justification::centred);
    }

private:
    juce::AudioProcessorValueTreeState& apvtsRef;
    juce::ToggleButton button;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> attachment;
};

// =============================================================================
CenterDashboard::CenterDashboard (juce::AudioProcessorValueTreeState& apvts)
    : apvtsRef (apvts)
{
    namespace P = AviatorKeyz::ParamID;

    globalsKnob = std::make_unique<MiniRotary> (apvtsRef, P::OUTPUT_GAIN, "OUT GAIN");
    addAndMakeVisible (*globalsKnob);

    limiterCell = std::make_unique<LimiterCell> (apvtsRef);
    addAndMakeVisible (*limiterCell);

    reverseCell = std::make_unique<ReverseCell> (apvtsRef);
    addAndMakeVisible (*reverseCell);

    stereoCell   = std::make_unique<MiniParam> (apvtsRef, P::TEX_WIDTH, "STEREO");
    dynamicsCell = std::make_unique<MiniParam> (apvtsRef, P::VELOCITY_SENSITIVITY, "DYNAMICS", true);
    widthCell    = std::make_unique<MiniParam> (apvtsRef, Mfx::sendId (0), "REV SEND");
    humanizeCell = std::make_unique<MiniParam> (apvtsRef, P::TEX_DRIFT, "HUMANIZE");
    for (auto* cell : { stereoCell.get(), dynamicsCell.get(), widthCell.get(), humanizeCell.get() })
        addAndMakeVisible (*cell);

    tuneSlider.setAlpha (0.0f);
    tuneSlider.setWantsKeyboardFocus (false);
    addAndMakeVisible (tuneSlider);
    AviationMini::configureAttachment (apvtsRef, P::SRC_TUNE, tuneSlider, tuneAttachment);
    tuneSlider.onValueChange = [this] { repaint (miniDisplayArea()); };

    if (auto* pan = apvtsRef.getParameter (P::PAN))
    {
        panAttachment = std::make_unique<juce::ParameterAttachment> (*pan, [this] (float v)
        {
            panValue = juce::jlimit (-1.f, 1.f, v);
            repaint (blueprintArea());
        });
        panAttachment->sendInitialUpdate();
    }

    startTimerHz (30);
}

CenterDashboard::~CenterDashboard() = default;

// -----------------------------------------------------------------------------
//  Blueprint = pan control. The outline aircraft flies left -> right on a loop;
//  the filled aircraft sits where the stereo pan is and can be dragged.
// -----------------------------------------------------------------------------
void CenterDashboard::applyPanDrag (int x)
{
    if (panAttachment == nullptr)
        return;
    const auto inner = blueprintArea().toFloat().reduced (6.0f);
    const float lane = inner.getWidth() - inner.getHeight() * 1.15f;
    const float norm = juce::jlimit (0.f, 1.f, ((float) x - inner.getX() - inner.getHeight() * 0.575f) / juce::jmax (1.f, lane));
    panAttachment->setValueAsPartOfGesture (norm * 2.f - 1.f);
}

void CenterDashboard::mouseDown (const juce::MouseEvent& e)
{
    if (! blueprintArea().contains (e.getPosition()) || panAttachment == nullptr)
        return;
    if (e.getNumberOfClicks() >= 2)
    {
        panAttachment->setValueAsCompleteGesture (0.f);
        return;
    }
    panDragging = true;
    panAttachment->beginGesture();
    applyPanDrag (e.x);
}

void CenterDashboard::mouseDrag (const juce::MouseEvent& e)
{
    if (panDragging)
        applyPanDrag (e.x);
}

void CenterDashboard::mouseUp (const juce::MouseEvent&)
{
    if (panDragging && panAttachment != nullptr)
        panAttachment->endGesture();
    panDragging = false;
}

void CenterDashboard::timerCallback()
{
    // Radar sweep speed follows LFO1 rate so the instruments track real state.
    const float rateNorm = AviationMini::parameterNorm (apvtsRef, AviatorKeyz::ParamID::LFO1_RATE);
    sweepPhase += (0.010f + rateNorm * 0.05f);
    if (sweepPhase > 1.0f)
        sweepPhase -= 1.0f;

    flyPhase += 1.0f / (30.0f * 6.0f); // one crossing every 6 s
    if (flyPhase > 1.15f)
        flyPhase = -0.15f;

    repaint (radarLeftArea());
    repaint (radarRightArea());
    repaint (blueprintArea());
}

void CenterDashboard::setKeyText (const juce::String& text)
{
    if (keyText != text)
    {
        keyText = text;
        repaint (miniDisplayArea());
    }
}

void CenterDashboard::setTitleText (const juce::String& text)
{
    if (titleText != text)
    {
        titleText = text;
        repaint();
    }
}

juce::Rectangle<int> CenterDashboard::miniDisplayArea() const
{
    return { getWidth() / 2 - 215, 0, 430, kMiniStripH };
}

juce::Rectangle<int> CenterDashboard::radarLeftArea() const
{
    return { 90, kMiniStripH + kTitleH + 4, 105, kDisplayRowH - 8 };
}

juce::Rectangle<int> CenterDashboard::radarRightArea() const
{
    return { getWidth() - 195, kMiniStripH + kTitleH + 4, 105, kDisplayRowH - 8 };
}

juce::Rectangle<int> CenterDashboard::blueprintArea() const
{
    return { 200, kMiniStripH + kTitleH + 4, getWidth() - 400, kDisplayRowH - 8 };
}

void CenterDashboard::resized()
{
    const int rowY = kMiniStripH + kTitleH;

    globalsKnob->setBounds (6, rowY + 16, 80, kDisplayRowH - 24);
    limiterCell->setBounds (getWidth() - 86, rowY + 6, 80, kDisplayRowH - 12);

    // TUNE hit zone in the mini display strip (right cell)
    const auto strip = miniDisplayArea();
    tuneSlider.setBounds (strip.getRight() - 120, strip.getY(), 120, strip.getHeight());

    // lower parameter strip
    const int stripY = rowY + kDisplayRowH + kStripGap;
    const int w = getWidth();
    reverseCell->setBounds  (0,            stripY, 100, kStripH);
    stereoCell->setBounds   (106,          stripY, 100, kStripH);
    dynamicsCell->setBounds (212,          stripY, w - 424, kStripH);
    widthCell->setBounds    (w - 206,      stripY, 100, kStripH);
    humanizeCell->setBounds (w - 100,      stripY, 100, kStripH);
}

void CenterDashboard::paintMiniDisplay (juce::Graphics& g)
{
    auto strip = miniDisplayArea().toFloat();
    Aviation::fillGlassScreen (g, strip, 4.0f, 0.22f);

    struct Cell { const char* label; juce::String value; };
    const Cell cells[] = {
        { "RPM",  bpmProvider ? juce::String (juce::roundToInt (bpmProvider())) : juce::String ("-") },
        { "KEY",  keyText },
        { "TUNE", tuneHzText (apvtsRef) },
    };

    const float cellW = strip.getWidth() / 3.0f;
    for (int i = 0; i < 3; ++i)
    {
        juce::Rectangle<float> cell (strip.getX() + cellW * (float) i, strip.getY(), cellW, strip.getHeight());
        g.setFont (Aviation::label (8.5f, 0.14f));
        g.setColour (Aviation::gold().withAlpha (0.8f));
        g.drawText (cells[i].label, cell.toNearestInt().removeFromLeft ((int) (cellW * 0.42f)),
                    juce::Justification::centredRight);
        g.setFont (Aviation::value (11.0f));
        g.setColour (Aviation::cyanBright());
        g.drawText (" " + cells[i].value, cell.toNearestInt().removeFromRight ((int) (cellW * 0.55f)),
                    juce::Justification::centredLeft);

        if (i > 0)
        {
            g.setColour (juce::Colour (0x33406080));
            g.fillRect (juce::Rectangle<float> (cell.getX(), cell.getY() + 5.0f, 1.0f, cell.getHeight() - 10.0f));
        }
    }
}

void CenterDashboard::paintRadar (juce::Graphics& g, juce::Rectangle<float> area, float phase, bool clockwise)
{
    Aviation::fillGlassScreen (g, area, 6.0f, 0.30f);

    auto scope = area.reduced (8.0f);
    const float cx = scope.getCentreX();
    const float cy = scope.getCentreY();
    const float maxR = juce::jmin (scope.getWidth(), scope.getHeight()) * 0.5f;

    // concentric rings + cross grid
    g.setColour (Aviation::cyan().withAlpha (0.30f));
    for (float f : { 1.0f, 0.66f, 0.33f })
        g.drawEllipse (cx - maxR * f, cy - maxR * f, maxR * f * 2.0f, maxR * f * 2.0f, 0.8f);
    g.drawLine (cx - maxR, cy, cx + maxR, cy, 0.6f);
    g.drawLine (cx, cy - maxR, cx, cy + maxR, 0.6f);

    // rotating sweep with fading trail
    const float a = juce::MathConstants<float>::twoPi * (clockwise ? phase : 1.0f - phase);
    for (int t = 0; t < 10; ++t)
    {
        const float trailA = a - (clockwise ? 1.0f : -1.0f) * 0.05f * (float) t;
        g.setColour (Aviation::cyan().withAlpha (0.30f * (1.0f - (float) t / 10.0f)));
        g.drawLine (cx, cy, cx + maxR * std::sin (trailA), cy - maxR * std::cos (trailA), t == 0 ? 1.4f : 1.0f);
    }

    g.setColour (Aviation::cyanBright().withAlpha (0.9f));
    g.fillEllipse (cx - 1.5f, cy - 1.5f, 3.0f, 3.0f);
}

void CenterDashboard::paintBlueprint (juce::Graphics& g, juce::Rectangle<float> area)
{
    Aviation::fillGlassScreen (g, area, 6.0f, 0.32f);

    auto inner = area.reduced (6.0f);

    // thin cyan grid
    g.setColour (Aviation::cyan().withAlpha (0.14f));
    for (float x = inner.getX(); x <= inner.getRight(); x += 14.0f)
        g.drawLine (x, inner.getY(), x, inner.getBottom(), 0.5f);
    for (float y = inner.getY(); y <= inner.getBottom(); y += 14.0f)
        g.drawLine (inner.getX(), y, inner.getRight(), y, 0.5f);

    // Pan lane: outline aircraft cruises left -> right on a loop (heading is
    // rotated 90°: nose to the right); the filled aircraft marks the pan position.
    const float planeW = inner.getHeight() * 1.15f;
    const float planeH = inner.getHeight() * 0.92f;
    const float lane = inner.getWidth() - planeW;
    const auto plane = AviationIcons::aircraftTop();
    auto planeAt = [&] (float norm)
    {
        return juce::Rectangle<float> (inner.getX() + lane * norm, inner.getCentreY() - planeH * 0.5f, planeW, planeH);
    };
    auto drawPlane = [&] (juce::Rectangle<float> area, bool filled, float alpha)
    {
        juce::Path p (plane);
        // AviationIcons::aircraftTop points up; rotate to fly right
        const auto b = p.getBounds();
        p.applyTransform (juce::AffineTransform::rotation (juce::MathConstants<float>::halfPi, b.getCentreX(), b.getCentreY()));
        const auto rb = p.getBounds();
        p.applyTransform (juce::AffineTransform::translation (-rb.getX(), -rb.getY())
                              .scaled (area.getWidth() / juce::jmax (1.0f, rb.getWidth()), area.getHeight() / juce::jmax (1.0f, rb.getHeight()))
                              .translated (area.getX(), area.getY()));
        if (filled)
        {
            g.setColour (Aviation::goldBright().withAlpha (0.9f * alpha));
            g.fillPath (p);
        }
        g.setColour (Aviation::gold().withAlpha ((filled ? 0.35f : 0.75f) * alpha));
        g.strokePath (p, juce::PathStrokeType (filled ? 0.8f : 1.0f));
    };

    // centre / L / R ticks
    g.setColour (Aviation::cyan().withAlpha (0.35f));
    const float midX = inner.getX() + lane * 0.5f + planeW * 0.5f;
    g.drawLine (midX, inner.getBottom() - 6.0f, midX, inner.getBottom(), 1.0f);
    g.setFont (Aviation::label (7.0f, 0.12f));
    g.drawText ("L", inner.toNearestInt().removeFromLeft (14).removeFromBottom (10), juce::Justification::centred);
    g.drawText ("R", inner.toNearestInt().removeFromRight (14).removeFromBottom (10), juce::Justification::centred);

    drawPlane (planeAt (juce::jlimit (-0.2f, 1.2f, flyPhase)), false, 0.7f);
    drawPlane (planeAt ((panValue + 1.f) * 0.5f), true, 1.0f);

    g.setFont (Aviation::value (8.5f));
    g.setColour (Aviation::cyanBright().withAlpha (0.85f));
    const juce::String panText = std::abs (panValue) < 0.01f ? juce::String ("PAN C")
                                 : (panValue < 0 ? "PAN L" : "PAN R") + juce::String (juce::roundToInt (std::abs (panValue) * 100.f));
    g.drawText (panText, inner.toNearestInt().removeFromTop (12).withTrimmedLeft (8), juce::Justification::centredLeft);

    // corner ticks
    g.setColour (Aviation::cyan().withAlpha (0.5f));
    const float tick = 7.0f;
    g.drawLine (inner.getX(), inner.getY(), inner.getX() + tick, inner.getY(), 1.0f);
    g.drawLine (inner.getX(), inner.getY(), inner.getX(), inner.getY() + tick, 1.0f);
    g.drawLine (inner.getRight(), inner.getBottom(), inner.getRight() - tick, inner.getBottom(), 1.0f);
    g.drawLine (inner.getRight(), inner.getBottom(), inner.getRight(), inner.getBottom() - tick, 1.0f);
}

void CenterDashboard::paint (juce::Graphics& g)
{
    // Console bezel — recessed dark module with warm trim, physically part
    // of the cockpit dashboard.
    auto r = getLocalBounds().toFloat();
    auto bezel = r.withTrimmedTop ((float) kMiniStripH + 2.0f);
    juce::ColourGradient grad (juce::Colour (0xd80a141d), bezel.getX(), bezel.getY(),
                               juce::Colour (0xe6050b12), bezel.getX(), bezel.getBottom(), false);
    g.setGradientFill (grad);
    g.fillRoundedRectangle (bezel, 8.0f);
    g.setColour (Aviation::goldDeep().withAlpha (0.30f));
    g.drawRoundedRectangle (bezel, 8.0f, 1.0f);
    g.setColour (juce::Colours::white.withAlpha (0.05f));
    g.fillRect (juce::Rectangle<float> (bezel.getX() + 3.0f, bezel.getY() + 1.0f, bezel.getWidth() - 6.0f, 1.0f));

    paintMiniDisplay (g);

    // gold preset title
    g.setFont (Aviation::value (17.0f));
    g.setColour (Aviation::goldBright());
    g.drawText (titleText, 0, kMiniStripH + 4, getWidth(), kTitleH - 6, juce::Justification::centred);

    // GLOBALS section label above the output knob
    g.setFont (Aviation::label (9.5f, 0.14f));
    g.setColour (Aviation::gold().withAlpha (0.92f));
    g.drawText ("GLOBALS", 6, kMiniStripH + kTitleH + 4, 80, 12, juce::Justification::centred);

    paintRadar (g, radarLeftArea().toFloat(), sweepPhase, true);
    paintRadar (g, radarRightArea().toFloat(), sweepPhase, false);
    paintBlueprint (g, blueprintArea().toFloat());
}
