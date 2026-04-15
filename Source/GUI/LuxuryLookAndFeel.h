#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

// =============================================================================
//  LuxuryLookAndFeel — M4
//
//  The visual identity of AviatorKeyz.
//
//  Palette (from UI reference mockup):
//    Background:   #111111 (near-black)
//    Surface:      #1E1E1E (elevated panels)
//    Gold primary: #C8922A (knob accents, highlights)
//    Gold light:   #E8B84B (hover states, active indicators)
//    Gold dark:    #8B6219 (inset shadows, borders)
//    Text primary: #F0E6D0 (warm off-white)
//    Text dim:     #7A7063 (labels, secondary)
//    Accent blue:  #4A7FA5 (waveform, optional)
//
//  Custom draws:
//    - Rotary knob: thin arc track + gold pointer + subtle glow on hover
//    - Toggle button: pill-shaped with gold fill when active
//    - Slider (linear): thin track with gold thumb
//    - ComboBox: minimal dropdown with gold caret
//    - ListBox: dark row backgrounds, gold selection highlight
//
//  Implemented in M4.
// =============================================================================

class LuxuryLookAndFeel : public juce::LookAndFeel_V4
{
public:
    LuxuryLookAndFeel();
    ~LuxuryLookAndFeel() override;

    // Colour palette accessors
    static juce::Colour backgroundColour()  noexcept { return juce::Colour (0xff111111); }
    static juce::Colour surfaceColour()     noexcept { return juce::Colour (0xff1e1e1e); }
    static juce::Colour goldPrimary()       noexcept { return juce::Colour (0xffc8922a); }
    static juce::Colour goldLight()         noexcept { return juce::Colour (0xffe8b84b); }
    static juce::Colour goldDark()          noexcept { return juce::Colour (0xff8b6219); }
    static juce::Colour textPrimary()       noexcept { return juce::Colour (0xfff0e6d0); }
    static juce::Colour textDim()           noexcept { return juce::Colour (0xff7a7063); }

    // M4: override drawRotarySlider, drawButtonBackground, drawComboBox, etc.

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LuxuryLookAndFeel)
};
