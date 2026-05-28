#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "GUI/MainPanel.h"
#include "PluginProcessor.h"

class AviatorKeyzEditor final : public juce::AudioProcessorEditor
{
public:
    explicit AviatorKeyzEditor (AviatorKeyzProcessor& processor);
    ~AviatorKeyzEditor() override;

    void paint (juce::Graphics& g) override;
    void resized() override;
    void visibilityChanged() override;

    void layoutMainPanel();

private:
    AviatorKeyzProcessor& processorRef;

    static constexpr int kDefaultWidth  = 1600;
    static constexpr int kDefaultHeight = 922; // 900 photo + 22 footer
    static constexpr int kMinWidth      = 1280;
    static constexpr int kMinHeight     = 742;
    static constexpr int kMaxWidth      = 2048;
    static constexpr int kMaxHeight     = 1186;

    std::unique_ptr<MainPanel> mainPanel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AviatorKeyzEditor)
};
