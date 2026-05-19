#include "HorizontalFader.h"
#include "DesignTokens.h"

HorizontalFader::HorizontalFader (juce::AudioProcessorValueTreeState& apvts,
                                  const juce::String& paramID,
                                  const juce::String& label,
                                  TextFormatter fmt)
    : formatter (std::move (fmt))
{
    setOpaque (false);
    slider.setSliderStyle (juce::Slider::LinearHorizontal);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.onValueChange = [this] {
        valueLabel.setText (formatter ((float) slider.getValue()), juce::dontSendNotification);
        repaint();
    };
    addChildComponent (slider);

    shortLabel.setText (label, juce::dontSendNotification);
    shortLabel.setJustificationType (juce::Justification::centredLeft);
    shortLabel.setFont (DesignTokens::labelFont (7.f, juce::Font::bold));
    shortLabel.setColour (juce::Label::textColourId, DesignTokens::textFaint());
    addAndMakeVisible (shortLabel);

    valueLabel.setJustificationType (juce::Justification::centredRight);
    valueLabel.setFont (DesignTokens::monoFont (7.f));
    valueLabel.setColour (juce::Label::textColourId, DesignTokens::textFaint());
    addAndMakeVisible (valueLabel);

    attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        apvts, paramID, slider);

    valueLabel.setText (formatter ((float) slider.getValue()), juce::dontSendNotification);
}

void HorizontalFader::resized()
{
    const float s = DesignTokens::scaleFactorFor (*this);

    shortLabel.setFont (DesignTokens::labelFont (7.f * s, juce::Font::bold));
    valueLabel.setFont (DesignTokens::monoFont (7.f * s));

    auto r = getLocalBounds();
    shortLabel.setBounds (r.removeFromLeft (DesignTokens::scaled (24, s)));
    valueLabel.setBounds (r.removeFromRight (DesignTokens::scaled (26, s)));
}

juce::Rectangle<int> HorizontalFader::trackBounds() const
{
    const float s = DesignTokens::scaleFactorFor (*this);
    return getLocalBounds().withTrimmedLeft (DesignTokens::scaled (24, s))
                           .withTrimmedRight (DesignTokens::scaled (26, s))
                           .reduced (0, DesignTokens::scaled (10, s));
}

float HorizontalFader::getProportion() const
{
    return (float) slider.getNormalisableRange().convertTo0to1 (slider.getValue());
}

void HorizontalFader::setFromMouse (int x)
{
    const auto track = trackBounds();
    if (track.isEmpty())
        return;

    const float proportion = juce::jlimit (0.f, 1.f, (float) (x - track.getX()) / (float) track.getWidth());
    slider.setValue (slider.getNormalisableRange().convertFrom0to1 (proportion),
                     juce::sendNotificationSync);
}

void HorizontalFader::paint (juce::Graphics& g)
{
    const float s = DesignTokens::scaleFactorFor (*this);
    const auto track = trackBounds().toFloat();
    const float proportion = getProportion();

    g.setColour (juce::Colours::white.withAlpha (0.055f));
    g.fillRoundedRectangle (track, 1.f * s);

    auto fill = track.withWidth (track.getWidth() * proportion);
    juce::ColourGradient fillGrad (DesignTokens::champagne().withAlpha (0.2f),
                                   fill.getX(), fill.getCentreY(),
                                   DesignTokens::champagneMid(),
                                   fill.getRight(), fill.getCentreY(),
                                   false);
    g.setGradientFill (fillGrad);
    g.fillRoundedRectangle (fill, 1.f * s);

    const float thumbX = track.getX() + track.getWidth() * proportion;
    const float thumbR = 3.5f * s;
    g.setColour (DesignTokens::elevated());
    g.fillEllipse (thumbX - thumbR, track.getCentreY() - thumbR, thumbR * 2.f, thumbR * 2.f);
    g.setColour (DesignTokens::champagne().withAlpha (0.38f));
    g.drawEllipse (thumbX - thumbR, track.getCentreY() - thumbR, thumbR * 2.f, thumbR * 2.f, 1.f * s);
}

void HorizontalFader::mouseDown (const juce::MouseEvent& e)
{
    setFromMouse (e.x);
}

void HorizontalFader::mouseDrag (const juce::MouseEvent& e)
{
    setFromMouse (e.x);
}

void HorizontalFader::mouseUp (const juce::MouseEvent&) {}
