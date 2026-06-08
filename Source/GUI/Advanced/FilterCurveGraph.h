#pragma once

#include "../AviatorTokens.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

/** Interactive log-scale filter response graph — drag handle for cutoff (X) and resonance (Y). */
class FilterCurveGraph : public juce::Component,
                         private juce::AudioProcessorValueTreeState::Listener
{
public:
    FilterCurveGraph (juce::AudioProcessorValueTreeState& apvts,
                      const char* cutoffParamId,
                      const char* resonanceParamId,
                      const char* typeParamId);

    ~FilterCurveGraph() override;

    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;
    void mouseUp (const juce::MouseEvent& e) override;

private:
    void parameterChanged (const juce::String& parameterID, float newValue) override;

    juce::Rectangle<float> plotArea() const;
    float readCutoffHz() const;
    float readResonance() const;
    int readFilterType() const;
    float resonanceToQ (float reso) const;
    float magnitudeDb (float freqHz, float cutoffHz, float Q, int filterType) const;
    float freqToX (float hz, juce::Rectangle<float> plot) const;
    float xToFreq (float x, juce::Rectangle<float> plot) const;
    float dbToY (float db, juce::Rectangle<float> plot) const;
    juce::Path buildResponsePath (juce::Rectangle<float> plot) const;
    juce::Point<float> handlePosition (juce::Rectangle<float> plot) const;
    void setCutoffFromFreq (float hz);
    void setResonanceNorm (float norm);
    void applyDrag (juce::Point<float> pos);
    juce::String formatCutoff (float hz) const;
    juce::String formatResonance (float reso) const;

    juce::AudioProcessorValueTreeState& apvtsRef;
    juce::String cutoffId;
    juce::String resonanceId;
    juce::String typeId;
    bool dragging { false };

    static constexpr float kMinHz  = 20.f;
    static constexpr float kMaxHz  = 20000.f;
    static constexpr float kMinDb  = -36.f;
    static constexpr float kMaxDb  = 12.f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FilterCurveGraph)
};
