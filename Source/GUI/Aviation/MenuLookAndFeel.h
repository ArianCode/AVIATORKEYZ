#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

// =============================================================================
//  MenuLookAndFeel — styles the preset / A/B popup menus as a private-jet
//  overhead luggage compartment: dark cabin panel with faint bin doors and
//  handles, a soft LED cabin light strip, fine gold trim. Presets are the
//  luggage — the loaded one is marked with a small gold suitcase.
// =============================================================================

class MenuLookAndFeel : public juce::LookAndFeel_V4
{
public:
    MenuLookAndFeel();

    void drawPopupMenuBackgroundWithOptions (juce::Graphics& g, int width, int height,
                                             const juce::PopupMenu::Options& options) override;

    void drawPopupMenuItem (juce::Graphics& g, const juce::Rectangle<int>& area,
                            bool isSeparator, bool isActive, bool isHighlighted,
                            bool isTicked, bool hasSubMenu,
                            const juce::String& text, const juce::String& shortcutKeyText,
                            const juce::Drawable* icon, const juce::Colour* textColour) override;

    juce::Font getPopupMenuFont() override;
    void getIdealPopupMenuItemSizeWithOptions (const juce::String& text, bool isSeparator,
                                               int standardMenuItemHeight,
                                               int& idealWidth, int& idealHeight,
                                               const juce::PopupMenu::Options& options) override;
    int getPopupMenuBorderSizeWithOptions (const juce::PopupMenu::Options& options) override;

private:
    static void drawSuitcase (juce::Graphics& g, juce::Rectangle<float> box, juce::Colour colour);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MenuLookAndFeel)
};
