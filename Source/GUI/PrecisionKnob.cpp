#include "PrecisionKnob.h"
#include "DesignTokens.h"

namespace
{
    juce::Point<float> polar (float cx, float cy, float r, float degrees)
    {
        const float rad = juce::degreesToRadians (degrees - 90.f);
        return { cx + r * std::cos (rad), cy + r * std::sin (rad) };
    }

    void addArc (juce::Path& path, float cx, float cy, float r, float a1, float a2)
    {
        const float sweep = std::fmod (a2 - a1 + 360.f, 360.f);
        path.addCentredArc (cx, cy, r, r, 0.f,
                            juce::degreesToRadians (a1 - 90.f),
                            juce::degreesToRadians (a2 - 90.f),
                            sweep > 180.f);
    }
}

PrecisionKnob::PrecisionKnob (juce::AudioProcessorValueTreeState& apvts,
                              const juce::String& paramID,
                              const juce::String& macroName,
                              const juce::String& sublabel,
                              ValueFormat format)
    : valueFormat (format)
{
    setOpaque (false);
    slider.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.onValueChange = [this] { syncFromSlider(); };
    addChildComponent (slider);

    nameLabel.setText (macroName, juce::dontSendNotification);
    nameLabel.setJustificationType (juce::Justification::centred);
    nameLabel.setFont (DesignTokens::labelFont (8.f, juce::Font::bold));
    nameLabel.setColour (juce::Label::textColourId, DesignTokens::textSecondary());
    addAndMakeVisible (nameLabel);

    subLabel.setText (sublabel, juce::dontSendNotification);
    subLabel.setJustificationType (juce::Justification::centred);
    subLabel.setFont (DesignTokens::labelFont (6.f));
    subLabel.setColour (juce::Label::textColourId, DesignTokens::textFaint());
    addAndMakeVisible (subLabel);

    valueLabel.setJustificationType (juce::Justification::centred);
    valueLabel.setFont (DesignTokens::monoFont (8.f));
    valueLabel.setColour (juce::Label::textColourId, DesignTokens::textMuted());
    addAndMakeVisible (valueLabel);

    attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        apvts, paramID, slider);

    syncFromSlider();
}

void PrecisionKnob::resized()
{
    const float s = DesignTokens::scaleFactorFor (*this);

    nameLabel.setFont (DesignTokens::labelFont (8.f * s, juce::Font::bold));
    subLabel.setFont (DesignTokens::labelFont (6.f * s));
    valueLabel.setFont (DesignTokens::monoFont (8.f * s));

    auto area = getLocalBounds();
    valueLabel.setBounds (area.removeFromBottom (DesignTokens::scaled (11, s)));
    subLabel.setBounds (area.removeFromBottom (DesignTokens::scaled (10, s)));
    nameLabel.setBounds (area.removeFromTop (DesignTokens::scaled (12, s)));
    slider.setBounds (area.withSizeKeepingCentre (DesignTokens::scaled (72, s), DesignTokens::scaled (72, s)));
}

float PrecisionKnob::getNormalisedValue() const
{
    return (float) slider.getNormalisableRange().convertTo0to1 (slider.getValue());
}

juce::String PrecisionKnob::getValueText() const
{
    switch (valueFormat)
    {
        case ValueFormat::glideSeconds:
        {
            const float ms = (float) slider.getValue();
            if (ms < 1.f)
                return "Off";
            return juce::String (ms / 1000.f, 2) + " s";
        }
        case ValueFormat::percent:
            return juce::String (juce::roundToInt (getNormalisedValue() * 100.f)) + "%";
        case ValueFormat::toneDb:
        {
            const float norm = getNormalisedValue();
            const float db = (norm - 0.5f) * 16.f;
            return (db >= 0.f ? "+" : "") + juce::String (db, 1) + " dB";
        }
    }
    return {};
}

void PrecisionKnob::syncFromSlider()
{
    valueLabel.setText (getValueText(), juce::dontSendNotification);
    repaint();
}

