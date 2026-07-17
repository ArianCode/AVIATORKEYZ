#pragma once

#include "../AviatorTokens.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

/** Simple in-panel about dialog. No AlertWindow / NSWindow. */
class AboutOverlay : public juce::Component
{
public:
    AboutOverlay();

    std::function<void()> onDismiss;

    void showOverlay();
    void dismiss();

    void paint (juce::Graphics& g) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent& e) override;

private:
    juce::Rectangle<int> getPanelBounds() const;

    juce::TextButton closeButton { "OK" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AboutOverlay)
};
