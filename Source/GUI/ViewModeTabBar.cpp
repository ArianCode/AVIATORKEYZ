#include "ViewModeTabBar.h"

void ViewModeTabBar::ModeTabButton::paintButton (juce::Graphics& g, bool highlighted, bool down)
{
    juce::ignoreUnused (highlighted, down);
    auto bounds = getLocalBounds().toFloat();
    g.setColour (findColour (juce::TextButton::buttonColourId));
    g.fillRect (bounds);

    g.setFont (AviatorTokens::hudBold (13.f * AviatorTokens::scaleFor (*this)));
    g.setColour (findColour (juce::TextButton::textColourOffId));
    g.drawText (getButtonText(), bounds, juce::Justification::centred);
}

ViewModeTabBar::ViewModeTabBar()
{
    for (auto* tab : { &mainTab, &advancedTab })
    {
        tab->setColour (juce::TextButton::buttonColourId, juce::Colour (0xff0d1f33));
        tab->setClickingTogglesState (true);
        tab->setRadioGroupId (88001);
        addAndMakeVisible (tab);
    }

    mainTab.setToggleState (true, juce::dontSendNotification);
    styleTab (mainTab, true);
    styleTab (advancedTab, false);

    mainTab.onClick = [this]
    {
        setAdvancedSelected (false);
        if (onModeChanged)
            onModeChanged (false);
    };
    advancedTab.onClick = [this]
    {
        setAdvancedSelected (true);
        if (onModeChanged)
            onModeChanged (true);
    };
}

void ViewModeTabBar::setAdvancedSelected (bool advanced)
{
    advancedSelected = advanced;
    mainTab.setToggleState (! advanced, juce::dontSendNotification);
    advancedTab.setToggleState (advanced, juce::dontSendNotification);
    styleTab (mainTab, ! advanced);
    styleTab (advancedTab, advanced);
}

void ViewModeTabBar::styleTab (ModeTabButton& btn, bool active)
{
    btn.setColour (juce::TextButton::textColourOffId,
                   active ? AviatorTokens::instrumentCyan() : AviatorTokens::textMuted().brighter (0.15f));
    btn.setColour (juce::TextButton::buttonColourId,
                   active ? AviatorTokens::instrumentCyan().withAlpha (0.22f) : juce::Colour (0xff0d1f33));
}

void ViewModeTabBar::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff060c18));
    g.setColour (AviatorTokens::instrumentCyan().withAlpha (0.35f));
    g.drawHorizontalLine (getHeight() - 1, 0.f, (float) getWidth());
}

void ViewModeTabBar::resized()
{
    auto area = getLocalBounds().reduced (6, 5);
    const int tabW = (area.getWidth() - 4) / 2;
    mainTab.setBounds (area.removeFromLeft (tabW));
    area.removeFromLeft (4);
    advancedTab.setBounds (area);
}
