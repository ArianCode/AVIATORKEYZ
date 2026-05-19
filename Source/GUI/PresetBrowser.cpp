#include "PresetBrowser.h"
#include "../PluginProcessor.h"
#include "LuxuryLookAndFeel.h"

PresetBrowser::PresetBrowser (AviatorKeyzProcessor& proc,
                               std::function<void()> onApplied)
    : processor (proc)
    , onPresetApplied (std::move (onApplied))
{
    categoryLabel.setText ("Category", juce::dontSendNotification);
    categoryLabel.setColour (juce::Label::textColourId, LuxuryLookAndFeel::textDim());
    addAndMakeVisible (categoryLabel);

    presetLabel.setText ("Preset", juce::dontSendNotification);
    presetLabel.setColour (juce::Label::textColourId, LuxuryLookAndFeel::textDim());
    addAndMakeVisible (presetLabel);

    addAndMakeVisible (categoryBox);
    addAndMakeVisible (presetBox);

    int itemId = 1;
    for (const auto& c : processor.getPresetManager().getAllCategories())
        categoryBox.addItem (c, itemId++);

    categoryBox.addListener (this);
    presetBox.addListener (this);

    if (categoryBox.getNumItems() > 0)
        categoryBox.setSelectedId (1, juce::dontSendNotification);

    refreshPresetList();
    applySelectedPreset();
}

void PresetBrowser::refreshPresetList()
{
    presetBox.clear (juce::dontSendNotification);

    const auto cat = categoryBox.getText();
    int        itemId = 1;

    for (const auto& n : processor.getPresetManager().getPresetsForCategory (cat))
        presetBox.addItem (n, itemId++);

    if (presetBox.getNumItems() > 0)
        presetBox.setSelectedId (1, juce::dontSendNotification);
}

void PresetBrowser::comboBoxChanged (juce::ComboBox* box)
{
    if (box == &categoryBox)
    {
        refreshPresetList();
        applySelectedPreset();
    }
    else if (box == &presetBox)
    {
        applySelectedPreset();
    }
}

void PresetBrowser::applySelectedPreset()
{
    if (categoryBox.getText().isEmpty() || presetBox.getText().isEmpty())
        return;

    processor.getPresetManager().loadPreset (categoryBox.getText(), presetBox.getText());

    if (onPresetApplied != nullptr)
        onPresetApplied();
}

void PresetBrowser::resized()
{
    auto r = getLocalBounds().reduced (4, 0);
    const int labelH = 16;
    const int gap = 6;
    const int comboH = 28;

    auto row1 = r.removeFromTop (labelH + comboH + gap);
    categoryLabel.setBounds (row1.removeFromTop (labelH));
    categoryBox.setBounds (row1.removeFromTop (comboH));

    r.removeFromTop (4);

    auto row2 = r.removeFromTop (labelH + comboH);
    presetLabel.setBounds (row2.removeFromTop (labelH));
    presetBox.setBounds (row2.removeFromTop (comboH));
}
