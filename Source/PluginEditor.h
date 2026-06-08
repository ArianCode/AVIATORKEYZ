#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "GUI/MainPanel.h"
#include "GUI/Advanced/AdvancedPanel.h"
#include "GUI/ViewModeTabBar.h"
#include "PluginProcessor.h"

class AviatorKeyzEditor final : public juce::AudioProcessorEditor
{
public:
    explicit AviatorKeyzEditor (AviatorKeyzProcessor& processor);
    ~AviatorKeyzEditor() override;

    void paint (juce::Graphics& g) override;
    void resized() override;
    void visibilityChanged() override;

private:
    void layoutContent();
    void setAdvancedView (bool advanced);

    AviatorKeyzProcessor& processorRef;

    static constexpr int kDefaultWidth  = 1400;
    static constexpr int kDefaultHeight = 808;
    static constexpr int kMinWidth      = 700;
    static constexpr int kMinHeight     = 404;
    static constexpr int kMaxWidth      = 1800;
    static constexpr int kMaxHeight     = 1040;
    static constexpr int kTopChromePad    = 12;

    ViewModeTabBar viewTabs;
    std::unique_ptr<MainPanel> mainPanel;
    std::unique_ptr<AdvancedPanel> advancedPanel;
    bool advancedView { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AviatorKeyzEditor)
};
