#include "MiniControls.h"
#include "AviationTheme.h"

namespace AviationMini
{

void configureAttachment (juce::AudioProcessorValueTreeState& apvts,
                          const juce::String& paramId,
                          juce::Slider& slider,
                          std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>& attachment)
{
    attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, paramId, slider);
    if (auto* param = apvts.getParameter (paramId))
        slider.setDoubleClickReturnValue (true, (double) param->convertFrom0to1 (param->getDefaultValue()));
}

juce::String parameterText (juce::AudioProcessorValueTreeState& apvts, const juce::String& paramId)
{
    if (auto* param = apvts.getParameter (paramId))
        return param->getCurrentValueAsText();
    return {};
}

float parameterNorm (juce::AudioProcessorValueTreeState& apvts, const juce::String& paramId)
{
    if (auto* param = apvts.getParameter (paramId))
        return param->getValue();
    return 0.0f;
}

} // namespace AviationMini

// =============================================================================
//  MiniRotary
// =============================================================================
MiniRotary::MiniRotary (juce::AudioProcessorValueTreeState& apvts,
                        const juce::String& id,
                        const juce::String& label,
                        bool showValueText)
    : apvtsRef (apvts), paramId (id), labelText (label), showValue (showValueText)
{
    slider.setAlpha (0.0f);
    slider.setWantsKeyboardFocus (false);
    addAndMakeVisible (slider);
    AviationMini::configureAttachment (apvtsRef, paramId, slider, attachment);
    slider.onValueChange = [this] { repaint(); };
}

void MiniRotary::resized()
{
    const int d = juce::jmin (getWidth(), getHeight() - (showValue ? 24 : 12));
    slider.setBounds ((getWidth() - d) / 2, 0, d, d);
}

void MiniRotary::paint (juce::Graphics& g)
{
    const auto knob = slider.getBounds().toFloat();
    const float cx = knob.getCentreX();
    const float cy = knob.getCentreY();
    const float radius = knob.getWidth() * 0.5f - 3.0f;

    constexpr float a0 = juce::MathConstants<float>::pi * 1.25f;
    constexpr float a1 = juce::MathConstants<float>::pi * 2.75f;
    const float norm = (float) juce::jmap (slider.getValue(), slider.getMinimum(), slider.getMaximum(), 0.0, 1.0);
    const float angle = a0 + norm * (a1 - a0);
    const bool hover = slider.isMouseOverOrDragging();

    // cyan value arc
    juce::Path track, arc;
    track.addCentredArc (cx, cy, radius + 2.5f, radius + 2.5f, 0.0f, a0, a1, true);
    g.setColour (juce::Colour (0xff17262f));
    g.strokePath (track, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    arc.addCentredArc (cx, cy, radius + 2.5f, radius + 2.5f, 0.0f, a0, angle, true);
    g.setColour (Aviation::cyan().withAlpha (hover ? 1.0f : 0.85f));
    g.strokePath (arc, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // bronze cap
    juce::ColourGradient grad (juce::Colour (0xffc3a566), cx - radius * 0.5f, cy - radius * 0.6f,
                               juce::Colour (0xff4e3a1c), cx + radius * 0.6f, cy + radius * 0.8f, true);
    g.setGradientFill (grad);
    g.fillEllipse (cx - radius, cy - radius, radius * 2.0f, radius * 2.0f);
    g.setColour (juce::Colour (0xff1c1508).withAlpha (0.9f));
    g.drawEllipse (cx - radius, cy - radius, radius * 2.0f, radius * 2.0f, 1.0f);

    // indicator
    const float r0 = radius * 0.25f, r1 = radius * 0.85f;
    g.setColour (juce::Colour (0xfff5efe0));
    g.drawLine (cx + r0 * std::sin (angle), cy - r0 * std::cos (angle),
                cx + r1 * std::sin (angle), cy - r1 * std::cos (angle), 1.6f);

    // label / value
    g.setFont (Aviation::label (9.0f, 0.10f));
    g.setColour (Aviation::gold().withAlpha (0.9f));
    g.drawText (labelText, 0, (int) knob.getBottom() + 1, getWidth(), 11, juce::Justification::centred);

    if (showValue)
    {
        juce::String text = AviationMini::parameterText (apvtsRef, paramId);
        if (valueFormatter)
            if (auto* param = apvtsRef.getParameter (paramId))
                text = valueFormatter (param->convertFrom0to1 (param->getValue()));

        g.setFont (Aviation::value (valueFormatter ? 8.5f : 10.0f));
        g.setColour (Aviation::textPrimary());
        g.drawText (text, 0, (int) knob.getBottom() + 12, getWidth(), 12, juce::Justification::centred);
    }
}

// =============================================================================
//  MiniParam
// =============================================================================
MiniParam::MiniParam (juce::AudioProcessorValueTreeState& apvts,
                      const juce::String& id,
                      const juce::String& label,
                      bool emphasized)
    : apvtsRef (apvts), paramId (id), labelText (label), emphasize (emphasized)
{
    slider.setAlpha (0.0f);
    slider.setWantsKeyboardFocus (false);
    addAndMakeVisible (slider);
    AviationMini::configureAttachment (apvtsRef, paramId, slider, attachment);
    slider.onValueChange = [this] { repaint(); };
}

void MiniParam::resized()
{
    slider.setBounds (getLocalBounds());
}

void MiniParam::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    const bool hover = slider.isMouseOverOrDragging();

    Aviation::fillGlassScreen (g, r, 5.0f, hover ? 0.5f : 0.28f);

    const float norm = (float) juce::jmap (slider.getValue(), slider.getMinimum(), slider.getMaximum(), 0.0, 1.0);

    // thin cyan fill meter along the bottom edge
    g.setColour (Aviation::cyan().withAlpha (0.55f));
    g.fillRect (juce::Rectangle<float> (r.getX() + 4.0f, r.getBottom() - 4.0f,
                                        (r.getWidth() - 8.0f) * norm, 2.0f));

    g.setFont (Aviation::label (emphasize ? 10.5f : 9.5f, 0.12f));
    g.setColour (Aviation::gold().withAlpha (0.92f));
    g.drawText (labelText, 0, 7, getWidth(), 12, juce::Justification::centred);

    // These macro cells are all 0..1 amounts — display as percentages.
    g.setFont (Aviation::value (emphasize ? 19.0f : 13.0f));
    g.setColour (emphasize ? juce::Colour (0xffffffff) : Aviation::cyanBright());
    g.drawText (juce::String (juce::roundToInt (norm * 100.0f)) + "%",
                0, 21, getWidth(), getHeight() - 26, juce::Justification::centred);
}

// =============================================================================
//  MiniFader
// =============================================================================
class MiniFader::FaderLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPos, float, float,
                           juce::Slider::SliderStyle, juce::Slider& s) override
    {
        const float cx = (float) x + (float) width * 0.5f;

        // recessed track
        g.setColour (juce::Colour (0xff0a151f));
        g.fillRoundedRectangle (cx - 2.0f, (float) y, 4.0f, (float) height, 2.0f);
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.drawRoundedRectangle (cx - 2.0f, (float) y, 4.0f, (float) height, 2.0f, 1.0f);

        // cyan level fill below the cap
        g.setColour (Aviation::cyan().withAlpha (0.6f));
        g.fillRoundedRectangle (cx - 1.5f, sliderPos, 3.0f, (float) (y + height) - sliderPos, 1.5f);

        // cap
        const bool hover = s.isMouseOverOrDragging();
        juce::Rectangle<float> cap (cx - 7.0f, sliderPos - 4.5f, 14.0f, 9.0f);
        juce::ColourGradient grad (juce::Colour (0xff3b4652), cap.getX(), cap.getY(),
                                   juce::Colour (0xff11161c), cap.getX(), cap.getBottom(), false);
        g.setGradientFill (grad);
        g.fillRoundedRectangle (cap, 2.5f);
        g.setColour (hover ? Aviation::cyanBright() : juce::Colour (0xff59667a));
        g.drawRoundedRectangle (cap, 2.5f, 1.0f);
    }
};

