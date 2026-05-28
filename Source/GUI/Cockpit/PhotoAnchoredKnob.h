#pragma once

#include "../PrecisionKnob.h"
#include <juce_audio_processors/juce_audio_processors.h>

/** Compact rotary bound to APVTS; labels hidden for photo-anchored hardware layer. */
class PhotoAnchoredKnob : public juce::Component
{
public:
    PhotoAnchoredKnob (juce::AudioProcessorValueTreeState& apvts,
                       const juce::String& paramID,
                       PrecisionKnob::ValueFormat format);

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    PrecisionKnob inner;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PhotoAnchoredKnob)
};
