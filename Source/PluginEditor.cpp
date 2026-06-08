#include "PluginEditor.h"
#include "GUI/AviatorTokens.h"

AviatorKeyzEditor::AviatorKeyzEditor (AviatorKeyzProcessor& p)
    : AudioProcessorEditor (&p)
    , processorRef (p)
{
    setOpaque (true);
    setResizable (true, true);
    setResizeLimits (kMinWidth, kMinHeight, kMaxWidth, kMaxHeight);
    setSize (kDefaultWidth, kDefaultHeight);

    addAndMakeVisible (viewTabs);
    viewTabs.onModeChanged = [this] (bool advanced) { setAdvancedView (advanced); };

    mainPanel = std::make_unique<MainPanel> (p);
    mainPanel->setOpaque (true);
    addAndMakeVisible (*mainPanel);

    advancedPanel = std::make_unique<AdvancedPanel> (p);
    advancedPanel->setVisible (false);
    addAndMakeVisible (*advancedPanel);

    viewTabs.setAdvancedSelected (false);
    setAdvancedView (false);

    layoutContent();
    repaint();
}

AviatorKeyzEditor::~AviatorKeyzEditor() = default;

void AviatorKeyzEditor::setAdvancedView (bool advanced)
{
    advancedView = advanced;
    if (mainPanel != nullptr)
        mainPanel->setVisible (! advanced);
    if (advancedPanel != nullptr)
        advancedPanel->setVisible (advanced);
    layoutContent();
}

void AviatorKeyzEditor::layoutContent()
{
    auto bounds = getLocalBounds();
    if (bounds.isEmpty())
        bounds = { 0, 0, kDefaultWidth, kDefaultHeight };

    bounds.removeFromTop (AviatorTokens::scaledFor (*this, kTopChromePad));

    const int tabH = AviatorTokens::scaledFor (*this, ViewModeTabBar::kDesignHeight);
    viewTabs.setBounds (bounds.removeFromTop (tabH));

    if (mainPanel != nullptr)
        mainPanel->setBounds (bounds);

    if (advancedPanel != nullptr)
        advancedPanel->setBounds (bounds);

    viewTabs.toFront (false);
}

void AviatorKeyzEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff030309));
}

void AviatorKeyzEditor::resized()
{
    layoutContent();
}

void AviatorKeyzEditor::visibilityChanged()
{
    if (isShowing())
        layoutContent();
}
