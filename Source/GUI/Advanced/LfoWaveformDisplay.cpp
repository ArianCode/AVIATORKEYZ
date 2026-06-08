#include "LfoWaveformDisplay.h"

LfoWaveformDisplay::LfoWaveformDisplay (juce::AudioProcessorValueTreeState& apvts,
                                        const char* shapeParamId,
                                        const char* phaseParamId)
    : apvtsRef (apvts)
    , shapeId (shapeParamId)
    , phaseId (phaseParamId)
{
    startTimerHz (30);
}

float LfoWaveformDisplay::sampleShape (float phase01) const
{
    int shape = 0;
    if (auto* p = dynamic_cast<juce::AudioParameterChoice*> (apvtsRef.getParameter (shapeId)))
        shape = p->getIndex();

    switch (shape)
    {
        case 1: return phase01 < 0.5f ? 1.f : -1.f;
        case 2:
        {
            const float t = phase01 < 0.5f ? phase01 * 2.f : 2.f - phase01 * 2.f;
            return t * 2.f - 1.f;
        }
        case 3: return phase01 * 2.f - 1.f;
        case 4: return 1.f - phase01 * 2.f;
        case 5: return juce::Random::getSystemRandom().nextFloat() * 2.f - 1.f;
        default: return std::sin (phase01 * juce::MathConstants<float>::twoPi);
    }
}

void LfoWaveformDisplay::timerCallback()
{
    animPhase += 0.02f;
    if (animPhase > 1.f)
        animPhase -= 1.f;
    repaint();
}

void LfoWaveformDisplay::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced (2.f);
    g.setColour (juce::Colour (0xff060d1a));
    g.fillRoundedRectangle (bounds, 3.f);
    g.setColour (AviatorTokens::instrumentCyan().withAlpha (0.15f));
    g.drawRoundedRectangle (bounds, 3.f, 0.75f);

    float phaseOffset = 0.f;
    if (auto* raw = apvtsRef.getRawParameterValue (phaseId))
        phaseOffset = raw->load();

    juce::Path wave;
    const int steps = juce::jmax (16, getWidth());
    for (int i = 0; i <= steps; ++i)
    {
        const float t = static_cast<float> (i) / static_cast<float> (steps);
        const float p = std::fmod (t + phaseOffset + animPhase * 0.15f, 1.f);
        const float y = bounds.getCentreY() - sampleShape (p) * bounds.getHeight() * 0.38f;
        const float x = bounds.getX() + t * bounds.getWidth();
        if (i == 0)
            wave.startNewSubPath (x, y);
        else
            wave.lineTo (x, y);
    }

    g.setColour (juce::Colour (0xff4DB8D4).withAlpha (0.85f));
    g.strokePath (wave, juce::PathStrokeType (1.5f));
}
