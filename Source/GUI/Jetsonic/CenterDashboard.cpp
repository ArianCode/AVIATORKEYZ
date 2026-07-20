#include "CenterDashboard.h"
#include "JetsonicIcons.h"
#include "JetsonicTheme.h"
#include "../../State/StateSchema.h"

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

        g.setFont (Jetsonic::label (9.5f, 0.14f));
        g.setColour (Jetsonic::gold().withAlpha (0.92f));
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
            g.setColour (Jetsonic::cyan().withAlpha (0.9f));
            g.strokePath (arc, juce::PathStrokeType (2.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        g.setFont (Jetsonic::value (12.5f));
        g.setColour (on ? juce::Colours::white : Jetsonic::textSecondary());
        g.drawText (on ? juce::String ("-0.5 dB") : juce::String ("OFF"),
                    (int) (cx - 40.0f), (int) (cy - 8.0f), 80, 16, juce::Justification::centred);

        g.setFont (Jetsonic::label (8.5f, 0.12f));
        g.setColour (Jetsonic::textSecondary());
        g.drawText ("OUTPUT", r.toNearestInt().removeFromBottom (12), juce::Justification::centred);
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

    lofiCell     = std::make_unique<MiniParam> (apvtsRef, P::FX_LOFI_AMOUNT, "LOFI");
    stereoCell   = std::make_unique<MiniParam> (apvtsRef, P::TEX_WIDTH, "STEREO");
    dynamicsCell = std::make_unique<MiniParam> (apvtsRef, P::VELOCITY_SENSITIVITY, "DYNAMICS", true);
    widthCell    = std::make_unique<MiniParam> (apvtsRef, P::PTEX_WIDTH, "WIDTH");
    humanizeCell = std::make_unique<MiniParam> (apvtsRef, P::TEX_DRIFT, "HUMANIZE");
    for (auto* cell : { lofiCell.get(), stereoCell.get(), dynamicsCell.get(), widthCell.get(), humanizeCell.get() })
        addAndMakeVisible (*cell);

    tuneSlider.setAlpha (0.0f);
    tuneSlider.setWantsKeyboardFocus (false);
    addAndMakeVisible (tuneSlider);
    JetsonicMini::configureAttachment (apvtsRef, P::SRC_TUNE, tuneSlider, tuneAttachment);
    tuneSlider.onValueChange = [this] { repaint (miniDisplayArea()); };

    startTimerHz (15);
}

CenterDashboard::~CenterDashboard() = default;

void CenterDashboard::timerCallback()
{
    // Radar sweep speed follows LFO1 rate so the instruments track real state.
    const float rateNorm = JetsonicMini::parameterNorm (apvtsRef, AviatorKeyz::ParamID::LFO1_RATE);
    sweepPhase += (0.010f + rateNorm * 0.05f);
    if (sweepPhase > 1.0f)
        sweepPhase -= 1.0f;

    repaint (radarLeftArea());
    repaint (radarRightArea());
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
    lofiCell->setBounds     (0,            stripY, 100, kStripH);
    stereoCell->setBounds   (106,          stripY, 100, kStripH);
    dynamicsCell->setBounds (212,          stripY, w - 424, kStripH);
    widthCell->setBounds    (w - 206,      stripY, 100, kStripH);
    humanizeCell->setBounds (w - 100,      stripY, 100, kStripH);
}

void CenterDashboard::paintMiniDisplay (juce::Graphics& g)
{
    auto strip = miniDisplayArea().toFloat();
    Jetsonic::fillGlassScreen (g, strip, 4.0f, 0.22f);

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
        g.setFont (Jetsonic::label (8.5f, 0.14f));
        g.setColour (Jetsonic::gold().withAlpha (0.8f));
        g.drawText (cells[i].label, cell.toNearestInt().removeFromLeft ((int) (cellW * 0.42f)),
                    juce::Justification::centredRight);
        g.setFont (Jetsonic::value (11.0f));
        g.setColour (Jetsonic::cyanBright());
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
    Jetsonic::fillGlassScreen (g, area, 6.0f, 0.30f);

    auto scope = area.reduced (8.0f);
    const float cx = scope.getCentreX();
    const float cy = scope.getCentreY();
    const float maxR = juce::jmin (scope.getWidth(), scope.getHeight()) * 0.5f;

    // concentric rings + cross grid
    g.setColour (Jetsonic::cyan().withAlpha (0.30f));
    for (float f : { 1.0f, 0.66f, 0.33f })
        g.drawEllipse (cx - maxR * f, cy - maxR * f, maxR * f * 2.0f, maxR * f * 2.0f, 0.8f);
    g.drawLine (cx - maxR, cy, cx + maxR, cy, 0.6f);
    g.drawLine (cx, cy - maxR, cx, cy + maxR, 0.6f);

    // rotating sweep with fading trail
    const float a = juce::MathConstants<float>::twoPi * (clockwise ? phase : 1.0f - phase);
    for (int t = 0; t < 10; ++t)
    {
        const float trailA = a - (clockwise ? 1.0f : -1.0f) * 0.05f * (float) t;
        g.setColour (Jetsonic::cyan().withAlpha (0.30f * (1.0f - (float) t / 10.0f)));
        g.drawLine (cx, cy, cx + maxR * std::sin (trailA), cy - maxR * std::cos (trailA), t == 0 ? 1.4f : 1.0f);
    }

    g.setColour (Jetsonic::cyanBright().withAlpha (0.9f));
    g.fillEllipse (cx - 1.5f, cy - 1.5f, 3.0f, 3.0f);
}

void CenterDashboard::paintBlueprint (juce::Graphics& g, juce::Rectangle<float> area)
{
    Jetsonic::fillGlassScreen (g, area, 6.0f, 0.32f);

    auto inner = area.reduced (6.0f);

    // thin cyan grid
    g.setColour (Jetsonic::cyan().withAlpha (0.14f));
    for (float x = inner.getX(); x <= inner.getRight(); x += 14.0f)
        g.drawLine (x, inner.getY(), x, inner.getBottom(), 0.5f);
    for (float y = inner.getY(); y <= inner.getBottom(); y += 14.0f)
        g.drawLine (inner.getX(), y, inner.getRight(), y, 0.5f);

    // gold aircraft illustration
    auto planeArea = inner.withSizeKeepingCentre (inner.getHeight() * 1.15f, inner.getHeight() * 0.92f);
    const auto plane = JetsonicIcons::aircraftTop();
    JetsonicIcons::fill (g, plane, planeArea, Jetsonic::goldBright().withAlpha (0.9f));
    JetsonicIcons::stroke (g, plane, planeArea.expanded (4.0f), Jetsonic::gold().withAlpha (0.35f), 0.8f);

    // corner ticks
    g.setColour (Jetsonic::cyan().withAlpha (0.5f));
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
    g.setColour (Jetsonic::goldDeep().withAlpha (0.30f));
    g.drawRoundedRectangle (bezel, 8.0f, 1.0f);
    g.setColour (juce::Colours::white.withAlpha (0.05f));
    g.fillRect (juce::Rectangle<float> (bezel.getX() + 3.0f, bezel.getY() + 1.0f, bezel.getWidth() - 6.0f, 1.0f));

    paintMiniDisplay (g);

    // gold preset title
    g.setFont (Jetsonic::value (17.0f));
    g.setColour (Jetsonic::goldBright());
    g.drawText (titleText, 0, kMiniStripH + 4, getWidth(), kTitleH - 6, juce::Justification::centred);

    // GLOBALS section label above the output knob
    g.setFont (Jetsonic::label (9.5f, 0.14f));
    g.setColour (Jetsonic::gold().withAlpha (0.92f));
    g.drawText ("GLOBALS", 6, kMiniStripH + kTitleH + 4, 80, 12, juce::Justification::centred);

    paintRadar (g, radarLeftArea().toFloat(), sweepPhase, true);
    paintRadar (g, radarRightArea().toFloat(), sweepPhase, false);
    paintBlueprint (g, blueprintArea().toFloat());
}
