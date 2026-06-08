#pragma once

#include "AdvancedKnobHelpers.h"
#include "LfoWaveformDisplay.h"
#include "../../State/StateSchema.h"
#include <array>
#include <memory>

class LfoPanelComponent : public juce::Component
{
public:
    explicit LfoPanelComponent (juce::AudioProcessorValueTreeState& apvts);

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    struct LfoIds { const char* rate, *depth, *shape, *sync, *phase; const char* label; };

    static constexpr LfoIds kLfos[3] {
        { AviatorKeyz::ParamID::LFO1_RATE, AviatorKeyz::ParamID::LFO1_DEPTH,
          AviatorKeyz::ParamID::LFO1_SHAPE, AviatorKeyz::ParamID::LFO1_SYNC, AviatorKeyz::ParamID::LFO1_PHASE, "LFO 1" },
        { AviatorKeyz::ParamID::LFO2_RATE, AviatorKeyz::ParamID::LFO2_DEPTH,
          AviatorKeyz::ParamID::LFO2_SHAPE, AviatorKeyz::ParamID::LFO2_SYNC, AviatorKeyz::ParamID::LFO2_PHASE, "LFO 2" },
        { AviatorKeyz::ParamID::LFO3_RATE, AviatorKeyz::ParamID::LFO3_DEPTH,
          AviatorKeyz::ParamID::LFO3_SHAPE, AviatorKeyz::ParamID::LFO3_SYNC, AviatorKeyz::ParamID::LFO3_PHASE, "LFO 3" },
    };

    struct LfoStrip
    {
        juce::Label title;
        std::unique_ptr<LfoWaveformDisplay> waveform;
        std::unique_ptr<PrecisionKnob> rateKnob;
        std::unique_ptr<PrecisionKnob> depthKnob;
        std::unique_ptr<PrecisionKnob> phaseKnob;
        std::unique_ptr<juce::ComboBox> shapeBox;
        juce::ToggleButton syncButton;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> shapeAttach;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> syncAttach;
    };

    std::array<LfoStrip, 3> strips;
};
