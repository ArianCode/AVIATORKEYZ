#pragma once

#include "DeckWidgets.h"
#include "../Aviation/MenuLookAndFeel.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>
#include <vector>

class AviatorKeyzProcessor;

// =============================================================================
//  MfxSlotPanel — one MFX slot ("MFX / Slot Concept").
//
//    header   : power · prev/next · effect selector (categorised menu) · PRESET
//               · REV SEND · LEVEL
//    generator: REROLL · SURPRISE ME · AMOUNT · UNDO · IDs
//    bank     : 16 generic parameter columns relabelled per effect, lockable
//    assigns  : 4 × (source, sens) modulating the descriptor's assign targets
//
//  Wide panels lay the bank out as 16 columns; narrow ones as 2 rows of 8.
// =============================================================================

class MfxSlotPanel : public juce::Component,
                     private juce::Timer
{
public:
    MfxSlotPanel (AviatorKeyzProcessor& processor, int slotIndex);
    ~MfxSlotPanel() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    class ParamColumn;
    class AssignCell;
    class HeaderButton;

    void timerCallback() override;
    void effectChanged();
    void openEffectMenu();
    void openPresetMenu();
    void stepEffect (int delta);
    void reroll();
    void surprise();
    void undo();
    void rebuildColumns();
    juce::Rectangle<int> headerArea() const;
    juce::Rectangle<int> generatorArea() const;
    juce::Rectangle<int> bankArea() const;
    juce::Rectangle<int> assignsArea() const;
    bool wide() const { return getWidth() >= 700; }

    AviatorKeyzProcessor& processorRef;
    juce::AudioProcessorValueTreeState& apvts;
    const int slot;

    std::unique_ptr<DeckPad> powerPad;
    std::unique_ptr<HeaderButton> prevBtn, nextBtn, selectorBtn, presetBtn,
                                  rerollBtn, surpriseBtn, undoBtn, idsBtn;
    std::unique_ptr<DeckKnob> sendKnob, levelKnob;
    juce::Slider amountSlider { juce::Slider::LinearHorizontal, juce::Slider::NoTextBox };
    std::vector<std::unique_ptr<ParamColumn>> columns;
    std::vector<std::unique_ptr<AssignCell>> assigns;

    std::unique_ptr<juce::ParameterAttachment> effectAttachment;
    int currentEffect { 0 };
    bool showIds { false };
    juce::Random rng;
    MenuLookAndFeel menuLookAndFeel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MfxSlotPanel)
};
