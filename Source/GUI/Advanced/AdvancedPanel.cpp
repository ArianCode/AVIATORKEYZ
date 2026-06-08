#include "AdvancedPanel.h"
#include "../../PluginProcessor.h"

AdvancedPanel::AdvancedPanel (AviatorKeyzProcessor& processor)
    : processorRef (processor)
{
    content = std::make_unique<AdvancedPageContent> (processor.getAPVTS());
    addAndMakeVisible (*content);
}

void AdvancedPanel::refreshPresetUI()
{
}

void AdvancedPanel::paint (juce::Graphics& g)
{
    g.fillAll (AdvancedWidgets::kNavyBg());
}

void AdvancedPanel::resized()
{
    if (content != nullptr)
        content->setBounds (getLocalBounds());
}
