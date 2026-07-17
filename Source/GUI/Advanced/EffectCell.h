#pragma once

#include "EffectCellFormat.h"
#include "EffectCellShell.h"
#include <juce_audio_processors/juce_audio_processors.h>

/**
 *  EffectCell — a large, self-explanatory control tile bound to one APVTS
 *  parameter. Drag anywhere on the tile to change the value.
 */
class EffectCell : public EffectCellShell
{
public:
    using Format = EffectCellFormat::Format;

    EffectCell (juce::AudioProcessorValueTreeState& apvts,
               const juce::String& paramID,
               const juce::String& title,
               const juce::String& dragHint,
               const juce::String& tooltip,
               Format format = Format::percent);

    void paint (juce::Graphics& g) override;
    void resized() override;

    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;
    void mouseUp (const juce::MouseEvent& e) override;
    void mouseDoubleClick (const juce::MouseEvent& e) override;

private:
    float getNormalisedValue() const;
    juce::String valueText() const;

    juce::AudioProcessorValueTreeState& apvtsRef;
    juce::String paramId;

    juce::Slider slider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    Format valueFormat;

    bool dragging { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EffectCell)
};
