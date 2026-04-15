#include "LuxuryLookAndFeel.h"

LuxuryLookAndFeel::LuxuryLookAndFeel()
{
    // Set base colour scheme to match our dark palette
    setColour (juce::ResizableWindow::backgroundColourId, backgroundColour());
    setColour (juce::Slider::thumbColourId,               goldPrimary());
    setColour (juce::Slider::trackColourId,               surfaceColour());
    setColour (juce::Slider::rotarySliderFillColourId,    goldPrimary());
    setColour (juce::Label::textColourId,                 textPrimary());
    setColour (juce::ComboBox::backgroundColourId,        surfaceColour());
    setColour (juce::ComboBox::textColourId,              textPrimary());
    setColour (juce::TextButton::buttonColourId,          surfaceColour());
    setColour (juce::TextButton::textColourOnId,          goldPrimary());
    setColour (juce::TextButton::textColourOffId,         textDim());
    setColour (juce::ListBox::backgroundColourId,         backgroundColour());
    setColour (juce::ListBox::textColourId,               textPrimary());
}

LuxuryLookAndFeel::~LuxuryLookAndFeel() = default;

// M4: drawRotarySlider(...), drawButtonBackground(...), drawComboBox(...) etc.
