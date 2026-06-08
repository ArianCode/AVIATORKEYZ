#pragma once

#include "AdvancedKnobHelpers.h"
#include <memory>
#include <vector>

class SynthPanelComponent : public juce::Component
{
public:
    explicit SynthPanelComponent (juce::AudioProcessorValueTreeState& apvts);

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    juce::Label filterTitle { {}, "FILTER" };
    juce::Label ampTitle { {}, "AMP ENVELOPE" };
    juce::Label voiceTitle { {}, "VOICE" };
    std::vector<std::unique_ptr<PrecisionKnob>> knobs;
};
