#include "StatusBar.h"
#include "AviationIcons.h"
#include "AviationTheme.h"

StatusBar::StatusBar()
{
    startTimerHz (2);
}

void StatusBar::timerCallback()
{
    const double sr  = sampleRateProvider ? sampleRateProvider() : 0.0;
    const double bpm = bpmProvider ? bpmProvider() : 0.0;
    const bool active = activeProvider ? activeProvider() : false;

    if (! juce::exactlyEqual (sr, lastSampleRate) || ! juce::exactlyEqual (bpm, lastBpm)
        || active != lastActive)
    {
        lastSampleRate = sr;
        lastBpm = bpm;
        lastActive = active;
        repaint();
    }
}

juce::Rectangle<int> StatusBar::abArea() const
{
    return { getWidth() - 150, getHeight() / 2 - 12, 62, 24 };
}

void StatusBar::mouseMove (const juce::MouseEvent& e)
{
    const bool over = abArea().contains (e.getPosition());
    if (over != abHovered)
    {
        abHovered = over;
        setMouseCursor (over ? juce::MouseCursor::PointingHandCursor
                             : juce::MouseCursor::NormalCursor);
        repaint();
    }
}

void StatusBar::mouseDown (const juce::MouseEvent& e)
{
    if (abArea().contains (e.getPosition()) && onAbClicked)
        onAbClicked();
}

void StatusBar::setAbLabel (const juce::String& text)
{
    if (abLabel != text)
    {
        abLabel = text;
        repaint();
    }
}

void StatusBar::setVersionText (const juce::String& text)
{
    versionText = text;
    repaint();
}

void StatusBar::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();

    juce::Path panel;
    panel.addRoundedRectangle (r.getX(), r.getY(), r.getWidth(), r.getHeight(),
                               9.0f, 9.0f, false, false, true, true);
    juce::ColourGradient grad (juce::Colour (0xff0a141d), r.getX(), r.getY(),
                               juce::Colour (0xff04090f), r.getX(), r.getBottom(), false);
    g.setGradientFill (grad);
    g.fillPath (panel);
    g.setColour (Aviation::goldDeep().withAlpha (0.30f));
    g.strokePath (panel, juce::PathStrokeType (1.0f));

    const int cy = getHeight() / 2;

    // --- Left cluster --------------------------------------------------------
    int x = 22;
    const bool active = lastActive;
    g.setColour ((active ? Aviation::activeGreen() : Aviation::textDim()).withAlpha (0.35f));
    g.fillEllipse ((float) x - 3.0f, (float) cy - 7.0f, 14.0f, 14.0f);
    g.setColour (active ? Aviation::activeGreen() : Aviation::textDim());
    g.fillEllipse ((float) x, (float) cy - 4.0f, 8.0f, 8.0f);
    x += 18;

    g.setFont (Aviation::label (11.0f, 0.12f));
    g.setColour (Aviation::textPrimary().withAlpha (0.9f));
    g.drawText ("ACTIVE", x, cy - 9, 60, 18, juce::Justification::centredLeft);
    x += 70;

    auto divider = [&] (int dx)
    {
        g.setColour (juce::Colour (0x28405a70));
        g.fillRect (dx, cy - 8, 1, 16);
    };

    g.setFont (Aviation::body (11.5f));

    divider (x); x += 14;
    const auto srText = [this]() -> juce::String
    {
        if (lastSampleRate <= 0.0)
            return "- kHz";
        const double kHz = lastSampleRate / 1000.0;
        const bool wholeKhz = std::abs (kHz - std::round (kHz)) < 0.05;
        if (wholeKhz)
            return juce::String (juce::roundToInt (kHz)) + " kHz";
        return juce::String (kHz, 1) + " kHz";
    }();
    g.setColour (Aviation::textSecondary());
    g.drawText (srText, x, cy - 9, 68, 18, juce::Justification::centredLeft);
    x += 74;

    divider (x); x += 14;
    g.setColour (Aviation::textSecondary());
    g.drawText ("24 bit", x, cy - 9, 50, 18, juce::Justification::centredLeft);
    x += 56;

    divider (x); x += 14;
    const auto bpmText = lastBpm > 0.0 ? juce::String (juce::roundToInt (lastBpm)) + " BPM"
                                       : juce::String ("- BPM");
    g.setColour (Aviation::textSecondary());
    g.drawText (bpmText, x, cy - 9, 80, 18, juce::Justification::centredLeft);

    // --- Center brand ---------------------------------------------------------
    g.setFont (Aviation::body (12.5f));
    g.setColour (Aviation::textSecondary());
    g.drawText ("FlybyLoops", getLocalBounds(), juce::Justification::centred);

    // --- Right cluster ----------------------------------------------------------
    g.setFont (Aviation::label (11.0f, 0.12f));
    g.setColour (Aviation::textSecondary());
    g.drawText ("PRESETS", getWidth() - 240, cy - 9, 80, 18, juce::Justification::centredLeft);

    {
        const auto ab = abArea().toFloat();
        if (abHovered)
        {
            g.setColour (Aviation::cyan().withAlpha (0.12f));
            g.fillRoundedRectangle (ab, 5.0f);
        }
        g.setColour (Aviation::textSecondary().withAlpha (abHovered ? 1.0f : 0.7f));
        g.drawRoundedRectangle (ab, 5.0f, 1.0f);
        g.setFont (Aviation::label (11.0f, 0.10f));
        g.setColour (Aviation::textPrimary());
        g.drawText (abLabel, abArea().withTrimmedRight (18), juce::Justification::centred);
        AviationIcons::stroke (g, AviationIcons::chevronDown(),
                               { ab.getRight() - 18.0f, ab.getCentreY() - 4.0f, 10.0f, 9.0f },
                               Aviation::textPrimary().withAlpha (0.85f), 1.4f);
    }

    g.setFont (Aviation::body (11.5f));
    g.setColour (Aviation::textSecondary());
    g.drawText (versionText, getWidth() - 76, cy - 9, 60, 18, juce::Justification::centredRight);
}
