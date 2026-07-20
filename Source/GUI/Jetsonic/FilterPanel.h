#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

// =============================================================================
//  FilterPanel — right cockpit glass display: FILTER response curve.
//  Curve reflects filter_cutoff / filter_resonance / filter_type.
//  Drag: X = cutoff (log), Y = resonance. Clicking the title toggles
//  filter_enabled. Values update on host automation and preset recall.
// =============================================================================

class FilterPanel : public juce::Component,
                    private juce::AudioProcessorValueTreeState::Listener,
                    private juce::AsyncUpdater
{
public:
    explicit FilterPanel (juce::AudioProcessorValueTreeState& apvts);
    ~FilterPanel() override;

    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;
    void mouseUp (const juce::MouseEvent& e) override;

private:
    void parameterChanged (const juce::String&, float) override { triggerAsyncUpdate(); }
    void handleAsyncUpdate() override { repaint(); }

    juce::Rectangle<float> graphArea() const;
    void applyDrag (juce::Point<float> pos);

    juce::AudioProcessorValueTreeState& apvtsRef;
    juce::RangedAudioParameter* cutoffParam { nullptr };
    juce::RangedAudioParameter* resoParam { nullptr };
    bool dragging { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FilterPanel)
};
