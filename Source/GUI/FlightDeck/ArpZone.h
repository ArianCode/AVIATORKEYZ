#pragma once

#include "DeckWidgets.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>
#include <vector>

class AviatorKeyzProcessor;

// =============================================================================
//  ArpZone — ARPEGGIATOR zone of the Flight Deck.
//    step visualizer (live from the audio-thread arp state)
//    5 mode pads + HOLD pad + ENGAGE pad
//    RATE / FEEL segments, OCTAVES stepper, GATE / SWING / HUMANIZE / OCT SPREAD
//    knobs, ARP TARGET (SLICES / NOTES)
// =============================================================================

class ArpZone : public juce::Component
{
public:
    explicit ArpZone (AviatorKeyzProcessor& processor);
    ~ArpZone() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    class StepViz;

    AviatorKeyzProcessor& processorRef;
    juce::AudioProcessorValueTreeState& apvts;

    std::unique_ptr<StepViz> viz;
    std::unique_ptr<ModePads> modePads;
    std::unique_ptr<DeckPad> holdPad, engagePad;
    std::unique_ptr<DeckSegment> rateSeg, feelSeg, targetSeg;
    std::unique_ptr<OctaveStepper> octaves;
    std::vector<std::unique_ptr<DeckKnob>> knobs;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ArpZone)
};
