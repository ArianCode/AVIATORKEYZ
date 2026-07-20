#pragma once

#include "MiniControls.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>
#include <vector>

// =============================================================================
//  LayerMixPanel — left cockpit glass display: LAYER MIX.
//  Slim vertical level faders for the real layer levels (osc1/osc2 levels,
//  texture amount, grain mix) plus illuminated enable squares for the
//  texture engines.
// =============================================================================

class LayerMixPanel : public juce::Component
{
public:
    explicit LayerMixPanel (juce::AudioProcessorValueTreeState& apvts);

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    juce::AudioProcessorValueTreeState& apvtsRef;
    std::vector<std::unique_ptr<MiniFader>> faders;
    std::unique_ptr<MiniToggle> texToggle, grainToggle;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LayerMixPanel)
};
