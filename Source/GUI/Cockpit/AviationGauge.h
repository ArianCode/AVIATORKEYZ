#pragma once

#include "../AviatorTokens.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

/** Aircraft-style circular gauge — gold needle, vertical drag, smooth animation. */
class AviationGauge : public juce::Component,
                      private juce::Timer,
                      private juce::AudioProcessorValueTreeState::Listener
{
public:
    enum class ValueFormat
    {
        percent,
        glideSeconds,
        toneDb,
        decibels,
        stereoWidth
    };

    AviationGauge (juce::AudioProcessorValueTreeState& apvts,
                   const juce::String& paramId,
                   const juce::String& gaugeName,
                   const juce::String& destinationHint,
                   ValueFormat format,
                   bool freezeToggleMode = false);

    ~AviationGauge() override;

    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;
    void mouseUp (const juce::MouseEvent& e) override;
    void mouseDoubleClick (const juce::MouseEvent& e) override;

private:
    void timerCallback() override;
    void parameterChanged (const juce::String& id, float) override;
    void syncNeedleTarget();
    float readNormalised() const;
    juce::String formatValueText() const;
    void showTextEditor();
    float needleAngleForNormalised (float norm) const;

    juce::AudioProcessorValueTreeState& apvtsRef;
    juce::String paramId;
    juce::String nameText;
    juce::String hintText;
    ValueFormat valueFormat;
    bool freezeToggle;

    float needleAngle { 0.f };
    float targetAngle { 0.f };
    float dragStartNorm { 0.f };
    int dragStartY { 0 };

    juce::Slider hiddenSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AviationGauge)
};
