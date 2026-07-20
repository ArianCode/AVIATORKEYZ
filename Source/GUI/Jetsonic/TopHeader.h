#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

// =============================================================================
//  TopHeader — deep metallic application header.
//  Jetsonic wing branding (left), MAIN / PERFORMANCE navigation (center),
//  settings gear + utility icon + SAVE button (right).
// =============================================================================

class TopHeader : public juce::Component
{
public:
    TopHeader();

    std::function<void (bool performance)> onModeChanged;
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

private:
    enum class Hit { none, mainTab, perfTab, gear, utility, save };
    Hit hitAt (juce::Point<int> pos) const;

    juce::Rectangle<int> mainTabArea, perfTabArea, gearArea, utilityArea, saveArea;
    bool performanceSelected { false };
    Hit hovered { Hit::none };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TopHeader)
};
