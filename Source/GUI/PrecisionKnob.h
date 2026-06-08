#pragma once

#include "Advanced/ModRoutingHub.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

/** Rotary control bound to one APVTS parameter (one attachment per param ID in the plugin). */
class PrecisionKnob : public juce::Component,
                      private juce::AudioProcessorValueTreeState::Listener
{
public:
    enum class ValueFormat
    {
        glideSeconds,
        percent,
        toneDb,
        envelopeMs
    };

    PrecisionKnob (juce::AudioProcessorValueTreeState& apvts,
                   const juce::String& paramID,
                   const juce::String& macroName,
                   const juce::String& sublabel,
                   ValueFormat format,
                   bool attachToParameter = true,
                   bool enableModAssignment = false);

    /** Refresh mod ring from APVTS (call after matrix edits). */
    void refreshModRing();

    ~PrecisionKnob() override;

    void paint (juce::Graphics& g) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;
    void mouseUp (const juce::MouseEvent& e) override;
    void mouseDoubleClick (const juce::MouseEvent& e) override;

    juce::String getValueText() const;

private:
    void parameterChanged (const juce::String& parameterID, float newValue) override;
    void syncFromSlider();
    void syncFromParameter();
    float getNormalisedValue() const;
    void paintModRing (juce::Graphics& g, float cx, float cy, float trackR, float scale) const;
    bool hitModRing (juce::Point<float> pt, float cx, float cy, float trackR, float scale) const;

    bool modAssignEnabled = false;
    ModRoutingHub::KnobModState modState;
    bool draggingModAmount = false;

    juce::AudioProcessorValueTreeState& apvtsRef;
    juce::String paramId;
    bool attachToParameter = true;
    bool updatingFromParameter = false;

    juce::Slider slider;
    juce::Label  nameLabel;
    juce::Label  subLabel;
    juce::Label  valueLabel;
    ValueFormat  valueFormat;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PrecisionKnob)
};
