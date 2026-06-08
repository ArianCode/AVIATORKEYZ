#pragma once

#include "PresetIconButtons.h"
#include "PresetNavButton.h"
#include "PresetDisplayUtils.h"
#include "../AviatorTokens.h"
#include <functional>

/** Analog Lab–style centered preset navigator over the cockpit horizon. */
class PresetCenterNavigator : public juce::Component
{
public:
    PresetCenterNavigator();

    std::function<void()> onPrevPreset;
    std::function<void()> onNextPreset;
    std::function<void (bool favourited)> onFavoriteToggled;

    void setPreset (const juce::String& category, const juce::String& fullPresetName);
    void setFavourited (bool favourited);

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    PresetNavButton prevBtn { PresetNavButton::Direction::left, PresetNavButton::Style::plainChevron };
    PresetNavButton nextBtn { PresetNavButton::Direction::right, PresetNavButton::Style::plainChevron };
    HeartIconButton favBtn;
    juce::String categoryText;
    juce::String displayName;
    juce::Rectangle<int> pillBounds;

    static constexpr int kMaxPillDesignW = 500;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetCenterNavigator)
};
