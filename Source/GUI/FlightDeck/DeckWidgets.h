#pragma once

#include "../Aviation/AviationTheme.h"
#include "../Aviation/MiniControls.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>
#include <memory>
#include <utility>
#include <vector>

// =============================================================================
//  DeckWidgets — FLIGHT DECK (performance view) building blocks.
//
//  The Flight Deck is laid out on a fixed 1366 x 860 canvas that the editor
//  scales as one unit (same rule as the Aviation MAIN view): coordinates in
//  these files are literal prototype pixels.
//
//    Deck::paintZone   — recessed zone panel with gold title + mono tag
//    Deck::paintScreen — dark instrument screen (visualizers)
//    DeckKnob          — APVTS rotary with arc, pointer, label, value, optional enable LED
//    DeckSegment       — segmented switch for choice / bool params (value-mapped)
//    DeckPad           — arcade toggle pad with LED (HOLD / FREEZE / ENGAGE)
//    ModePads          — five arp-mode pads with direction glyphs
//    OctaveStepper     — [−] 2 OCT [+] integer stepper
//    DeckChip          — small mono readout chip, optionally clickable / draggable
// =============================================================================

namespace Deck
{
    static constexpr int kDesignW = 1366;
    static constexpr int kDesignH = 860;
    static constexpr int kZoneHeaderH = 34;

    inline juce::Colour warn()        { return juce::Colour (0xffff6a4d); }
    inline juce::Colour warnBright()  { return juce::Colour (0xffffb3a2); }
    inline juce::Colour green()       { return Aviation::activeGreen(); }
    inline juce::Colour zoneBorder()  { return juce::Colour (0xff16283a); }
    inline juce::Colour screenBg()    { return juce::Colour (0xff04101a); }
    inline juce::Colour screenLine()  { return juce::Colour (0xff0d1d2c); }
    inline juce::Colour padBorder()   { return juce::Colour (0xff1d3347); }
    inline juce::Colour segBorder()   { return juce::Colour (0xff1a2e40); }
    inline juce::Colour segOnBg()     { return juce::Colour (0xff12293b); }

    inline juce::Font mono (float size)
    {
        auto f = juce::Font (juce::FontOptions ("Menlo", size, juce::Font::plain));
        f.setExtraKerningFactor (0.08f);
        return f;
    }

    /** Zone panel: gradient body, border, header rule, title (gold) + tag (mono). */
    void paintZone (juce::Graphics& g, juce::Rectangle<int> bounds,
                    const juce::String& title, const juce::String& tag);

    /** Recessed instrument screen with faint border. */
    void paintScreen (juce::Graphics& g, juce::Rectangle<float> r);

    /** Arcade pad face (raised / pressed) with optional accent border. */
    void paintPad (juce::Graphics& g, juce::Rectangle<float> r, bool lit,
                   juce::Colour accent, bool hover, bool greenTint = false);

    juce::String noteName (int midiNote);
} // namespace Deck

// -----------------------------------------------------------------------------
class DeckKnob : public juce::Component
{
public:
    enum class Format { percent, raw, ms, semitones, decibels, text };

    DeckKnob (juce::AudioProcessorValueTreeState& apvts,
              const juce::String& paramId,
              const juce::String& label,
              int diameter,
              Format format = Format::percent,
              bool gold = false,
              const juce::String& enableParamId = {});
    ~DeckKnob() override;

    void paint (juce::Graphics& g) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent& e) override;

    /** Preferred total size for a knob of this diameter (label + value below). */
    static int preferredWidth (int diameter) { return juce::jmax (52, diameter + 16); }
    static int preferredHeight (int diameter) { return diameter + 30; }

private:
    juce::String valueText() const;
    juce::Rectangle<int> ledArea() const;

    juce::AudioProcessorValueTreeState& apvtsRef;
    juce::String paramId, labelText, enableId;
    int diameter;
    Format format;
    bool isGold;

    AviationMini::FineDragSlider slider { juce::Slider::RotaryVerticalDrag, juce::Slider::NoTextBox };
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    std::unique_ptr<juce::ParameterAttachment> enableAttachment;
    bool enabledState { true };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DeckKnob)
};

// -----------------------------------------------------------------------------
class DeckSegment : public juce::Component
{
public:
    using Option = std::pair<juce::String, int>; // label -> denormalised param value

