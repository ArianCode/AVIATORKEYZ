#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>

// =============================================================================
//  MiniControls — compact APVTS-bound widgets shared by the cockpit displays:
//    MiniRotary — small bronze rotary with cyan value arc (GLOBALS, A/D/S/R)
//    MiniParam  — glass value cell with vertical drag (LOFI / STEREO / ...)
//    MiniFader  — slim vertical level fader (LAYER MIX)
//    MiniToggle — small illuminated square button (enables, LIMITER)
//  All support host automation, preset recall, double-click default and
//  fine dragging with Cmd/Ctrl/Shift.
// =============================================================================

namespace JetsonicMini
{
    /** Slider subclass with modifier-based fine dragging. */
    class FineDragSlider : public juce::Slider
    {
    public:
        using juce::Slider::Slider;
        void mouseDown (const juce::MouseEvent& e) override
        {
            const bool fine = e.mods.isCommandDown() || e.mods.isCtrlDown() || e.mods.isShiftDown();
            setMouseDragSensitivity (fine ? 1400 : 220);
            juce::Slider::mouseDown (e);
        }
        void mouseEnter (const juce::MouseEvent& e) override
        {
            juce::Slider::mouseEnter (e);
            if (auto* p = getParentComponent()) p->repaint();
        }
        void mouseExit (const juce::MouseEvent& e) override
        {
            juce::Slider::mouseExit (e);
            if (auto* p = getParentComponent()) p->repaint();
        }
    };

    void configureAttachment (juce::AudioProcessorValueTreeState& apvts,
                              const juce::String& paramId,
                              juce::Slider& slider,
                              std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>& attachment);

    juce::String parameterText (juce::AudioProcessorValueTreeState& apvts, const juce::String& paramId);
    float parameterNorm (juce::AudioProcessorValueTreeState& apvts, const juce::String& paramId);
} // namespace JetsonicMini

// -----------------------------------------------------------------------------
class MiniRotary : public juce::Component
{
public:
    MiniRotary (juce::AudioProcessorValueTreeState& apvts,
                const juce::String& paramId,
                const juce::String& label,
                bool showValueText = true);

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    juce::AudioProcessorValueTreeState& apvtsRef;
    juce::String paramId, labelText;
    bool showValue;
    JetsonicMini::FineDragSlider slider { juce::Slider::RotaryVerticalDrag, juce::Slider::NoTextBox };
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MiniRotary)
};

// -----------------------------------------------------------------------------
class MiniParam : public juce::Component
{
public:
    MiniParam (juce::AudioProcessorValueTreeState& apvts,
               const juce::String& paramId,
               const juce::String& label,
               bool emphasized = false);

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    juce::AudioProcessorValueTreeState& apvtsRef;
    juce::String paramId, labelText;
    bool emphasize;
    JetsonicMini::FineDragSlider slider { juce::Slider::RotaryVerticalDrag, juce::Slider::NoTextBox };
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MiniParam)
};

// -----------------------------------------------------------------------------
class MiniFader : public juce::Component
{
public:
    MiniFader (juce::AudioProcessorValueTreeState& apvts,
               const juce::String& paramId,
               const juce::String& label);
    ~MiniFader() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    juce::AudioProcessorValueTreeState& apvtsRef;
    juce::String paramId, labelText;
    JetsonicMini::FineDragSlider slider { juce::Slider::LinearVertical, juce::Slider::NoTextBox };
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;

    class FaderLookAndFeel;
    std::unique_ptr<FaderLookAndFeel> lookAndFeel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MiniFader)
};

// -----------------------------------------------------------------------------
class MiniToggle : public juce::Component
{
public:
    MiniToggle (juce::AudioProcessorValueTreeState& apvts,
                const juce::String& paramId,
                const juce::String& label);

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    juce::AudioProcessorValueTreeState& apvtsRef;
    juce::String paramId, labelText;

    class SquareButton : public juce::Button
    {
    public:
        SquareButton() : juce::Button ({}) { setClickingTogglesState (true); }
        void paintButton (juce::Graphics& g, bool highlighted, bool down) override;
    };

    SquareButton button;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> attachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MiniToggle)
};
