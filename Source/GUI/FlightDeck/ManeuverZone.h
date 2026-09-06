#pragma once

#include "DeckWidgets.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>
#include <vector>

class AviatorKeyzProcessor;

// =============================================================================
//  ManeuverZone — MANEUVER · SAMPLE FLIP.
//  An arcade throw lever bound to `reverse` (click a side, drag, or ⇧-hold for
//  momentary), a mirrored playhead strip, FLIP WINDOW (phrase/slice/beat) and
//  the beat-snap selector.
// =============================================================================

class ManeuverZone : public juce::Component,
                     private juce::Timer
{
public:
    explicit ManeuverZone (AviatorKeyzProcessor& processor);
    ~ManeuverZone() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    class FlipLever;
    class MirrorStrip;

    void timerCallback() override;

    AviatorKeyzProcessor& processorRef;
    juce::AudioProcessorValueTreeState& apvts;

    std::unique_ptr<FlipLever> lever;
    std::unique_ptr<MirrorStrip> mirror;
    std::unique_ptr<DeckSegment> windowSeg, snapSeg;
    std::unique_ptr<juce::ParameterAttachment> reverseAttachment;
    bool reversed { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ManeuverZone)
};