    DeckSegment (juce::AudioProcessorValueTreeState& apvts,
                 const juce::String& paramId,
                 std::vector<Option> options,
                 const juce::String& caption = {});

    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseMove (const juce::MouseEvent& e) override;
    void mouseExit (const juce::MouseEvent&) override { hovered = -1; repaint(); }

    /** Width needed for the segments at the given per-cell padding. */
    int preferredWidth (int cellPadding = 10) const;
    static constexpr int kSegH = 24;
    static constexpr int kCaptionH = 14;

private:
    int cellAt (juce::Point<int> p) const;
    juce::Rectangle<int> segArea() const;

    juce::AudioProcessorValueTreeState& apvtsRef;
    juce::RangedAudioParameter* param { nullptr };
    std::vector<Option> options;
    juce::String caption;
    std::unique_ptr<juce::ParameterAttachment> attachment;
    int currentValue { 0 };
    int hovered { -1 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DeckSegment)
};

// -----------------------------------------------------------------------------
class DeckPad : public juce::Component
{
public:
    DeckPad (juce::AudioProcessorValueTreeState& apvts,
             const juce::String& paramId,
             const juce::String& label,
             const juce::String& sublabel,
             juce::Colour litColour,
             bool momentaryOnShift = false);

    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseUp (const juce::MouseEvent& e) override;
    void mouseEnter (const juce::MouseEvent&) override { hover = true; repaint(); }
    void mouseExit (const juce::MouseEvent&) override { hover = false; repaint(); }

    bool isLit() const noexcept { return lit; }

private:
    juce::AudioProcessorValueTreeState& apvtsRef;
    juce::RangedAudioParameter* param { nullptr };
    juce::String labelText, subText;
    juce::Colour accent;
    bool momentary;
    std::unique_ptr<juce::ParameterAttachment> attachment;
    bool lit { false };
    bool hover { false };
    bool momentaryActive { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DeckPad)
};

// -----------------------------------------------------------------------------
class ModePads : public juce::Component
{
public:
    ModePads (juce::AudioProcessorValueTreeState& apvts, const juce::String& paramId);

    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseMove (const juce::MouseEvent& e) override;
    void mouseExit (const juce::MouseEvent&) override { hovered = -1; repaint(); }

private:
    juce::Rectangle<int> padArea (int index) const;
    static juce::Path glyphFor (int mode);

    juce::AudioProcessorValueTreeState& apvtsRef;
    juce::RangedAudioParameter* param { nullptr };
    std::unique_ptr<juce::ParameterAttachment> attachment;
    int current { 0 };
    int hovered { -1 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ModePads)
};

// -----------------------------------------------------------------------------
class OctaveStepper : public juce::Component
{
public:
    OctaveStepper (juce::AudioProcessorValueTreeState& apvts, const juce::String& paramId,
                   const juce::String& suffix = " OCT");

    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent& e) override;
    static constexpr int kW = 86;
    static constexpr int kH = 26;

private:
    juce::AudioProcessorValueTreeState& apvtsRef;
    juce::RangedAudioParameter* param { nullptr };
    juce::String suffixText;
    std::unique_ptr<juce::ParameterAttachment> attachment;
    int current { 1 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OctaveStepper)
};

// -----------------------------------------------------------------------------
class DeckChip : public juce::Component
{
public:
    DeckChip (const juce::String& label, const juce::String& value = {}, juce::Colour valueColour = Deck::green());

    void setValue (const juce::String& v, juce::Colour colour);
    void setValue (const juce::String& v) { setValue (v, valueCol); }
    void setLabel (const juce::String& l);

    /** Click handler (toggle chips). */
    std::function<void()> onClick;
    /** Vertical-drag handler: delta in "ticks" (positive = up). */
    std::function<void (int)> onDragTicks;

    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;
    void mouseUp (const juce::MouseEvent& e) override;
    void mouseEnter (const juce::MouseEvent&) override { hover = true; repaint(); }
    void mouseExit (const juce::MouseEvent&) override { hover = false; repaint(); }

    int preferredWidth() const;
    static constexpr int kH = 22;

private:
    juce::String labelText, valueText;
    juce::Colour valueCol;
    bool hover { false };
    int dragAccum { 0 };
    bool dragged { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DeckChip)
};
