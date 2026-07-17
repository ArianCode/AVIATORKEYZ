#include "AdvancedPanel.h"
#include "AdvancedWidgets.h"
#include "../../PluginProcessor.h"

AdvancedPanel::AdvancedPanel (AviatorKeyzProcessor& processor)
    : processorRef (processor)
{
    content = std::make_unique<AdvancedPageContent> (processor.getAPVTS());
    addAndMakeVisible (*content);
}

void AdvancedPanel::refreshPresetUI()
{
    if (content == nullptr)
        return;

    const auto& macros = processorRef.getMacroControls();
    std::array<juce::String, 4> labels;
    for (int i = 0; i < 4; ++i)
    {
        labels[(size_t) i] = macros[(size_t) i].name.isNotEmpty()
                                 ? macros[(size_t) i].name
                                 : "MACRO " + juce::String (i + 1);
    }

    content->setMacroLabels (labels);
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
