#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

// =============================================================================
//  LuxuryLookAndFeel — M4
//
//  The visual identity of AviatorKeyz.
//
//  Palette (from DesignTokens / aviatorkeyz_ui_v3.html):
//    Background:   #08080d (deep)
//    Surface:      #100f17 (panel)
//    Champagne:    #d4bc86 (knob accents, highlights)
//    Champagne bright: #e2ce9e (hover states, active indicators)
//    Champagne mid:    #c4aa70 (inset shadows, borders)
//    Text primary: #edeae1 (warm off-white)
//    Text dim:     #7b7788 (labels, secondary)
//
//  Custom draws:
//    - Rotary knob: thin arc track + champagne pointer + subtle glow on hover
//    - Button background: pill-shaped, minimal surface or transparent
//    - Toggle button: pill-shaped with champagne fill when active
//    - Slider (linear): thin track with champagne thumb
//    - ComboBox: minimal dropdown with champagne caret
//    - ListBox: dark row backgrounds, champagne selection highlight
// =============================================================================

class LuxuryLookAndFeel : public juce::LookAndFeel_V4
{
public:
    LuxuryLookAndFeel();
    ~LuxuryLookAndFeel() override;

    // Colour palette accessors
    static juce::Colour backgroundColour()  noexcept;
    static juce::Colour surfaceColour()     noexcept;
    static juce::Colour goldPrimary()       noexcept;
    static juce::Colour goldLight()         noexcept;
    static juce::Colour goldDark()          noexcept;
    static juce::Colour textPrimary()       noexcept;
    static juce::Colour textDim()           noexcept;

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPosProportional, float rotaryStartAngle,
                           float rotaryEndAngle, juce::Slider& slider) override;

    void drawButtonBackground (juce::Graphics& g, juce::Button& button,
                               const juce::Colour& backgroundColour,
                               bool shouldDrawButtonAsHighlighted,
                               bool shouldDrawButtonAsDown) override;

    void drawToggleButton (juce::Graphics& g, juce::ToggleButton& button,
                           bool shouldDrawButtonAsHighlighted,
                           bool shouldDrawButtonAsDown) override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LuxuryLookAndFeel)
};
