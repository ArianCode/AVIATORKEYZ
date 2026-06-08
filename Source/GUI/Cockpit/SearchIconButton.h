#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

/** Compact search icon button for preset bar. */
class SearchIconButton : public juce::Button
{
public:
    SearchIconButton() : juce::Button ("Search") {}

    void paintButton (juce::Graphics& g, bool highlighted, bool down) override;
};
