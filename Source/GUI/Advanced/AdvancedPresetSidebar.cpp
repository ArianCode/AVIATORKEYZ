#include "AdvancedPresetSidebar.h"
#include "../../PluginProcessor.h"

AdvancedPresetSidebar::AdvancedPresetSidebar (AviatorKeyzProcessor& p)
    : processorRef (p)
{
    titleLabel.setFont (AviatorTokens::hudBold (12.f));
    titleLabel.setColour (juce::Label::textColourId, AviatorTokens::champagneGold());
    titleLabel.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (titleLabel);

    searchBox.setTextToShowWhenEmpty ("Search", AviatorTokens::textMuted());
    searchBox.setFont (AviatorTokens::hudBold (11.f));
    searchBox.setColour (juce::TextEditor::backgroundColourId, juce::Colour (0x40050d1a));
    searchBox.setColour (juce::TextEditor::outlineColourId, AviatorTokens::instrumentCyan().withAlpha (0.18f));
    searchBox.setColour (juce::TextEditor::textColourId, AviatorTokens::textPrimary());
    searchBox.setIndents (6, 5);
    searchBox.onTextChange = [this] { rebuildList(); };
    addAndMakeVisible (searchBox);

    presetList.setColour (juce::ListBox::backgroundColourId, juce::Colour (0x66101828));
    presetList.setColour (juce::ListBox::outlineColourId, juce::Colours::transparentBlack);
    presetList.setOutlineThickness (0);
    presetList.setRowHeight (30);
    addAndMakeVisible (presetList);

    refresh();
}

void AdvancedPresetSidebar::refresh()
{
    auto& pm = processorRef.getPresetManager();
    activeCategory = pm.getCurrentCategory();
    titleLabel.setText (activeCategory.toUpperCase() + " / PRESETS", juce::dontSendNotification);
    rebuildList();
}

void AdvancedPresetSidebar::rebuildList()
{
    presetNames.clear();
    auto& pm = processorRef.getPresetManager();
    const auto cat = activeCategory.isEmpty() ? pm.getCurrentCategory() : activeCategory;
    const auto query = searchBox.getText().trim().toLowerCase();

    for (const auto& name : pm.getPresetsForCategory (cat))
    {
        if (query.isEmpty() || name.toLowerCase().contains (query))
            presetNames.add (name);
    }

    presetList.updateContent();

    const int idx = presetNames.indexOf (pm.getCurrentPresetName());
    if (idx >= 0)
    {
        presetList.selectRow (idx);
        presetList.scrollToEnsureRowIsOnscreen (idx);
    }
}

int AdvancedPresetSidebar::getNumRows()
{
    return presetNames.size();
}

void AdvancedPresetSidebar::paintListBoxItem (int row, juce::Graphics& g, int w, int h, bool sel)
{
    if (! juce::isPositiveAndBelow (row, presetNames.size()))
        return;

    if (sel)
    {
        g.setColour (AviatorTokens::instrumentCyan().withAlpha (0.22f));
        g.fillRect (0, 0, w, h);
    }

    g.setFont (AviatorTokens::hudBold (12.f));
    g.setColour (sel ? AviatorTokens::textPrimary() : AviatorTokens::textMuted().brighter (0.35f));
    g.drawFittedText (presetNames[row], 8, 0, w - 12, h, juce::Justification::centredLeft, 1, 0.5f);
}

void AdvancedPresetSidebar::listBoxItemClicked (int row, const juce::MouseEvent&)
{
    if (! juce::isPositiveAndBelow (row, presetNames.size()))
        return;

    auto& pm = processorRef.getPresetManager();
    pm.loadPreset (activeCategory, presetNames[row]);

    if (onPresetChanged)
        onPresetChanged();
}

void AdvancedPresetSidebar::paint (juce::Graphics& g)
{
    g.setColour (juce::Colour (0xd8101828));
    g.fillRect (getLocalBounds());

    g.setColour (AviatorTokens::instrumentCyan().withAlpha (0.18f));
    g.drawVerticalLine (getWidth() - 1, 0.f, (float) getHeight());
}

void AdvancedPresetSidebar::resized()
{
    auto area = getLocalBounds().reduced (8);
    titleLabel.setBounds (area.removeFromTop (16));
    area.removeFromTop (4);
    searchBox.setBounds (area.removeFromTop (24));
    area.removeFromTop (4);
    presetList.setBounds (area);
}
