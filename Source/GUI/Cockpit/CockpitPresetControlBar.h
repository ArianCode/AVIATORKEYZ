#pragma once

#include "PresetIconButtons.h"
#include "PresetNavButton.h"
#include "SearchIconButton.h"
#include <functional>

/** Slim preset quick-control bar — library, name + arrows, favorite + save. */
class CockpitPresetControlBar : public juce::Component
{
public:
    CockpitPresetControlBar();

    std::function<void()> onBrowseRequested;
    std::function<void()> onPrevPreset;
    std::function<void()> onNextPreset;
    std::function<void (bool favourited)> onFavoriteToggled;
    std::function<void()> onSaveRequested;

    void setPresetName (const juce::String& fullPresetName);
    void setFavourited (bool favourited);

    void paint (juce::Graphics& g) override;
    void resized() override;

    static constexpr int kDesignHeight = 34;

private:
    SearchIconButton browseBtn;
    PresetNavButton prevBtn { PresetNavButton::Direction::up, PresetNavButton::Style::plainChevron };
    PresetNavButton nextBtn { PresetNavButton::Direction::down, PresetNavButton::Style::plainChevron };
    HeartIconButton favBtn;
    SavePresetButton saveBtn;
    juce::Label presetName { "presetName", {} };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CockpitPresetControlBar)
};
