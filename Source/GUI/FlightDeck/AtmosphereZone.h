#pragma once

#include "DeckWidgets.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>
#include <vector>

class AviatorKeyzProcessor;

// =============================================================================
//  AtmosphereZone — granular texture (ptex_*) + CABIN FX post-chain sends.
//  Re-skin of the existing texture engine controls with a live grain-cloud
//  visualizer driven by the texture parameters and the post-pipeline level.
// =============================================================================

class AtmosphereZone : public juce::Component
{
public:
    explicit AtmosphereZone (AviatorKeyzProcessor& processor);
    ~AtmosphereZone() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    class GrainCloud;

    AviatorKeyzProcessor& processorRef;
    juce::AudioProcessorValueTreeState& apvts;

    std::unique_ptr<GrainCloud> cloud;
    std::unique_ptr<DeckPad> texturePad, freezePad;
    std::vector<std::unique_ptr<DeckKnob>> textureKnobs, fxKnobs;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AtmosphereZone)
};
