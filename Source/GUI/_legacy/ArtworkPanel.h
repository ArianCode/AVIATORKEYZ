#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

struct PresetDisplayInfo
{
    juce::String categoryTag;
    juce::String heroName;
    juce::String subtitle;
    juce::String hudAlt;
    juce::String hudHdg;
    juce::String hudRoute;
};

class ArtworkPanel : public juce::Component,
                     private juce::Timer
{
public:
    ArtworkPanel();
    ~ArtworkPanel() override;

    void setPresetInfo (const PresetDisplayInfo& info);
    void triggerPresetFlash();

    std::function<void()> onPreviousPreset;
    std::function<void()> onNextPreset;

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    struct CityLight
    {
        float x, y, r, a;
        bool warm;
        float phase, speed;
    };

    struct Star
    {
        float x, y, r, a;
        bool warm;
        float phase, speed, drift;
    };

    struct Wisp
    {
        float x, y, rx, ry, a, drift;
    };

    void timerCallback() override;
    void rebuildSceneBuffer();
    void drawStaticScenery (juce::Graphics& g, juce::Rectangle<float> bounds);
    void drawTwinkleOverlay (juce::Graphics& g, juce::Rectangle<float> bounds);
    void buildScenery();

    static uint32_t seededRng (uint32_t& state);

    PresetDisplayInfo displayInfo;
    juce::TextButton  prevButton { "<" };
    juce::TextButton  nextButton { ">" };

    std::vector<CityLight> cityLights;
    std::vector<Star>      stars;
    std::vector<Wisp>      wisps;

    juce::Image sceneBuffer;
    bool sceneBufferValid = false;

    float artTime   = 0.f;
    float flashAlpha = 0.f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ArtworkPanel)
};
