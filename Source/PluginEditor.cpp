#include "PluginEditor.h"
#include "GUI/DesignTokens.h"

AviatorKeyzEditor::AviatorKeyzEditor (AviatorKeyzProcessor& p)
    : AudioProcessorEditor (&p)
    , processorRef (p)
{
    setOpaque (true);
    setSize (kDefaultWidth, kDefaultHeight);
    setResizable (true, true);
    setResizeLimits (kMinWidth, kMinHeight, kMaxWidth, kMaxHeight);

    if (auto* c = getConstrainer())
        c->setFixedAspectRatio ((double) kDefaultWidth / (double) kDefaultHeight);

    mainPanel = std::make_unique<MainPanel> (p);
    mainPanel->setOpaque (true);
    addAndMakeVisible (*mainPanel);

    layoutMainPanel();
    repaint();
}

AviatorKeyzEditor::~AviatorKeyzEditor() = default;

void AviatorKeyzEditor::layoutMainPanel()
{
    if (mainPanel == nullptr)
        return;

    auto bounds = getLocalBounds();
    if (bounds.isEmpty())
        bounds = { 0, 0, kDefaultWidth, kDefaultHeight };

    mainPanel->setBounds (bounds);
    mainPanel->resized();
}

void AviatorKeyzEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff030309));
}

void AviatorKeyzEditor::resized()
{
    layoutMainPanel();
}

void AviatorKeyzEditor::visibilityChanged()
{
    if (isShowing())
        layoutMainPanel();
}
