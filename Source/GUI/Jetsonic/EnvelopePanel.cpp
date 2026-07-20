#include "EnvelopePanel.h"
#include "JetsonicTheme.h"
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
    for (const auto& [id, label] : specs)
    {
        auto knob = std::make_unique<MiniRotary> (apvtsRef, id, label, false);
        addAndMakeVisible (*knob);
        adsrKnobs.push_back (std::move (knob));
        apvtsRef.addParameterListener (id, this);
    }
}

EnvelopePanel::~EnvelopePanel()
{
    for (const char* id : { P::ENV_ATTACK, P::ENV_AMP_DECAY, P::ENV_AMP_SUSTAIN, P::ENV_RELEASE })
        apvtsRef.removeParameterListener (id, this);
}

void EnvelopePanel::resized()
{
    auto knobRow = getLocalBounds().reduced (8, 0).removeFromBottom (44);
    const int w = knobRow.getWidth() / (int) adsrKnobs.size();
    for (auto& knob : adsrKnobs)
        knob->setBounds (knobRow.removeFromLeft (w).reduced (3, 2));
}

void EnvelopePanel::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    Jetsonic::fillGlassScreen (g, r, 6.0f, 0.30f);

    g.setFont (Jetsonic::label (9.5f, 0.12f));
    g.setColour (Jetsonic::gold().withAlpha (0.92f));
    g.drawText ("ENVELOPE", r.toNearestInt().removeFromTop (18), juce::Justification::centred);

    auto graph = r.reduced (10.0f).withTrimmedTop (16.0f).withTrimmedBottom (46.0f);

    g.setColour (Jetsonic::cyan().withAlpha (0.12f));
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

    g.setColour (Jetsonic::cyan().withAlpha (0.25f));
    g.strokePath (env, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved));
    g.setColour (Jetsonic::cyanBright());
    g.strokePath (env, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved));
}
