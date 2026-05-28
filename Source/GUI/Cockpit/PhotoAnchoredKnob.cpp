#include "PhotoAnchoredKnob.h"
#include "../AviatorTokens.h"

PhotoAnchoredKnob::PhotoAnchoredKnob (juce::AudioProcessorValueTreeState& apvts,
                                      const juce::String& paramID,
                                      PrecisionKnob::ValueFormat format)
    : inner (apvts, paramID, "", "", format)
{
    setOpaque (false);
    addAndMakeVisible (inner);
}

void PhotoAnchoredKnob::paint (juce::Graphics& g)
{
    if (isMouseOverOrDragging())
    {
        const auto r = getLocalBounds().toFloat().reduced (2.f);
        g.setColour (AviatorTokens::instrumentCyan().withAlpha (0.35f));
        g.drawEllipse (r, 1.5f);
    }
}

void PhotoAnchoredKnob::resized()
{
    inner.setBounds (getLocalBounds());
}
