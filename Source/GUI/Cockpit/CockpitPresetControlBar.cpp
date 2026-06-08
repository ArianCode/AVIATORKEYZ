#include "CockpitPresetControlBar.h"
#include "../AviatorTokens.h"

CockpitPresetControlBar::CockpitPresetControlBar()
{
    addAndMakeVisible (browseBtn);
    addAndMakeVisible (prevBtn);
    addAndMakeVisible (nextBtn);
    addAndMakeVisible (favBtn);
    addAndMakeVisible (saveBtn);
    addAndMakeVisible (presetName);

    browseBtn.onClick = [this] { if (onBrowseRequested) onBrowseRequested(); };
    prevBtn.onClick = [this] { if (onPrevPreset) onPrevPreset(); };
    nextBtn.onClick = [this] { if (onNextPreset) onNextPreset(); };
    favBtn.onClick = [this] {
        favBtn.setFavourited (! favBtn.isFavourited());
        if (onFavoriteToggled)
            onFavoriteToggled (favBtn.isFavourited());
    };
    saveBtn.onClick = [this] { if (onSaveRequested) onSaveRequested(); };

    presetName.setJustificationType (juce::Justification::centred);
    presetName.setColour (juce::Label::textColourId, AviatorTokens::textPrimary());
    presetName.setMinimumHorizontalScale (0.5f);

    saveBtn.setColour (juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    saveBtn.setColour (juce::TextButton::textColourOffId, AviatorTokens::instrumentCyan().withAlpha (0.9f));
}

void CockpitPresetControlBar::setPresetName (const juce::String& fullPresetName)
{
    presetName.setText (fullPresetName, juce::dontSendNotification);
}

void CockpitPresetControlBar::setFavourited (bool favourited)
{
    favBtn.setFavourited (favourited);
}

void CockpitPresetControlBar::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff050a14));
    g.setColour (AviatorTokens::instrumentCyan().withAlpha (0.22f));
    g.drawHorizontalLine (getHeight() - 1, 0.f, (float) getWidth());
}

void CockpitPresetControlBar::resized()
{
    const float sc = AviatorTokens::scaleFor (*this);
    auto area = getLocalBounds().reduced (8, 4);
    const int iconW = AviatorTokens::scaledFor (*this, 26);
    const int arrowW = AviatorTokens::scaledFor (*this, 22);
    const int saveW = AviatorTokens::scaledFor (*this, 52);

    browseBtn.setBounds (area.removeFromLeft (iconW));
    area.removeFromLeft (6);

    auto right = area.removeFromRight (saveW + iconW + 10);
    saveBtn.setBounds (right.removeFromRight (saveW));
    right.removeFromRight (6);
    favBtn.setBounds (right.removeFromRight (iconW));

    prevBtn.setBounds (area.removeFromLeft (arrowW));
    area.removeFromLeft (4);
    nextBtn.setBounds (area.removeFromRight (arrowW));
    area.removeFromRight (4);
    presetName.setBounds (area);

    presetName.setFont (AviatorTokens::hudBold (13.f * sc));
    saveBtn.setButtonText ("SAVE");
}
