#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class HeartIconButton : public juce::Button
{
public:
    HeartIconButton() : juce::Button ("Favorite") {}

    void setFavourited (bool fav) { favourited = fav; repaint(); }
    bool isFavourited() const { return favourited; }

    void paintButton (juce::Graphics& g, bool highlighted, bool down) override;

private:
    bool favourited { false };
};

class SavePresetButton : public juce::TextButton
{
public:
    SavePresetButton() : juce::TextButton ("SAVE") {}
};
