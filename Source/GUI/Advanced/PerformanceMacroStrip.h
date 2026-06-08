#pragma once

#include "../PrecisionKnob.h"
#include "../../State/StateSchema.h"
#include <array>
#include <memory>

/** Bottom performance macro knobs for the Matrix tab. */
class PerformanceMacroStrip : public juce::Component
{
public:
    explicit PerformanceMacroStrip (juce::AudioProcessorValueTreeState& apvts);

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    std::array<std::unique_ptr<PrecisionKnob>, 4> macros;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PerformanceMacroStrip)
};
