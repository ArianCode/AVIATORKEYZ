#include "FilterPanel.h"
#include "AviationTheme.h"
#include "../../State/StateSchema.h"

namespace
{
namespace P = AviatorKeyz::ParamID;

float paramValue (juce::AudioProcessorValueTreeState& apvts, const char* id, float fallback)
{
    if (auto* raw = apvts.getRawParameterValue (id))
        return raw->load();
    return fallback;
}
} // namespace

FilterPanel::FilterPanel (juce::AudioProcessorValueTreeState& apvts)
    : apvtsRef (apvts)
{
    cutoffParam = apvtsRef.getParameter (P::FILTER_CUTOFF);
    resoParam   = apvtsRef.getParameter (P::FILTER_RESONANCE);

    for (const char* id : { P::FILTER_CUTOFF, P::FILTER_RESONANCE, P::FILTER_TYPE, P::FILTER_ENABLED })
        apvtsRef.addParameterListener (id, this);
}

FilterPanel::~FilterPanel()
{
    for (const char* id : { P::FILTER_CUTOFF, P::FILTER_RESONANCE, P::FILTER_TYPE, P::FILTER_ENABLED })
        apvtsRef.removeParameterListener (id, this);
}

juce::Rectangle<float> FilterPanel::graphArea() const
{
    return getLocalBounds().toFloat().reduced (10.0f).withTrimmedTop (16.0f);
}

void FilterPanel::applyDrag (juce::Point<float> pos)
{
    const auto graph = graphArea();
    if (cutoffParam != nullptr)
    {
        const float xNorm = juce::jlimit (0.0f, 1.0f, (pos.x - graph.getX()) / graph.getWidth());
        // log 20..20000 Hz mapped through the parameter's own range
        const float hz = 20.0f * std::pow (1000.0f, xNorm);
        cutoffParam->setValueNotifyingHost (cutoffParam->convertTo0to1 (hz));
    }
    if (resoParam != nullptr)
    {
        const float yNorm = juce::jlimit (0.0f, 1.0f, 1.0f - (pos.y - graph.getY()) / graph.getHeight());
        resoParam->setValueNotifyingHost (resoParam->convertTo0to1 (yNorm));
    }
}

void FilterPanel::mouseDown (const juce::MouseEvent& e)
{
    // Title zone toggles the filter on/off.
    if (e.y < 18)
    {
        if (auto* enabled = apvtsRef.getParameter (P::FILTER_ENABLED))
        {
            enabled->beginChangeGesture();
            enabled->setValueNotifyingHost (enabled->getValue() > 0.5f ? 0.0f : 1.0f);
            enabled->endChangeGesture();
        }
        return;
    }

    dragging = true;
    if (cutoffParam) cutoffParam->beginChangeGesture();
    if (resoParam)   resoParam->beginChangeGesture();
    applyDrag (e.position);
}

void FilterPanel::mouseDrag (const juce::MouseEvent& e)
{
    if (dragging)
        applyDrag (e.position);
}

void FilterPanel::mouseUp (const juce::MouseEvent&)
{
    if (dragging)
    {
        if (cutoffParam) cutoffParam->endChangeGesture();
        if (resoParam)   resoParam->endChangeGesture();
        dragging = false;
    }
}

void FilterPanel::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    Aviation::fillGlassScreen (g, r, 6.0f, 0.30f);

    const bool enabled = paramValue (apvtsRef, P::FILTER_ENABLED, 0.0f) > 0.5f;

    g.setFont (Aviation::label (9.5f, 0.12f));
    g.setColour (enabled ? Aviation::gold() : Aviation::gold().withAlpha (0.5f));
    g.drawText ("FILTER", r.toNearestInt().removeFromTop (18), juce::Justification::centred);

    const auto graph = graphArea();

    g.setColour (Aviation::cyan().withAlpha (0.12f));
    for (int i = 1; i < 4; ++i)
    {
        const float fx = graph.getX() + graph.getWidth() * (float) i / 4.0f;
        g.drawLine (fx, graph.getY(), fx, graph.getBottom(), 0.5f);
    }
    g.drawLine (graph.getX(), graph.getCentreY(), graph.getRight(), graph.getCentreY(), 0.5f);

    // Response curve sketch from the real parameter values.
    const float cutoffHz = paramValue (apvtsRef, P::FILTER_CUTOFF, 8000.0f);
    const float reso     = paramValue (apvtsRef, P::FILTER_RESONANCE, 0.25f);
    const int   type     = (int) paramValue (apvtsRef, P::FILTER_TYPE, 0.0f);

    const float cutoffX = graph.getX()
        + graph.getWidth() * (std::log (juce::jlimit (20.0f, 20000.0f, cutoffHz) / 20.0f) / std::log (1000.0f));
    const float baseY  = graph.getY() + graph.getHeight() * 0.35f;
    const float peak   = graph.getHeight() * 0.30f * reso;

    juce::Path curve;
    const float bottom = graph.getBottom() - 2.0f;
    switch (type)
    {
        case 1: // HP
            curve.startNewSubPath (graph.getX(), bottom);
            curve.quadraticTo (cutoffX - 14.0f, bottom, cutoffX, baseY - peak * 0.5f);
            curve.quadraticTo (cutoffX + 10.0f, baseY - peak, cutoffX + 22.0f, baseY);
            curve.lineTo (graph.getRight(), baseY);
            break;
        case 2: // BP
            curve.startNewSubPath (graph.getX(), bottom);
            curve.quadraticTo (cutoffX - 26.0f, bottom, cutoffX, baseY - peak);
            curve.quadraticTo (cutoffX + 26.0f, bottom, graph.getRight(), bottom);
            break;
        case 3: // Notch
            curve.startNewSubPath (graph.getX(), baseY);
            curve.lineTo (cutoffX - 22.0f, baseY);
            curve.quadraticTo (cutoffX, bottom, cutoffX + 22.0f, baseY);
            curve.lineTo (graph.getRight(), baseY);
            break;
        default: // LP
            curve.startNewSubPath (graph.getX(), baseY);
            curve.lineTo (cutoffX - 22.0f, baseY);
            curve.quadraticTo (cutoffX - 6.0f, baseY - peak, cutoffX + 4.0f, baseY - peak * 0.4f);
            curve.quadraticTo (cutoffX + 14.0f, baseY + (bottom - baseY) * 0.55f, cutoffX + 30.0f, bottom);
            break;
    }

    const float alpha = enabled ? 1.0f : 0.35f;
    g.setColour (Aviation::cyan().withAlpha (0.25f * alpha));
    g.strokePath (curve, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved));
    g.setColour (Aviation::cyanBright().withAlpha (alpha));
    g.strokePath (curve, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved));
}
