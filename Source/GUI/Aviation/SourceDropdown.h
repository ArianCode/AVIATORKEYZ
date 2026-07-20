#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

// =============================================================================
//  SourceDropdown — floating top-right sound-source selector.
//  Dark rounded rectangle, fine gold border, waveform icon, source name,
//  down chevron. Clicking opens the sample-source menu (owner-provided).
// =============================================================================

class SourceDropdown : public juce::Component
{
public:
    SourceDropdown();

    std::function<void()> onClicked;

    void setSourceText (const juce::String& text);

    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseEnter (const juce::MouseEvent&) override { hovered = true; repaint(); }
    void mouseExit (const juce::MouseEvent&) override { hovered = false; repaint(); }

private:
    juce::String sourceText;
    bool hovered { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SourceDropdown)
};
