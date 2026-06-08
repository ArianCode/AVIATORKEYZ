#include "TextureVisualizerComponent.h"

namespace
{
constexpr int kMaxParticles = 72;
} // namespace

TextureVisualizerComponent::TextureVisualizerComponent()
{
    juce::Random rng;
    particles.resize ((size_t) kMaxParticles);

    for (auto& p : particles)
    {
        p.x     = rng.nextFloat();
        p.y     = rng.nextFloat();
        p.speed = 0.0015f + rng.nextFloat() * 0.004f;
        p.size  = 1.f + rng.nextFloat() * 2.5f;
        p.alpha = 0.15f + rng.nextFloat() * 0.55f;
        p.phase = rng.nextFloat() * juce::MathConstants<float>::twoPi;
    }

    startTimerHz (30);
}

TextureVisualizerComponent::~TextureVisualizerComponent()
{
    stopTimer();
}

void TextureVisualizerComponent::setAmount (float a)   { amount = juce::jlimit (0.f, 1.f, a); }
void TextureVisualizerComponent::setFrozen (bool f)     { frozen = f; }
void TextureVisualizerComponent::setReverse (bool r)    { reverse = r; }
void TextureVisualizerComponent::setRate (float r)      { rate = juce::jlimit (0.f, 1.f, r); }
void TextureVisualizerComponent::setSize (float s)      { grainSize = juce::jlimit (0.f, 1.f, s); }
void TextureVisualizerComponent::setDensity (float d)     { density = juce::jlimit (0.f, 1.f, d); }
void TextureVisualizerComponent::setSpread (float s)    { spread = juce::jlimit (0.f, 1.f, s); }
void TextureVisualizerComponent::setMotion (float m)    { motion = juce::jlimit (0.f, 1.f, m); }
void TextureVisualizerComponent::setDrift (float d)       { drift = juce::jlimit (0.f, 1.f, d); }

int TextureVisualizerComponent::activeParticleCount() const
{
    return juce::jmax (12, juce::roundToInt (12.f + density * (float) (kMaxParticles - 12)));
}

void TextureVisualizerComponent::paintInnerGlow (juce::Graphics& g, juce::Rectangle<float> bounds) const
{
    juce::ColourGradient edgeGlow (AviatorTokens::instrumentCyan().withAlpha (0.14f),
                                   bounds.getCentreX(), bounds.getY(),
                                   juce::Colours::transparentBlack,
                                   bounds.getCentreX(), bounds.getCentreY(),
                                   true);
    g.setGradientFill (edgeGlow);
    g.fillRect (bounds);

    juce::ColourGradient bottomGlow (AviatorTokens::instrumentCyan().withAlpha (0.08f),
                                      bounds.getCentreX(), bounds.getBottom(),
                                      juce::Colours::transparentBlack,
                                      bounds.getCentreX(), bounds.getCentreY(),
                                      true);
    g.setGradientFill (bottomGlow);
    g.fillRect (bounds);
}

void TextureVisualizerComponent::paintBorder (juce::Graphics& g, juce::Rectangle<float> bounds, float sc) const
{
    const float pulse = frozen ? (0.55f + 0.35f * std::sin ((float) time * 0.12f)) : 1.f;
    const auto borderCol = frozen ? AviatorTokens::champagneGold().withAlpha (0.55f * pulse)
                                  : AviatorTokens::instrumentCyan().withAlpha (0.55f);

    g.setColour (borderCol);
    g.drawRect (bounds, 1.f);

    const float bracket = 10.f * sc;
    const float t = 1.2f;
    g.setColour (AviatorTokens::champagneGold().withAlpha (0.85f));

    g.drawLine (bounds.getX(), bounds.getY() + bracket, bounds.getX(), bounds.getY(), t);
    g.drawLine (bounds.getX(), bounds.getY(), bounds.getX() + bracket, bounds.getY(), t);
    g.drawLine (bounds.getRight(), bounds.getY() + bracket, bounds.getRight(), bounds.getY(), t);
    g.drawLine (bounds.getRight() - bracket, bounds.getY(), bounds.getRight(), bounds.getY(), t);
    g.drawLine (bounds.getX(), bounds.getBottom() - bracket, bounds.getX(), bounds.getBottom(), t);
    g.drawLine (bounds.getX(), bounds.getBottom(), bounds.getX() + bracket, bounds.getBottom(), t);
    g.drawLine (bounds.getRight(), bounds.getBottom() - bracket, bounds.getRight(), bounds.getBottom(), t);
    g.drawLine (bounds.getRight() - bracket, bounds.getBottom(), bounds.getRight(), bounds.getBottom(), t);
}

