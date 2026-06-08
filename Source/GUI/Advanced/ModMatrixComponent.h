#pragma once

#include "ModRoutingHub.h"
#include "../AviatorTokens.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>

/** Mod matrix — ON toggles + source/dest; depth set via knob double-click. */
class ModMatrixComponent : public juce::Component,
                           private juce::Timer
{
public:
    static constexpr int kNumRows = AviatorKeyz::ParamID::MOD_MATRIX_ROWS;

    explicit ModMatrixComponent (juce::AudioProcessorValueTreeState& apvts);

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    void timerCallback() override;
    void refreshDestLabels();

    juce::AudioProcessorValueTreeState& apvtsRef;

    struct MatrixRow
    {
        juce::TextButton onButton { "OFF" };
        juce::ComboBox sourceBox;
        juce::Label destLabel;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> sourceAttach;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> enableAttach;
    };

    juce::Label hintLabel;
    std::array<MatrixRow, kNumRows> rows;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ModMatrixComponent)
};
