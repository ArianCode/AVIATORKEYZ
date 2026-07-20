#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

// =============================================================================
//  StatusBar — bottom strip.
//  Left: ACTIVE dot, sample rate, bit depth, host BPM.
//  Center: FlybyLoops brand.
//  Right: PRESETS caption, A/B compare dropdown, version.
// =============================================================================

class StatusBar : public juce::Component,
                  private juce::Timer
{
public:
    StatusBar();

    /** Live host/plugin state providers (message thread). */
    std::function<double()> sampleRateProvider;
    std::function<double()> bpmProvider;
    std::function<bool()> activeProvider;

    std::function<void()> onAbClicked;

    void setAbLabel (const juce::String& text);
    void setVersionText (const juce::String& text);

    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseMove (const juce::MouseEvent& e) override;
    void mouseExit (const juce::MouseEvent&) override { abHovered = false; repaint(); }

private:
    void timerCallback() override;
    juce::Rectangle<int> abArea() const;

    juce::String abLabel { "AB" };
    juce::String versionText;
    bool abHovered { false };

    // cached display state so we only repaint on change
    double lastSampleRate { 0.0 };
    double lastBpm { 0.0 };
    bool lastActive { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StatusBar)
};
