#pragma once

#include "MiniControls.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>
#include <vector>

// =============================================================================
//  EnvelopePanel — right cockpit glass display: amplitude ENVELOPE.
//  ADSR shape drawn from env_attack / env_amp_decay / env_amp_sustain /
//  env_release, with four small A D S R rotaries beneath the graph.
// =============================================================================

class EnvelopePanel : public juce::Component,
                      private juce::AudioProcessorValueTreeState::Listener,
                      private juce::AsyncUpdater
{
public:
    explicit EnvelopePanel (juce::AudioProcessorValueTreeState& apvts);
    ~EnvelopePanel() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    void parameterChanged (const juce::String&, float) override { triggerAsyncUpdate(); }
    void handleAsyncUpdate() override { repaint(); }

    juce::AudioProcessorValueTreeState& apvtsRef;
    std::vector<std::unique_ptr<MiniRotary>> adsrKnobs;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EnvelopePanel)
};
