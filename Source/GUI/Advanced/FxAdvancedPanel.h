#pragma once

#include "AdvancedKnobHelpers.h"
#include "../ReverseToggle.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <memory>
#include <vector>

class FxAdvancedPanel : public juce::Component
{
public:
    explicit FxAdvancedPanel (juce::AudioProcessorValueTreeState& apvts);

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    juce::Label routingTitle { {}, "FX ROUTING" };
    juce::Label reverbTitle { {}, "REVERB" };
    juce::Label delayTitle { {}, "DELAY" };
    juce::Label chorusTitle { {}, "CHORUS" };
    juce::Label lofiTitle { {}, "LO-FI" };
    juce::Label distTitle { {}, "DISTORT" };

    juce::ComboBox routingBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> routingAttach;

    std::unique_ptr<ReverseToggle> reverbOnToggle;
    std::unique_ptr<ReverseToggle> delayOnToggle;
    std::unique_ptr<ReverseToggle> chorusOnToggle;
    std::unique_ptr<ReverseToggle> lofiOnToggle;
    std::unique_ptr<ReverseToggle> distOnToggle;

    std::vector<std::unique_ptr<PrecisionKnob>> knobs;

    void addKnob (juce::AudioProcessorValueTreeState& apvts,
                  const char* id,
                  const juce::String& name,
                  const juce::String& sub,
                  PrecisionKnob::ValueFormat fmt = PrecisionKnob::ValueFormat::percent);
};
