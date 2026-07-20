#include "MacroKnob.h"
#include "JetsonicTheme.h"

namespace
{
constexpr float kStartAngle = juce::MathConstants<float>::pi * 1.25f;  // 7:30
constexpr float kEndAngle   = juce::MathConstants<float>::pi * 2.75f;  // 4:30
constexpr int   kLedCount   = 25;
} // namespace

void MacroKnob::KnobSlider::mouseDown (const juce::MouseEvent& e)
{
    // Fine / precision dragging with the platform modifier.
    const bool fine = e.mods.isCommandDown() || e.mods.isCtrlDown() || e.mods.isShiftDown();
    setMouseDragSensitivity (fine ? 1600 : 260);
    juce::Slider::mouseDown (e);
}

void MacroKnob::KnobSlider::mouseEnter (const juce::MouseEvent& e)
{
    juce::Slider::mouseEnter (e);
    if (auto* p = getParentComponent())
        p->repaint();
}

void MacroKnob::KnobSlider::mouseExit (const juce::MouseEvent& e)
{
    juce::Slider::mouseExit (e);
    if (auto* p = getParentComponent())
        p->repaint();
}

MacroKnob::MacroKnob (juce::AudioProcessorValueTreeState& apvts,
                      const juce::String& id,
                      const juce::String& title,
                      const juce::String& sub)
    : apvtsRef (apvts), paramId (id), titleText (title), subText (sub)
{
    setInterceptsMouseClicks (true, true);

    slider.setWantsKeyboardFocus (false);
    slider.setColour (juce::Slider::rotarySliderFillColourId, juce::Colours::transparentBlack);
    slider.setAlpha (0.0f); // painted entirely by MacroKnob::paint
    addAndMakeVisible (slider);

    attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        apvtsRef, paramId, slider);

    // Double-click resets to the parameter default (denormalized via the
    // parameter's own range so skewed ranges land on the true default).
    if (auto* param = apvtsRef.getParameter (paramId))
        slider.setDoubleClickReturnValue (true,
            (double) param->convertFrom0to1 (param->getDefaultValue()));

    slider.onValueChange = [this] { repaint(); };
}

MacroKnob::~MacroKnob() = default;

void MacroKnob::resized()
{
    // Knob hit area: circle at top center of the cell.
    const int d = juce::jmin (getWidth(), 116);
    slider.setBounds ((getWidth() - d) / 2, 8, d, d);
}

juce::String MacroKnob::valueText() const
{
    if (auto* param = apvtsRef.getParameter (paramId))
        return param->getCurrentValueAsText();
    return {};
}

