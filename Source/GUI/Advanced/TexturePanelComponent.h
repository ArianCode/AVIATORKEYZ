#pragma once

#include "AdvancedKnobHelpers.h"
#include "TextureVisualizerComponent.h"
#include <memory>
#include <vector>

class TexturePanelComponent : public juce::Component,
                              private juce::Timer
{
public:
    explicit TexturePanelComponent (juce::AudioProcessorValueTreeState& apvts);

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    void timerCallback() override;

    juce::AudioProcessorValueTreeState& apvtsRef;
    std::unique_ptr<TextureVisualizerComponent> visualizer;
    std::vector<std::unique_ptr<PrecisionKnob>> knobs;
};
