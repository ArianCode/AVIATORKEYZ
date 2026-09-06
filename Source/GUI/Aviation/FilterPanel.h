#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

// =============================================================================
//  FilterPanel — right cockpit glass display: FILTER response curve.
//  Curve reflects filter_cutoff / filter_resonance / filter_type.
//  Drag: X = cutoff (log), Y = resonance — dragging switches the filter ON.
//  HP / LP chips in the lower corners pick the mode; clicking the title
//  toggles filter_enabled. Values update on host automation and preset recall.
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
    void mouseMove (const juce::MouseEvent& e) override;
    void mouseExit (const juce::MouseEvent& e) override;

private:
    void parameterChanged (const juce::String&, float) override { triggerAsyncUpdate(); }
    void handleAsyncUpdate() override { repaint(); }

    juce::Rectangle<float> graphArea() const;
    juce::Rectangle<int> hpChipArea() const;
    juce::Rectangle<int> lpChipArea() const;
    void applyDrag (juce::Point<float> pos);
    void setEnabledParam (bool on);
    void setTypeParam (int typeIndex);

    juce::AudioProcessorValueTreeState& apvtsRef;
    juce::RangedAudioParameter* cutoffParam { nullptr };
    juce::RangedAudioParameter* resoParam { nullptr };
    bool dragging { false };
    int hoveredChip { -1 }; // 0 = HP, 1 = LP

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FilterPanel)
};
