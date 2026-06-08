#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

/** Compact prev/next control — paints chevron (avoids JUCE truncating TextButton names to "p..."). */
class PresetNavButton : public juce::Button
{
public:
    enum class Direction { left, right, up, down };
    enum class Style { boxed, plainChevron };

    explicit PresetNavButton (Direction dir, Style style = Style::boxed);

    void paintButton (juce::Graphics& g,
                      bool highlighted,
                      bool down) override;

private:
    Direction direction;
    Style style;
};
