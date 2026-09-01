#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

// =============================================================================
//  PresetHeader — row under the app header:
//  "FACTORY PRESETS" label (left), prev/next arrows around the centered
//  active preset name, favorite heart to the right.
// =============================================================================

class PresetHeader : public juce::Component
{
public:
    PresetHeader();

    std::function<void()> onPrevPreset;
    std::function<void()> onNextPreset;
    std::function<void()> onPresetNameClicked;
    std::function<void (bool)> onFavoriteToggled;

    void setPresetName (const juce::String& name);
    void setFavourited (bool fav);

    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseMove (const juce::MouseEvent& e) override;
    void mouseExit (const juce::MouseEvent& e) override;

private:
    enum class Hit { none, prev, next, name, heart };
    Hit hitAt (juce::Point<int> pos) const;
    juce::Rectangle<int> prevArea() const;
    juce::Rectangle<int> nextArea() const;
    juce::Rectangle<int> nameArea() const;
    juce::Rectangle<int> heartArea() const;

    juce::String presetName;
    bool favourited { false };
    Hit hovered { Hit::none };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetHeader)
};
