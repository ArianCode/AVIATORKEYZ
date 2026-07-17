#include "OscWaveformDisplay.h"

OscWaveformDisplay::OscWaveformDisplay (juce::AudioProcessorValueTreeState& apvts,
                                        const char* typeParamId,
                                        const char* shapeParamId,
                                        juce::Colour waveColour)
    : apvtsRef (apvts)
    , typeId (typeParamId)
    , shapeId (shapeParamId)
    , colour (waveColour)
{
    apvtsRef.addParameterListener (typeId, this);
    apvtsRef.addParameterListener (shapeId, this);
    startTimerHz (20);
}

OscWaveformDisplay::~OscWaveformDisplay()
{
    stopTimer();
    apvtsRef.removeParameterListener (typeId, this);
    apvtsRef.removeParameterListener (shapeId, this);
}

void OscWaveformDisplay::parameterChanged (const juce::String&, float)
{
    const auto apply = [self = juce::Component::SafePointer<OscWaveformDisplay> (this)]
    {
        if (self != nullptr)
            self->repaint();
    };

    if (juce::MessageManager::getInstance()->isThisTheMessageThread())
        apply();
    else
        juce::MessageManager::callAsync (apply);
}

void OscWaveformDisplay::timerCallback()
{
    animPhase += 0.015f;
    if (animPhase > 1.f)
        animPhase -= 1.f;
    repaint();
}

float OscWaveformDisplay::sampleAt (float phase) const
{
    int type = 0;
    if (auto* p = dynamic_cast<juce::AudioParameterChoice*> (apvtsRef.getParameter (typeId)))
        type = p->getIndex();

    float shape = 0.3f;
    if (auto* raw = apvtsRef.getRawParameterValue (shapeId))
        shape = raw->load();

    switch (type)
    {
        case 0: // saw
            return phase * 2.f - 1.f;
        case 1: // square
            return phase < 0.5f ? 1.f : -1.f;
        case 2: // tri
        {
            const float t = phase < 0.5f ? phase * 2.f : 2.f - phase * 2.f;
            return t * 2.f - 1.f;
        }
        case 3: // sine
            return std::sin (phase * juce::MathConstants<float>::twoPi);
        case 4: // noise
            return (juce::Random::getSystemRandom().nextFloat() * 2.f - 1.f) * shape;
        default:
            return std::sin (phase * juce::MathConstants<float>::twoPi * (1.f + shape * 4.f));
    }
}

void OscWaveformDisplay::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced (1.f);
    g.setColour (juce::Colour (0xff060d1a));
    g.fillRoundedRectangle (bounds, 2.f);
    g.setColour (colour.withAlpha (0.15f));
    g.drawRoundedRectangle (bounds, 2.f, 0.75f);

    // Centre reference line
    g.setColour (colour.withAlpha (0.06f));
    g.drawHorizontalLine (juce::roundToInt (bounds.getCentreY()), bounds.getX(), bounds.getRight());

    juce::Path wave;
    const int steps = juce::jmax (128, getWidth() * 3);
    float firstX = bounds.getX();

    for (int i = 0; i <= steps; ++i)
    {
        const float t = static_cast<float> (i) / static_cast<float> (steps);
        const float p = std::fmod (t + animPhase * 0.05f, 1.f);
        const float y = bounds.getCentreY() - sampleAt (p) * bounds.getHeight() * 0.4f;
        const float x = bounds.getX() + t * bounds.getWidth();

        if (i == 0)
        {
            firstX = x;
            wave.startNewSubPath (x, y);
        }
        else
            wave.lineTo (x, y);
    }

    // Gradient fill beneath wave
    juce::Path fill = wave;
    fill.lineTo (bounds.getRight(), bounds.getBottom());
    fill.lineTo (firstX, bounds.getBottom());
    fill.closeSubPath();

    juce::ColourGradient fillGrad (colour.withAlpha (0.18f), bounds.getX(), bounds.getCentreY() - bounds.getHeight() * 0.2f,
                                     colour.withAlpha (0.f), bounds.getX(), bounds.getBottom(), false);
    g.setGradientFill (fillGrad);
    g.fillPath (fill);

    // Soft glow layers (outer → inner)
    for (int layer = 3; layer >= 0; --layer)
    {
        g.setColour (colour.withAlpha (0.05f + 0.04f * (float) layer));
        g.strokePath (wave, juce::PathStrokeType (1.f + (float) layer * 2.5f,
                                                   juce::PathStrokeType::curved,
                                                   juce::PathStrokeType::rounded));
    }

    // Crisp anti-aliased core stroke
    g.setColour (colour.withAlpha (0.95f));
    g.strokePath (wave, juce::PathStrokeType (1.25f,
                                               juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));
}