void PrecisionKnob::paint (juce::Graphics& g)
{
    const float s = DesignTokens::scaleFactorFor (*this);

    const auto knobBounds = slider.getBounds().toFloat();
    const float cx = knobBounds.getCentreX();
    const float cy = knobBounds.getCentreY();
    const float trackR = 30.f * s;
    const float bodyR  = 22.f * s;
    const float capR   = 7.f * s;
    const float val    = getNormalisedValue();

    constexpr float startDeg = 135.f;
    constexpr float sweep    = 270.f;
    const float valDeg = startDeg + val * sweep;

    for (int i = 0; i < 12; ++i)
    {
        const float deg = startDeg + (i / 11.f) * sweep;
        const bool primary = (i % 3 == 0);
        const auto p1 = polar (cx, cy, trackR + 2.f * s, deg);
        const auto p2 = polar (cx, cy, trackR + 5.f * s, deg);
        g.setColour (primary ? DesignTokens::champagne().withAlpha (0.28f)
                             : juce::Colours::white.withAlpha (0.08f));
        g.drawLine ({ p1, p2 }, primary ? 1.2f * s : 0.8f * s);
    }

    juce::Path trackPath;
    addArc (trackPath, cx, cy, trackR, startDeg, startDeg + sweep);
    g.setColour (juce::Colours::white.withAlpha (0.05f));
    g.strokePath (trackPath, juce::PathStrokeType (1.6f * s, juce::PathStrokeType::curved,
                                                   juce::PathStrokeType::rounded));

    if (val > 0.006f)
    {
        juce::Path valuePath;
        addArc (valuePath, cx, cy, trackR, startDeg, valDeg);
        g.setColour (DesignTokens::champagne().withAlpha (0.18f));
        g.strokePath (valuePath, juce::PathStrokeType (1.8f * s, juce::PathStrokeType::curved,
                                                         juce::PathStrokeType::rounded));
        g.setColour (DesignTokens::champagne().withAlpha (0.88f));
        g.strokePath (valuePath, juce::PathStrokeType (1.2f * s, juce::PathStrokeType::curved,
                                                         juce::PathStrokeType::rounded));
    }

    juce::ColourGradient bodyGrad (juce::Colour (0xff2c2a3c), cx - bodyR * 0.2f, cy - bodyR * 0.3f,
                                   juce::Colour (0xff0e0c18), cx, cy + bodyR, true);
    g.setGradientFill (bodyGrad);
    g.fillEllipse (cx - bodyR, cy - bodyR, bodyR * 2.f, bodyR * 2.f);

    g.setColour (juce::Colours::white.withAlpha (0.07f));
    g.drawEllipse (cx - bodyR, cy - bodyR, bodyR * 2.f, bodyR * 2.f, 0.8f * s);

    juce::ColourGradient shine (juce::Colours::white.withAlpha (0.09f),
                                cx - bodyR * 0.3f, cy - bodyR * 0.4f,
                                juce::Colours::transparentWhite,
                                cx, cy, true);
    g.setGradientFill (shine);
    g.fillEllipse (cx - bodyR, cy - bodyR, bodyR * 2.f, bodyR * 2.f);

    g.setColour (juce::Colours::white.withAlpha (0.025f));
    g.drawEllipse (cx - (bodyR - 6.f * s), cy - (bodyR - 6.f * s),
                   (bodyR - 6.f * s) * 2.f, (bodyR - 6.f * s) * 2.f, 0.5f * s);

    const auto tipOuter = polar (cx, cy, bodyR - 4.f * s, valDeg);
    const auto tipInner = polar (cx, cy, capR + 2.f * s, valDeg);
    const auto tipDot   = polar (cx, cy, bodyR - 6.f * s, valDeg);

    g.setColour (DesignTokens::champagne().withAlpha (0.6f));
    g.drawLine ({ tipInner, tipOuter }, 1.f * s);
    g.setColour (DesignTokens::champagne());
    g.fillEllipse (tipDot.x - 2.f * s, tipDot.y - 2.f * s, 4.f * s, 4.f * s);
    g.setColour (DesignTokens::champagneBright());
    g.fillEllipse (tipDot.x - 1.2f * s, tipDot.y - 1.2f * s, 2.4f * s, 2.4f * s);

    juce::ColourGradient capGrad (juce::Colour (0xff201e2e), cx, cy,
                                  juce::Colour (0xff0c0a16), cx + capR, cy, true);
    g.setGradientFill (capGrad);
    g.fillEllipse (cx - capR, cy - capR, capR * 2.f, capR * 2.f);
    g.setColour (juce::Colours::white.withAlpha (0.07f));
    g.fillEllipse (cx - 3.4f * s, cy - 3.4f * s, 2.8f * s, 2.8f * s);
}

void PrecisionKnob::mouseDown (const juce::MouseEvent& e)
{
    if (! slider.getBounds().contains (e.getPosition()))
        return;

    slider.mouseDown (e.getEventRelativeTo (&slider));
}

void PrecisionKnob::mouseDrag (const juce::MouseEvent& e)
{
    slider.mouseDrag (e.getEventRelativeTo (&slider));
}

void PrecisionKnob::mouseUp (const juce::MouseEvent& e)
{
    slider.mouseUp (e.getEventRelativeTo (&slider));
}

void PrecisionKnob::mouseDoubleClick (const juce::MouseEvent& e)
{
    slider.mouseDoubleClick (e.getEventRelativeTo (&slider));
}
