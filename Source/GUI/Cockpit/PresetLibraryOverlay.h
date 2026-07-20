#pragma once

#include "../AviatorTokens.h"
#include "../PresetBrowser.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>
#include <memory>

class AviatorKeyzProcessor;

/** In-panel preset library (category + preset combos). No floating NSWindow. */
class PresetLibraryOverlay : public juce::Component
{
public:
    explicit PresetLibraryOverlay (AviatorKeyzProcessor& processor);

    std::function<void()> onDismiss;

    void showOverlay();
    void dismiss();

    void paint (juce::Graphics& g) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent& e) override;

private:
    juce::Rectangle<int> getPanelBounds() const;

    AviatorKeyzProcessor& processorRef;
    std::unique_ptr<PresetBrowser> browser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetLibraryOverlay)
};
