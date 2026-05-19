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

private:
    AviatorKeyzProcessor& processorRef;

    static constexpr int kDefaultWidth  = 900;
    static constexpr int kMinWidth      = 700;
    static constexpr int kDefaultHeight = 520;
    static constexpr int kMinHeight     = 404;
    static constexpr int kMaxWidth      = 1800;
    static constexpr int kMaxHeight     = 1040;

    std::unique_ptr<MainPanel> mainPanel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AviatorKeyzEditor)
};
