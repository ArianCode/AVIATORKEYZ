#pragma once

#include "../AviatorTokens.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <array>
#include <vector>

/** Animated grain particle display — hero visual for the Texture section. */
class TextureVisualizerComponent : public juce::Component,
                                   private juce::Timer
{
public:
    TextureVisualizerComponent();
    ~TextureVisualizerComponent() override;

    void setAmount (float a);
    void setFrozen (bool f);
    void setReverse (bool r);
    void setRate (float r);
    void setSize (float s);
    void setDensity (float d);
    void setSpread (float s);
    void setMotion (float m);
    void setDrift (float d);

    void paint (juce::Graphics& g) override;

private:
    struct Grain
    {
        float x { 0.f };
        float y { 0.f };
        float speed { 0.f };
        float size { 1.f };
        float alpha { 0.5f };
        float phase { 0.f };
        std::array<juce::Point<float>, 4> trail {};
    };

    void timerCallback() override;
    void paintBorder (juce::Graphics& g, juce::Rectangle<float> bounds, float sc) const;
    void paintInnerGlow (juce::Graphics& g, juce::Rectangle<float> bounds) const;
    void paintFrozenScan (juce::Graphics& g, juce::Rectangle<float> bounds) const;
    void paintParticles (juce::Graphics& g, juce::Rectangle<float> bounds) const;
    int activeParticleCount() const;

    std::vector<Grain> particles;
    int   time     { 0 };
    bool  frozen   { false };
    bool  reverse  { false };
    float amount   { 0.45f };
    float rate     { 0.55f };
    float grainSize { 0.4f };
    float density  { 0.7f };
    float spread   { 0.55f };
    float motion   { 0.35f };
    float drift    { 0.35f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TextureVisualizerComponent)
};