void TextureVisualizerComponent::paintFrozenScan (juce::Graphics& g, juce::Rectangle<float> bounds) const
{
    for (int i = 0; i < 4; ++i)
    {
        const float ly = bounds.getHeight() * (0.22f + i * 0.16f);
        g.setColour (AviatorTokens::champagneGold().withAlpha (0.28f - i * 0.05f));
        g.fillRect (bounds.getX(), bounds.getY() + ly, bounds.getWidth(), 0.75f);
    }
}

void TextureVisualizerComponent::paintParticles (juce::Graphics& g, juce::Rectangle<float> bounds) const
{
    const float sizeMul = 0.6f + grainSize * 2.2f;
    const int count = activeParticleCount();

    for (int i = 0; i < count; ++i)
    {
        const auto& p = particles[(size_t) i];

        for (int t = 0; t < (int) p.trail.size() - 1; ++t)
        {
            const auto& a = p.trail[(size_t) t];
            const auto& b = p.trail[(size_t) (t + 1)];
            if (a.isOrigin() && b.isOrigin())
                continue;

            const float trailAlpha = p.alpha * amount * (0.08f + 0.06f * (float) t);
            g.setColour (AviatorTokens::instrumentCyan().withAlpha (juce::jlimit (0.f, 0.35f, trailAlpha)));
            g.drawLine (a.x, a.y, b.x, b.y, juce::jmax (0.5f, p.size * sizeMul * 0.35f));
        }

        juce::Point<float> pos = p.trail[0];
        if (pos.isOrigin())
        {
            const float motionMul = 0.15f + motion * 0.85f;
            const float spreadMul = 0.08f + spread * 0.38f;
            const float wobble = std::sin (time * 0.04f + p.x * 8.f + p.phase) * spreadMul * motionMul;
            pos = { bounds.getX() + p.x * bounds.getWidth(),
                    bounds.getCentreY() + wobble * bounds.getHeight() * 0.5f };
        }

        const float alpha = p.alpha * (0.45f + 0.55f * std::sin (time * 0.05f + p.phase)) * amount;
        g.setColour (AviatorTokens::instrumentCyan().withAlpha (juce::jlimit (0.f, 1.f, alpha)));
        const float r = p.size * sizeMul;
        g.fillEllipse (pos.x - r, pos.y - r, r * 2.f, r * 2.f);
        g.setColour (AviatorTokens::instrumentCyan().withAlpha (juce::jlimit (0.f, 0.5f, alpha * 0.35f)));
        g.fillEllipse (pos.x - r * 1.8f, pos.y - r * 1.8f, r * 3.6f, r * 3.6f);
    }
}

void TextureVisualizerComponent::paint (juce::Graphics& g)
{
    const float sc = AviatorTokens::scaleFor (*this);
    auto bounds = getLocalBounds().toFloat().reduced (1.f);

    g.setColour (juce::Colour (0xff060d1a));
    g.fillRect (bounds);

    paintInnerGlow (g, bounds);
    paintParticles (g, bounds);

    if (frozen)
        paintFrozenScan (g, bounds);

    paintBorder (g, bounds, sc);

    g.setFont (AviatorTokens::hudBold (9.f * sc));
    g.setColour (AviatorTokens::champagneGold().withAlpha (0.92f));
    g.drawText ("TEXTURE", bounds.reduced (8.f * sc, 6.f * sc).removeFromTop (14.f * sc),
                juce::Justification::centredLeft);
}

void TextureVisualizerComponent::timerCallback()
{
    if (! frozen)
    {
        ++time;
        const float speedMul = (0.25f + rate * 1.75f) * (reverse ? -1.f : 1.f);
        const float motionMul = 0.15f + motion * 0.85f;
        const float spreadMul = 0.08f + spread * 0.38f;
        const float driftStep = drift * 0.0004f;
        const auto bounds = getLocalBounds().toFloat().reduced (1.f);

        for (auto& p : particles)
        {
            for (int t = (int) p.trail.size() - 1; t > 0; --t)
                p.trail[(size_t) t] = p.trail[(size_t) (t - 1)];

            p.x = std::fmod (p.x + p.speed * speedMul + 1.f, 1.f);
            p.y = std::fmod (p.y + driftStep + std::sin (time * 0.025f + p.phase) * spreadMul * 0.004f + 1.f, 1.f);

            const float px = bounds.getX() + p.x * bounds.getWidth();
            const float py = bounds.getCentreY()
                           + std::sin (time * 0.04f + p.x * 8.f + p.phase) * spreadMul * motionMul
                             * bounds.getHeight() * 0.5f;
            p.trail[0] = { px, py };
        }
    }

    repaint();
}
