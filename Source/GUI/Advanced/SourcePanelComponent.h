#pragma once

#include "AdvancedKnobHelpers.h"
#include "../../State/StateSchema.h"
#include <memory>
#include <vector>

class SourcePanelComponent : public juce::Component
{
public:
    explicit SourcePanelComponent (juce::AudioProcessorValueTreeState& apvts);

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    std::vector<std::unique_ptr<PrecisionKnob>> knobs;
    juce::Label osc1Title { {}, "OSC 1" };
    juce::Label osc2Title { {}, "OSC 2" };
    juce::Label sampleTitle { {}, "SAMPLE ENGINE" };
};
