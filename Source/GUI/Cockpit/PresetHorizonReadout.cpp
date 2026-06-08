#include "PresetHorizonReadout.h"

void PresetHorizonReadout::setText (const juce::String& category, const juce::String& presetName)
{
    readoutText = category.toUpperCase() + "  |  " + presetName.toUpperCase();
    repaint();
}

void PresetHorizonReadout::paint (juce::Graphics& g)
{
    if (readoutText.isEmpty())
        return;

    const float sc = AviatorTokens::scaleFor (*this);
    g.setFont (AviatorTokens::hudBold (14.f * sc));

    const int tw = juce::roundToInt (g.getCurrentFont().getStringWidth (readoutText));
    const int padH = AviatorTokens::scaledFor (*this, 14);
    const int padV = AviatorTokens::scaledFor (*this, 6);
    const int pillW = tw + padH * 2;
    const int pillH = AviatorTokens::scaledFor (*this, 24);
    const juce::Rectangle<int> pill (getWidth() / 2 - pillW / 2,
                                     getHeight() / 2 - pillH / 2,
                                     pillW,
                                     pillH);

    g.setColour (juce::Colour (0x88000000));
    g.fillRoundedRectangle (pill.toFloat(), 6.f);
    g.setColour (juce::Colours::white);
    g.drawText (readoutText, pill, juce::Justification::centred);
}
