#pragma once

#include "HeaderBar.h"
#include "KnobComponent.h"
#include "PresetBrowser.h"
#include "WaveformDisplay.h"
#include <juce_gui_basics/juce_gui_basics.h>

class AviatorKeyzProcessor;

class MainPanel : public juce::Component
{
public:
    explicit MainPanel (AviatorKeyzProcessor& processor);
    ~MainPanel() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    void syncHeaderFromPresetManager();

    AviatorKeyzProcessor& processor;

    std::unique_ptr<juce::LookAndFeel> luxuryLookAndFeel;

    HeaderBar       headerBar;
    PresetBrowser   presetBrowser;
    WaveformDisplay waveformDisplay;

    KnobComponent knobInputGain;
    KnobComponent knobOutputGain;
    KnobComponent knobGlide;
    KnobComponent knobSmear;
    KnobComponent knobTone;
    KnobComponent knobReverbAmt;
    KnobComponent knobReverbSize;
    KnobComponent knobWidth;
    KnobComponent knobAttack;
    KnobComponent knobRelease;
    KnobComponent knobPan;

    juce::ToggleButton reverseButton;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> reverseAttachment;

    juce::TextButton saveUserPresetButton { "Save user preset…" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainPanel)
};
