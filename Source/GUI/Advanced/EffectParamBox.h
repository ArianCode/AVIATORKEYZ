#pragma once

#include "EffectParamRow.h"
#include <memory>
#include <vector>

struct EffectParamRowSpec
{
    const char* paramId {};
    juce::String label;
    juce::String tooltip;
    EffectCell::Format format { EffectCell::Format::percent };
    juce::StringArray choices;
    bool isChoice { false };
};

/** Function box with header, row-list params, and shared vertical slider rail. */
class EffectParamBox : public juce::Component
{
public:
    EffectParamBox (juce::AudioProcessorValueTreeState& apvts,
                    const juce::String& title,
                    std::vector<EffectParamRowSpec> rows);

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    juce::String titleText;
    std::vector<std::unique_ptr<EffectParamRow>> paramRows;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EffectParamBox)
};
