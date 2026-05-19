#include "LuxuryLookAndFeel.h"

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