MiniFader::MiniFader (juce::AudioProcessorValueTreeState& apvts,
                      const juce::String& id,
                      const juce::String& label)
    : apvtsRef (apvts), paramId (id), labelText (label),
      lookAndFeel (std::make_unique<FaderLookAndFeel>())
{
    slider.setLookAndFeel (lookAndFeel.get());
    slider.setWantsKeyboardFocus (false);
    addAndMakeVisible (slider);
    AviationMini::configureAttachment (apvtsRef, paramId, slider, attachment);
}

MiniFader::~MiniFader()
{
    slider.setLookAndFeel (nullptr);
}

void MiniFader::resized()
{
    slider.setBounds (0, 2, getWidth(), getHeight() - 16);
}

void MiniFader::paint (juce::Graphics& g)
{
    g.setFont (Aviation::label (8.0f, 0.06f));
    g.setColour (Aviation::gold().withAlpha (0.85f));
    g.drawText (labelText, 0, getHeight() - 12, getWidth(), 11, juce::Justification::centred);
}

// =============================================================================
//  MiniToggle
// =============================================================================
void MiniToggle::SquareButton::paintButton (juce::Graphics& g, bool highlighted, bool down)
{
    auto r = getLocalBounds().toFloat().reduced (1.0f);
    const bool on = getToggleState();

    g.setColour (on ? juce::Colour (0xff0d3550) : juce::Colour (0xff0a1420));
    g.fillRoundedRectangle (r, 2.5f);

    if (on)
    {
        g.setColour (Aviation::cyan().withAlpha (0.35f));
        g.fillRoundedRectangle (r.expanded (1.5f), 3.5f);
        g.setColour (Aviation::cyanBright());
        g.fillRoundedRectangle (r.reduced (2.5f), 1.5f);
    }

    g.setColour ((highlighted || down) ? Aviation::cyan() : juce::Colour (0xff2b3c4c));
    g.drawRoundedRectangle (r, 2.5f, 1.0f);
}

MiniToggle::MiniToggle (juce::AudioProcessorValueTreeState& apvts,
                        const juce::String& id,
                        const juce::String& label)
    : apvtsRef (apvts), paramId (id), labelText (label)
{
    addAndMakeVisible (button);
    attachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (apvtsRef, paramId, button);
}

void MiniToggle::resized()
{
    const int d = juce::jmin (getWidth(), getHeight() - 12);
    button.setBounds ((getWidth() - d) / 2, 0, d, d);
}

void MiniToggle::paint (juce::Graphics& g)
{
    g.setFont (Aviation::label (8.0f, 0.06f));
    g.setColour (Aviation::gold().withAlpha (0.85f));
    g.drawText (labelText, 0, getHeight() - 11, getWidth(), 10, juce::Justification::centred);
}
