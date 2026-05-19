#include "LuxuryLookAndFeel.h"
#include "DesignTokens.h"

juce::Colour LuxuryLookAndFeel::backgroundColour() noexcept { return DesignTokens::deep(); }
juce::Colour LuxuryLookAndFeel::surfaceColour()    noexcept { return DesignTokens::panel(); }
juce::Colour LuxuryLookAndFeel::goldPrimary()      noexcept { return DesignTokens::champagne(); }
juce::Colour LuxuryLookAndFeel::goldLight()        noexcept { return DesignTokens::champagneBright(); }
juce::Colour LuxuryLookAndFeel::goldDark()         noexcept { return DesignTokens::champagneMid(); }
juce::Colour LuxuryLookAndFeel::textPrimary()      noexcept { return DesignTokens::textPrimary(); }
juce::Colour LuxuryLookAndFeel::textDim()          noexcept { return DesignTokens::textSecondary(); }

LuxuryLookAndFeel::LuxuryLookAndFeel()
{
    setColour (juce::ResizableWindow::backgroundColourId, backgroundColour());
    setColour (juce::Slider::thumbColourId, goldPrimary());
    setColour (juce::Slider::trackColourId, surfaceColour());
    setColour (juce::Slider::rotarySliderFillColourId, goldPrimary());
    setColour (juce::Label::textColourId, textPrimary());
    setColour (juce::ComboBox::backgroundColourId, surfaceColour());
    setColour (juce::ComboBox::textColourId, textPrimary());
    setColour (juce::ComboBox::outlineColourId, goldDark());
    setColour (juce::TextButton::buttonColourId, surfaceColour());
    setColour (juce::TextButton::textColourOnId, goldPrimary());
    setColour (juce::TextButton::textColourOffId, textDim());
    setColour (juce::ListBox::backgroundColourId, backgroundColour());
    setColour (juce::ListBox::textColourId, textPrimary());
    setColour (juce::ToggleButton::textColourId, textPrimary());
    setColour (juce::ToggleButton::tickColourId, goldPrimary());
}

LuxuryLookAndFeel::~LuxuryLookAndFeel() = default;

void LuxuryLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                           float sliderPosProportional,
                                           float rotaryStartAngle,
                                           float rotaryEndAngle,
                                           juce::Slider& slider)
{
    juce::ignoreUnused (slider);

    const auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat().reduced (6.f);
    const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto  centre = bounds.getCentre();

    g.setColour (surfaceColour());
    g.fillEllipse (bounds);

    g.setColour (goldDark());
    g.drawEllipse (bounds, 1.2f);

    juce::Path track;
    track.addCentredArc (centre.x,
                         centre.y,
                         radius * 0.72f,
                         radius * 0.72f,
                         0.f,
                         rotaryStartAngle,
                         rotaryEndAngle,
                         true);
    g.setColour (goldDark().withAlpha (0.55f));
    g.strokePath (track, juce::PathStrokeType (2.5f));

    const float angle =
        rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

    juce::Path valueArc;
    valueArc.addCentredArc (centre.x,
                            centre.y,
                            radius * 0.72f,
                            radius * 0.72f,
                            0.f,
                            rotaryStartAngle,
                            angle,
                            true);
    g.setColour (goldPrimary());
    g.strokePath (valueArc, juce::PathStrokeType (3.f));

    const juce::Line<float> needle (centre,
                                    centre.getPointOnCircumference (radius * 0.55f, angle));
    g.setColour (goldLight());
    g.drawLine (needle, 2.2f);
}

void LuxuryLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button,
                                               const juce::Colour& backgroundColour,
                                               bool shouldDrawButtonAsHighlighted,
                                               bool shouldDrawButtonAsDown)
{
    auto bounds = button.getLocalBounds().toFloat().reduced (0.5f);
    const float corner = juce::jmax (4.f, bounds.getHeight() * 0.5f);

    if (backgroundColour.isTransparent())
    {
        if (shouldDrawButtonAsHighlighted || shouldDrawButtonAsDown)
        {
            g.setColour (goldPrimary().withAlpha (shouldDrawButtonAsDown ? 0.14f : 0.07f));
            g.fillRoundedRectangle (bounds, corner);
        }

        g.setColour (shouldDrawButtonAsHighlighted ? goldPrimary().withAlpha (0.35f)
                                                    : juce::Colours::transparentBlack);
        g.drawRoundedRectangle (bounds, corner, shouldDrawButtonAsHighlighted ? 0.8f : 0.f);
        return;
    }

    juce::ColourGradient bg (backgroundColour.brighter (0.08f), bounds.getX(), bounds.getY(),
                             backgroundColour.darker (0.12f), bounds.getX(), bounds.getBottom(), false);

    if (shouldDrawButtonAsDown)
        bg = juce::ColourGradient (backgroundColour.darker (0.08f), bounds.getX(), bounds.getY(),
                                   backgroundColour, bounds.getX(), bounds.getBottom(), false);

    g.setGradientFill (bg);
    g.fillRoundedRectangle (bounds, 6.f);

    g.setColour (shouldDrawButtonAsHighlighted ? goldPrimary().withAlpha (0.45f) : goldDark().withAlpha (0.35f));
    g.drawRoundedRectangle (bounds, 6.f, 0.8f);
}

void LuxuryLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& button,
                                           bool shouldDrawButtonAsHighlighted,
                                           bool shouldDrawButtonAsDown)
{
    juce::ignoreUnused (shouldDrawButtonAsDown);
    const bool on = button.getToggleState();
    auto bounds = button.getLocalBounds().toFloat().reduced (1.f);
    const float corner = bounds.getHeight() * 0.5f;

    if (on)
    {
        juce::ColourGradient bg (goldPrimary().withAlpha (0.22f), bounds.getX(), bounds.getY(),
                                 goldPrimary().withAlpha (0.08f), bounds.getX(), bounds.getBottom(), false);
        g.setGradientFill (bg);
        g.fillRoundedRectangle (bounds, corner);
        g.setColour (goldPrimary().withAlpha (0.55f));
    }
    else
    {
        g.setColour (surfaceColour());
        g.fillRoundedRectangle (bounds, corner);
        g.setColour (goldDark().withAlpha (0.4f));
    }

    if (shouldDrawButtonAsHighlighted && ! on)
        g.setColour (goldPrimary().withAlpha (0.25f));

    g.drawRoundedRectangle (bounds, corner, on ? 1.f : 0.7f);
}
