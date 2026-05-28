#include "WaveformDisplay.h"
#include "../PluginProcessor.h"
#include "LuxuryLookAndFeel.h"

WaveformDisplay::WaveformDisplay (AviatorKeyzProcessor& processorRef)
    : processor (processorRef)
{
}

void WaveformDisplay::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    g.setColour (juce::Colour (0xff0d0d0d));
    g.fillRoundedRectangle (bounds, 6.f);
    g.setColour (LuxuryLookAndFeel::goldDark());
    g.drawRoundedRectangle (bounds.reduced (0.5f), 6.f, 1.f);

    const float* data = processor.getFactoryWaveformData();
    const int    n    = processor.getFactoryWaveformFrames();

    if (data == nullptr || n < 2)
    {
        g.setColour (LuxuryLookAndFeel::textDim());
        g.setFont (juce::Font (juce::FontOptions (14.f)));
        g.drawText ("No factory sample loaded", getLocalBounds(), juce::Justification::centred);
        return;
    }

    juce::Path path;
    const float midY = bounds.getCentreY();
    const float amp  = bounds.getHeight() * 0.42f;
    const int   w    = getWidth();

    path.startNewSubPath (0.f, midY);

    for (int x = 0; x < w; ++x)
    {
        const int idx = juce::jlimit (0, n - 1, x * n / juce::jmax (1, w));
        const float y = midY - juce::jlimit (-1.f, 1.f, data[idx]) * amp;
        path.lineTo ((float) x, y);
    }

    g.setColour (juce::Colour (0xff4a7fa5));
    g.strokePath (path, juce::PathStrokeType (1.4f));
}
