#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

// =============================================================================
//  TopHeader — deep metallic application header.
//  Aviation wing branding (left), MAIN / PERFORMANCE navigation (center),
//  dice + settings gear + utility icon + SAVE button (right).
// =============================================================================

class TopHeader : public juce::Component,
                  public juce::TooltipClient
{
public:
    TopHeader();

    std::function<void (bool performance)> onModeChanged;
    /** Dice: new sound, same effects. true when shift-clicked (any category). */
    std::function<void (bool anyCategory)> onRandomSoundClicked;
    std::function<void()> onSettingsClicked;
    std::function<void()> onUtilityClicked;
    std::function<void()> onSaveClicked;

    void setPerformanceSelected (bool performance);
    bool isPerformanceSelected() const noexcept { return performanceSelected; }

    void paint (juce::Graphics& g) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseMove (const juce::MouseEvent& e) override;
    void mouseExit (const juce::MouseEvent& e) override;
    juce::String getTooltip() override;

private:
    enum class Hit { none, mainTab, perfTab, dice, gear, utility, save };
    Hit hitAt (juce::Point<int> pos) const;

    juce::Rectangle<int> mainTabArea, perfTabArea, diceArea, gearArea, utilityArea, saveArea;
    int diceFace { 5 };
    bool performanceSelected { false };
    Hit hovered { Hit::none };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TopHeader)
};