void MacroKnob::paint (juce::Graphics& g)
{
    const float cx = (float) getWidth() * 0.5f;
    const float cy = 8.0f + 58.0f;
    const float housingR = 46.0f;
    const float capR     = 30.0f;
    const float ledR     = 52.5f;

    const bool hover   = slider.isMouseOverOrDragging();
    const bool pressed = slider.isMouseButtonDown();

    const float norm = (float) juce::jmap (slider.getValue(),
                                           slider.getMinimum(), slider.getMaximum(), 0.0, 1.0);
    const float angle = kStartAngle + norm * (kEndAngle - kStartAngle);

    // --- LED value ring ----------------------------------------------------
    for (int i = 0; i < kLedCount; ++i)
    {
        const float t = (float) i / (float) (kLedCount - 1);
        const float a = kStartAngle + t * (kEndAngle - kStartAngle);
        const bool lit = t <= norm + 0.0001f;

        const juce::Point<float> pos (cx + ledR * std::sin (a), cy - ledR * std::cos (a));
        const float dotR = lit ? 1.9f : 1.3f;

        if (lit)
        {
            // soft bloom
            g.setColour (Jetsonic::cyan().withAlpha (0.20f));
            g.fillEllipse (pos.x - dotR * 2.4f, pos.y - dotR * 2.4f, dotR * 4.8f, dotR * 4.8f);
            g.setColour (juce::Colour (0xffcfeeff).withAlpha (hover ? 1.0f : 0.92f));
        }
        else
        {
            g.setColour (juce::Colour (0xff1c2e3d));
        }
        g.fillEllipse (pos.x - dotR, pos.y - dotR, dotR * 2.0f, dotR * 2.0f);
    }

    // --- Outer black housing -------------------------------------------------
    {
        juce::ColourGradient grad (juce::Colour (0xff20242a), cx - housingR, cy - housingR,
                                   juce::Colour (0xff05070a), cx + housingR * 0.6f, cy + housingR, true);
        g.setGradientFill (grad);
        g.fillEllipse (cx - housingR, cy - housingR, housingR * 2.0f, housingR * 2.0f);

        // thin metallic bevel
        g.setColour (juce::Colour (0xff3a4149).withAlpha (hover ? 0.95f : 0.75f));
        g.drawEllipse (cx - housingR, cy - housingR, housingR * 2.0f, housingR * 2.0f, 1.2f);
        g.setColour (juce::Colours::black.withAlpha (0.8f));
        g.drawEllipse (cx - housingR + 1.5f, cy - housingR + 1.5f,
                       (housingR - 1.5f) * 2.0f, (housingR - 1.5f) * 2.0f, 1.5f);

        // knurled grip ticks on the housing rim
        g.setColour (juce::Colour (0xff2c333b).withAlpha (0.9f));
        for (int i = 0; i < 36; ++i)
        {
            const float a = juce::MathConstants<float>::twoPi * (float) i / 36.0f;
            const float r0 = housingR - 6.0f, r1 = housingR - 2.5f;
            g.drawLine (cx + r0 * std::sin (a), cy - r0 * std::cos (a),
                        cx + r1 * std::sin (a), cy - r1 * std::cos (a), 1.0f);
        }
    }

    // --- Bronze cap ----------------------------------------------------------
    {
        // radial highlight from upper-left, dark toward lower-right
        juce::ColourGradient grad (juce::Colour (0xffd8bc80), cx - capR * 0.55f, cy - capR * 0.65f,
                                   juce::Colour (0xff59421f), cx + capR * 0.7f, cy + capR * 0.85f, true);
        grad.addColour (0.45, juce::Colour (0xffa9853f));
        g.setGradientFill (grad);
        g.fillEllipse (cx - capR, cy - capR, capR * 2.0f, capR * 2.0f);

        if (pressed)
        {
            g.setColour (juce::Colours::black.withAlpha (0.18f));
            g.fillEllipse (cx - capR, cy - capR, capR * 2.0f, capR * 2.0f);
        }

        // dark centre shadow + inner bevel ring
        juce::ColourGradient centreShadow (juce::Colours::transparentBlack, cx, cy - capR * 0.3f,
                                           juce::Colours::black.withAlpha (0.45f), cx, cy + capR, false);
        g.setGradientFill (centreShadow);
        g.fillEllipse (cx - capR, cy - capR, capR * 2.0f, capR * 2.0f);

        g.setColour (juce::Colour (0xffe9d6a4).withAlpha (0.85f));
        g.drawEllipse (cx - capR, cy - capR, capR * 2.0f, capR * 2.0f, 1.0f);
        g.setColour (juce::Colour (0xff33260f).withAlpha (0.9f));
        g.drawEllipse (cx - capR + 1.8f, cy - capR + 1.8f,
                       (capR - 1.8f) * 2.0f, (capR - 1.8f) * 2.0f, 1.0f);

        // thin specular arc top-left
        juce::Path spec;
        spec.addCentredArc (cx, cy, capR - 4.0f, capR - 4.0f, 0.0f, -2.4f, -0.9f, true);
        g.setColour (juce::Colours::white.withAlpha (hover ? 0.35f : 0.25f));
        g.strokePath (spec, juce::PathStrokeType (1.4f));
    }

    // --- Position indicator ----------------------------------------------------
    {
        const float r0 = capR * 0.30f, r1 = capR * 0.86f;
        const juce::Point<float> p0 (cx + r0 * std::sin (angle), cy - r0 * std::cos (angle));
        const juce::Point<float> p1 (cx + r1 * std::sin (angle), cy - r1 * std::cos (angle));
        g.setColour (juce::Colour (0xff2a1f0d));
        g.drawLine ({ p0, p1 }, 3.2f);
        g.setColour (juce::Colour (0xfff3ecdb));
        g.drawLine ({ p0, p1 }, 1.6f);
    }

    // focus ring (keyboard / host focus)
    if (hasKeyboardFocus (true))
    {
        g.setColour (Jetsonic::cyan().withAlpha (0.5f));
        g.drawEllipse (cx - housingR - 4.0f, cy - housingR - 4.0f,
                       (housingR + 4.0f) * 2.0f, (housingR + 4.0f) * 2.0f, 1.0f);
    }

    // --- Labels ------------------------------------------------------------------
    const int textTop = (int) (cy + housingR) + 12;
    g.setFont (Jetsonic::label (13.0f, 0.10f));
    g.setColour (Jetsonic::goldBright());
    g.drawText (titleText, 0, textTop, getWidth(), 15, juce::Justification::centred);

    g.setFont (Jetsonic::value (13.0f));
    g.setColour (juce::Colour (0xfff2f5f7));
    g.drawText (valueText(), 0, textTop + 17, getWidth(), 15, juce::Justification::centred);

    g.setFont (Jetsonic::body (11.0f));
    g.setColour (Jetsonic::cyan().withAlpha (0.75f));
    g.drawText (subText, 0, textTop + 34, getWidth(), 13, juce::Justification::centred);
}
