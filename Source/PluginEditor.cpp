#include "PluginEditor.h"
#include "GUI/LuxuryLookAndFeel.h"

AviatorKeyzEditor::AviatorKeyzEditor (AviatorKeyzProcessor& p)
    : AudioProcessorEditor (&p)
    , processorRef (p)
{
    setResizable (true, true);
    setResizeLimits (kMinWidth, kMinHeight, kMaxWidth, kMaxHeight);
    setSize (kDefaultWidth, kDefaultHeight);

    mainPanel = std::make_unique<MainPanel> (p);
    addAndMakeVisible (*mainPanel);
}

AviatorKeyzEditor::~AviatorKeyzEditor() = default;

void AviatorKeyzEditor::paint (juce::Graphics& g)
{
    g.fillAll (LuxuryLookAndFeel::backgroundColour());
}

void AviatorKeyzEditor::resized()
{
    if (mainPanel != nullptr)
        mainPanel->setBounds (getLocalBounds());
}
