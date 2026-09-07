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
//  Title click toggles env_enabled (LED); the ↺ glyph resets all four stages.
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
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseMove (const juce::MouseEvent& e) override;
    void mouseExit (const juce::MouseEvent&) override { hoverReset = false; repaint(); }

private:
    juce::Rectangle<int> resetArea() const { return { getWidth() - 20, 3, 14, 14 }; }
    void resetToDefaults();
    bool hoverReset { false };

    void parameterChanged (const juce::String&, float) override { triggerAsyncUpdate(); }
    void handleAsyncUpdate() override { repaint(); }

    static constexpr int kKnobRowH = 56;

    juce::AudioProcessorValueTreeState& apvtsRef;
    std::vector<std::unique_ptr<MiniRotary>> adsrKnobs;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EnvelopePanel)
};
