#pragma once

#include "EffectCell.h"
#include "../../State/StateSchema.h"
#include <array>
#include <memory>

/** Bottom performance macro tiles for the Matrix tab. */
class PerformanceMacroStrip : public juce::Component
{
public:
    explicit PerformanceMacroStrip (juce::AudioProcessorValueTreeState& apvts);

    void paint (juce::Graphics& g) override;
    void resized() override;

    void setMacroLabels (const std::array<juce::String, 4>& labels);

private:
    std::array<std::unique_ptr<EffectCell>, 4> macros;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PerformanceMacroStrip)
};
