#include "EnvelopePanel.h"
#include "AviationTheme.h"
#include "../../State/StateSchema.h"

namespace
{
namespace P = AviatorKeyz::ParamID;

float normValue (juce::AudioProcessorValueTreeState& apvts, const char* id)
{
    if (auto* param = apvts.getParameter (id))
        return param->getValue();
    return 0.0f;
}
} // namespace

EnvelopePanel::EnvelopePanel (juce::AudioProcessorValueTreeState& apvts)
    : apvtsRef (apvts)
{
    const std::pair<const char*, const char*> specs[] = {
        { P::ENV_ATTACK,      "A" },
        { P::ENV_AMP_DECAY,   "D" },
        { P::ENV_AMP_SUSTAIN, "S" },
        { P::ENV_RELEASE,     "R" },
    };
    auto msText = [] (float ms)
    {
        return ms >= 1000.f ? juce::String (ms / 1000.f, 1) + "s"
                            : juce::String (juce::roundToInt (ms)) + "ms";
    };

    for (const auto& [id, label] : specs)
    {
        // Compact readouts: A/R in ms/s, D (stored in seconds) in ms/s, S in %.
        auto knob = std::make_unique<MiniRotary> (apvtsRef, id, label, true);
        if (id == P::ENV_AMP_SUSTAIN)
            knob->valueFormatter = [] (float v) { return juce::String (juce::roundToInt (v * 100.f)) + "%"; };
        else if (id == P::ENV_AMP_DECAY)
            knob->valueFormatter = [msText] (float seconds) { return msText (seconds * 1000.f); };
        else
            knob->valueFormatter = msText;
        addAndMakeVisible (*knob);
        adsrKnobs.push_back (std::move (knob));
        apvtsRef.addParameterListener (id, this);
    }
    apvtsRef.addParameterListener (P::ENV_ENABLED, this);
}

EnvelopePanel::~EnvelopePanel()
{
    for (const char* id : { P::ENV_ATTACK, P::ENV_AMP_DECAY, P::ENV_AMP_SUSTAIN, P::ENV_RELEASE, P::ENV_ENABLED })
        apvtsRef.removeParameterListener (id, this);
}

void EnvelopePanel::resetToDefaults()
{
    for (const char* id : { P::ENV_ATTACK, P::ENV_AMP_DECAY, P::ENV_AMP_SUSTAIN, P::ENV_RELEASE })
    {
        if (auto* p = apvtsRef.getParameter (id))
        {
            p->beginChangeGesture();
            p->setValueNotifyingHost (p->getDefaultValue());
            p->endChangeGesture();
        }
    }
}

void EnvelopePanel::mouseDown (const juce::MouseEvent& e)
{
    if (resetArea().expanded (3).contains (e.getPosition()))
    {
        resetToDefaults();
        return;
    }
    if (e.y < 18)
    {
        if (auto* on = apvtsRef.getParameter (P::ENV_ENABLED))
        {
            on->beginChangeGesture();
            on->setValueNotifyingHost (on->getValue() > 0.5f ? 0.0f : 1.0f);
            on->endChangeGesture();
        }
    }
}

void EnvelopePanel::mouseMove (const juce::MouseEvent& e)
{
    const bool h = resetArea().expanded (3).contains (e.getPosition());
    if (h != hoverReset) { hoverReset = h; repaint(); }
}

void EnvelopePanel::resized()
{
    auto knobRow = getLocalBounds().reduced (6, 0).removeFromBottom (kKnobRowH);
    const int w = knobRow.getWidth() / (int) adsrKnobs.size();
    for (auto& knob : adsrKnobs)
        knob->setBounds (knobRow.removeFromLeft (w).reduced (1, 2));
}

void EnvelopePanel::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    Aviation::fillGlassScreen (g, r, 6.0f, 0.30f);

    const bool enabled = normValue (apvtsRef, P::ENV_ENABLED) > 0.5f;
    g.setFont (Aviation::label (9.5f, 0.12f));
    g.setColour (enabled ? Aviation::gold().withAlpha (0.92f) : Aviation::gold().withAlpha (0.5f));
    g.drawText ("ENVELOPE", r.toNearestInt().removeFromTop (18), juce::Justification::centred);

    // power LED (title click) + reset glyph
    {
        juce::Rectangle<float> led (8.0f, 6.5f, 5.0f, 5.0f);
        g.setColour (enabled ? Aviation::activeGreen() : juce::Colour (0xff20323f));
        g.fillEllipse (led);
        if (enabled)
        {
            g.setColour (Aviation::activeGreen().withAlpha (0.35f));
            g.drawEllipse (led.expanded (2.0f), 1.0f);
        }
        auto ra = resetArea().toFloat();
        juce::Path arc;
        arc.addCentredArc (ra.getCentreX(), ra.getCentreY(), 4.5f, 4.5f, 0.0f,
                           juce::MathConstants<float>::pi * 0.35f, juce::MathConstants<float>::pi * 2.05f, true);
        g.setColour (hoverReset ? Aviation::cyanBright() : Aviation::textSecondary());
        g.strokePath (arc, juce::PathStrokeType (1.2f));
        juce::Path head;
        head.addTriangle (ra.getCentreX() + 1.5f, ra.getY() + 1.0f, ra.getCentreX() + 5.5f, ra.getY() + 3.0f, ra.getCentreX() + 2.0f, ra.getY() + 5.5f);
        g.fillPath (head);
    }

    auto graph = r.reduced (10.0f).withTrimmedTop (16.0f).withTrimmedBottom ((float) kKnobRowH + 2.0f);

    g.setColour (Aviation::cyan().withAlpha (0.12f));
    g.drawLine (graph.getX(), graph.getCentreY(), graph.getRight(), graph.getCentreY(), 0.5f);

    // ADSR shape from normalized stage values (display proportions)
    const float a = normValue (apvtsRef, P::ENV_ATTACK);
    const float d = normValue (apvtsRef, P::ENV_AMP_DECAY);
    const float s = normValue (apvtsRef, P::ENV_AMP_SUSTAIN);
    const float rel = normValue (apvtsRef, P::ENV_RELEASE);

    const float wA = 0.06f + a * 0.24f;
    const float wD = 0.05f + d * 0.22f;
    const float wR = 0.08f + rel * 0.28f;
    const float wS = juce::jmax (0.08f, 1.0f - wA - wD - wR);
    const float sustainY = graph.getBottom() - graph.getHeight() * (0.12f + s * 0.82f);

    juce::Path env;
    float x = graph.getX();
    env.startNewSubPath (x, graph.getBottom());
    x += graph.getWidth() * wA;
    env.lineTo (x, graph.getY() + 2.0f);
    const float dx = graph.getWidth() * wD;
    env.quadraticTo (x + dx * 0.35f, sustainY - (sustainY - graph.getY()) * 0.15f, x + dx, sustainY);
    x += dx;
    x += graph.getWidth() * wS;
    env.lineTo (x, sustainY);
    env.quadraticTo (x + graph.getWidth() * wR * 0.4f, graph.getBottom() - 2.0f,
                     juce::jmin (x + graph.getWidth() * wR, graph.getRight()), graph.getBottom());

    const float alpha = enabled ? 1.0f : 0.35f;
    g.setColour (Aviation::cyan().withAlpha (0.25f * alpha));
    g.strokePath (env, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved));
    g.setColour (Aviation::cyanBright().withAlpha (alpha));
    g.strokePath (env, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved));
}
