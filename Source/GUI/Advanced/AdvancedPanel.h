#pragma once

#include "AdvancedPageContent.h"
#include "../AviatorTokens.h"
#include <memory>

class AviatorKeyzProcessor;

/** Advanced view — full-width engine layout (preset browser lives on Main page). */
class AdvancedPanel : public juce::Component
{
public:
    explicit AdvancedPanel (AviatorKeyzProcessor& processor);

    void refreshPresetUI();

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    AviatorKeyzProcessor& processorRef;
    std::unique_ptr<AdvancedPageContent> content;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AdvancedPanel)
};
