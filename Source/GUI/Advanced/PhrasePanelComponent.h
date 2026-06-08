#pragma once

#include "AdvancedKnobHelpers.h"
#include "../ReverseToggle.h"
#include <memory>
#include <vector>

class PhrasePanelComponent : public juce::Component
{
public:
    explicit PhrasePanelComponent (juce::AudioProcessorValueTreeState& apvts);

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    juce::Label browserLabel { {}, "PHRASE BROWSER — select preset category for one-shots" };
    std::unique_ptr<ReverseToggle> phraseOn;
    std::unique_ptr<ReverseToggle> tempoSync;
    std::unique_ptr<ReverseToggle> keySync;
    std::unique_ptr<ReverseToggle> loopToggle;
    std::vector<std::unique_ptr<PrecisionKnob>> knobs;
};
