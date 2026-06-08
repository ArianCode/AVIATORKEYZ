#pragma once

#include "FilterCurveGraph.h"
#include "AdvancedWidgets.h"
#include "BracketValueBox.h"
#include "LfoWaveformDisplay.h"
#include "ModAmountSlider.h"
#include "ModRoutingHub.h"
#include "OscWaveformDisplay.h"
#include "PerformanceMacroStrip.h"
#include "TextureSectionComponent.h"
#include "../../DSP/ModMatrix.h"
#include "../../State/StateSchema.h"
#include "../AviatorTokens.h"
#include <array>
#include <memory>
#include <unordered_map>
#include <vector>

/** Fixed-layout Advanced page — no scroll, scales to fill parent bounds. */
class AdvancedPageContent : public juce::Component,
                            private juce::Timer
{
public:
    static constexpr int kDesignWidth  = 1366;
    static constexpr int kDesignHeight = 860;

    explicit AdvancedPageContent (juce::AudioProcessorValueTreeState& apvts);

    void paint (juce::Graphics& g) override;
    void resized() override;
    void mouseMove (const juce::MouseEvent& e) override;
    void mouseDown (const juce::MouseEvent& e) override;

private:
    using BV  = BracketValueBox;
    using Fmt = BracketValueBox::Format;

    struct ModRow
    {
        juce::TextButton onBtn { "OFF" };
        juce::TextButton deleteBtn { "X" };
        juce::ComboBox sourceBox { "mod_src" };
        juce::ComboBox destBox { "mod_dst" };
        juce::Label sourceLabel { {}, {} };
        juce::Label destLabel { {}, {} };
        std::unique_ptr<ModAmountSlider> amountSlider;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> srcA;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> dstA;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> onA;
        juce::Rectangle<int> rowBounds;
    };

    enum class Section
    {
        textureEngine,
        matrix,
        fxRouting,
        utility
    };

    bool isModRowActive (int rowIndex) const;
    void setSection (Section section);
    void styleSectionTab (juce::TextButton& btn, bool active) const;
    void layoutModMatrixZone (juce::Rectangle<int> modArea, int rowHpx);
    void layoutModulatorFocus (juce::Rectangle<int> focusArea, const std::function<int (int)>& rh);
    void selectModSource (int sourceIndex);
    int modChoiceIndex (const char* paramId) const;

    struct Region
    {
        juce::String title;
        juce::Rectangle<int> bounds;
    };

    void addBox (const char* id, const juce::String& label, Fmt fmt);
    BV* box (const char* id) const;
    void placeBox (BV* b, juce::Rectangle<int> r) const;
    void placeRow (juce::Rectangle<int> row, const std::vector<BV*>& boxes, int boxW, int gap) const;
    void styleModOnBtn (juce::TextButton& btn, bool on) const;
    void timerCallback() override;

    juce::AudioProcessorValueTreeState& apvtsRef;
    std::vector<std::unique_ptr<BV>> allBoxes;
    std::unordered_map<std::string, BV*> boxMap;
    std::vector<Region> regions;

    std::unique_ptr<OscWaveformDisplay> osc1Wave;
    std::unique_ptr<OscWaveformDisplay> osc2Wave;
    std::unique_ptr<AdvancedWidgets::ChoiceToggleRow> osc1Types;
    std::unique_ptr<AdvancedWidgets::ChoiceToggleRow> osc2Types;

    std::unique_ptr<FilterCurveGraph> filterCurve;

    std::unique_ptr<LfoWaveformDisplay> lfo1Wave;
    std::unique_ptr<LfoWaveformDisplay> lfo2Wave;
    std::unique_ptr<LfoWaveformDisplay> lfo3Wave;
    juce::ComboBox lfo1Shape { "lfo1_shape" };
    juce::ComboBox lfo2Shape { "lfo2_shape" };
    juce::ComboBox lfo3Shape { "lfo3_shape" };
    std::unique_ptr<AdvancedWidgets::FlatToggle> lfo1Sync;
    std::unique_ptr<AdvancedWidgets::FlatToggle> lfo2Sync;
    std::unique_ptr<AdvancedWidgets::FlatToggle> lfo3Sync;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> lfo1ShapeA;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> lfo2ShapeA;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> lfo3ShapeA;

    std::unique_ptr<TextureSectionComponent> textureSection;
    std::unique_ptr<PerformanceMacroStrip> performanceMacros;

    juce::Label modHdrOn { {}, "ON" };
    juce::Label modHdrRoute { {}, "ROUTE" };
    juce::Label modHdrAmt { {}, "AMT" };
    std::array<ModRow, ModRoutingHub::kNumRows> modRows {};
    juce::TextButton addModBtn { "+ Add Modulation" };
    int selectedModSourceIndex { 1 };
    int hoveredModRowIndex { -1 };

    juce::TextButton tabTexture { "TEXTURE ENGINE" };
    juce::TextButton tabMatrix { "MATRIX" };
    juce::TextButton tabFx { "FX ROUTING" };
    juce::TextButton tabUtility { "UTILITY / GLOBAL" };
    Section activeSection { Section::textureEngine };

    std::unique_ptr<AdvancedWidgets::FlatToggle> outLimiter;
    std::unique_ptr<AdvancedWidgets::ChoiceToggleRow> voiceMode;
    std::unique_ptr<BV> voiceGlideBox;

    std::unique_ptr<AdvancedWidgets::FlatToggle> phraseEnabled;
    std::unique_ptr<AdvancedWidgets::FlatToggle> phraseTempoSync;
    std::unique_ptr<AdvancedWidgets::FlatToggle> phraseKeySync;
    std::unique_ptr<AdvancedWidgets::FlatToggle> phraseLoop;
    juce::TextButton phraseOneShot { "ONE SHOT" };

    std::unique_ptr<AdvancedWidgets::FxEnableButton> fxReverbOn;
    std::unique_ptr<AdvancedWidgets::FxEnableButton> fxDelayOn;
    std::unique_ptr<AdvancedWidgets::FxEnableButton> fxChorusOn;
    std::unique_ptr<AdvancedWidgets::FxEnableButton> fxLofiOn;
    std::unique_ptr<AdvancedWidgets::FxEnableButton> fxDistOn;
    std::unique_ptr<AdvancedWidgets::FlatToggle> fxDelaySync;
    std::unique_ptr<AdvancedWidgets::FlatToggle> fxEditsToggle;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AdvancedPageContent)
};
