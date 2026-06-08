#pragma once

#include "AdvancedWidgets.h"
#include "BracketValueBox.h"
#include "TextureVisualizerComponent.h"
#include "../../State/StateSchema.h"
#include <memory>
#include <vector>

/** Hero Texture section — large visualizer + organized parameter grid. */
class TextureSectionComponent : public juce::Component
{
public:
    static constexpr int kDesignMinHeight = 220;

    explicit TextureSectionComponent (juce::AudioProcessorValueTreeState& apvts);

    void paint (juce::Graphics& g) override;
    void resized() override;
    void syncVisualizerFromParams();

private:
    using BV  = BracketValueBox;
    using Fmt = BracketValueBox::Format;

    void layoutParamGrid (juce::Rectangle<int> area, int boxW, int boxH, int rowGap, int colGap);

    juce::AudioProcessorValueTreeState& apvtsRef;
    std::unique_ptr<TextureVisualizerComponent> visualizer;
    std::unique_ptr<AdvancedWidgets::FlatToggle> freezeToggle;
    std::unique_ptr<AdvancedWidgets::FlatToggle> reverseToggle;

    std::vector<std::unique_ptr<BV>> grainBoxes;
    std::vector<std::unique_ptr<BV>> motionBoxes;

    juce::Rectangle<int> grainHeaderArea;
    juce::Rectangle<int> motionHeaderArea;
    juce::Rectangle<int> subgroupDividerArea;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TextureSectionComponent)
};
