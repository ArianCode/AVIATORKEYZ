#pragma once

#include "../AviatorTokens.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

/** Vital-style bracket corner value box — vertical drag, double-click to type. */
class BracketValueBox : public juce::Component,
                        private juce::AudioProcessorValueTreeState::Listener
{
public:
    enum class Format
    {
        percent,
        hz,
        ms,
        semitones,
        cents,
        pan,
        cutoff,
        plain,
        decibels,
        integer
    };

    BracketValueBox (juce::AudioProcessorValueTreeState& apvts,
                     const juce::String& paramId,
                     const juce::String& label,
                     Format format,
                     juce::Colour bracketColour);

    ~BracketValueBox() override;

    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;
    void mouseUp (const juce::MouseEvent& e) override;
    void mouseDoubleClick (const juce::MouseEvent& e) override;

    static constexpr int kDefaultW = 58;
    static constexpr int kDefaultH = 50;
    static constexpr int kCompactW = 48;
    static constexpr int kCompactH = 42;

private:
    void parameterChanged (const juce::String& parameterID, float newValue) override;
    void syncFromParam();
    juce::String formatValue() const;
    void paintBrackets (juce::Graphics& g, juce::Rectangle<float> b) const;
    void showTextEditor();

    juce::AudioProcessorValueTreeState& apvtsRef;
    juce::String paramId;
    juce::String labelText;
    Format valueFormat;
    juce::Colour bracketColour;

    float dragStartValue { 0.f };
    int dragStartY { 0 };

    juce::Slider hiddenSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BracketValueBox)
};
