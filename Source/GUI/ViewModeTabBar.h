#pragma once

#include "AviatorTokens.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

/** Persistent MAIN / ADVANCED tabs — lives in PluginEditor, never hidden. */
class ViewModeTabBar : public juce::Component
{
public:
    ViewModeTabBar();

    std::function<void (bool advanced)> onModeChanged;

    void setAdvancedSelected (bool advanced);
    bool isAdvancedSelected() const { return advancedSelected; }

    void paint (juce::Graphics& g) override;
    void resized() override;

    static constexpr int kDesignHeight = 34;

private:
    struct ModeTabButton : juce::TextButton
    {
        explicit ModeTabButton (const juce::String& text) : juce::TextButton (text) {}
        void paintButton (juce::Graphics& g, bool highlighted, bool down) override;
    };

    void styleTab (ModeTabButton& btn, bool active);

    ModeTabButton mainTab { "MAIN" };
    ModeTabButton advancedTab { "PERFORMANCE" };
    bool advancedSelected { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ViewModeTabBar)
};
