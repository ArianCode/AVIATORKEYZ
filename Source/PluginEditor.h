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

    static constexpr int kDefaultWidth  = 860;
    static constexpr int kDefaultHeight = 608;
    static constexpr int kMinWidth      = 700;
    static constexpr int kMinHeight     = 495;  // 700 * 608/860
    static constexpr int kMaxWidth      = 1720; // 2x design
    static constexpr int kMaxHeight     = 1216;

    std::unique_ptr<MainPanel> mainPanel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AviatorKeyzEditor)
};
