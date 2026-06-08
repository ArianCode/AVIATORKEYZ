#include "FilterCurveGraph.h"
#include "AdvancedWidgets.h"

FilterCurveGraph::FilterCurveGraph (juce::AudioProcessorValueTreeState& apvts,
                                    const char* cutoffParamId,
                                    const char* resonanceParamId,
                                    const char* typeParamId)
    : apvtsRef (apvts)
    , cutoffId (cutoffParamId)
    , resonanceId (resonanceParamId)
    , typeId (typeParamId)
{
    apvtsRef.addParameterListener (cutoffId, this);
    apvtsRef.addParameterListener (resonanceId, this);
    apvtsRef.addParameterListener (typeId, this);
}

FilterCurveGraph::~FilterCurveGraph()
{
    apvtsRef.removeParameterListener (cutoffId, this);
    apvtsRef.removeParameterListener (resonanceId, this);
    apvtsRef.removeParameterListener (typeId, this);
}

void FilterCurveGraph::parameterChanged (const juce::String&, float)
{
    repaint();
}

juce::Rectangle<float> FilterCurveGraph::plotArea() const
{
    return getLocalBounds().toFloat().reduced (6.f, 4.f).withTrimmedBottom (14.f);
}

float FilterCurveGraph::readCutoffHz() const
{
    if (auto* raw = apvtsRef.getRawParameterValue (cutoffId))
        return raw->load();
    return 8000.f;
}

float FilterCurveGraph::readResonance() const
{
    if (auto* raw = apvtsRef.getRawParameterValue (resonanceId))
        return raw->load();
    return 0.25f;
}

int FilterCurveGraph::readFilterType() const
{
    if (auto* p = dynamic_cast<juce::AudioParameterChoice*> (apvtsRef.getParameter (typeId)))
        return p->getIndex();
    return 0;
}

float FilterCurveGraph::resonanceToQ (float reso) const
{
    return 0.707f + reso * reso * 14.f;
}

float FilterCurveGraph::magnitudeDb (float freqHz, float cutoffHz, float Q, int filterType) const
{
    const float fc = juce::jmax (1.f, cutoffHz);
    const float w  = juce::jmax (0.001f, freqHz / fc);
    const float w2 = w * w;
    const float denom = (1.f - w2) * (1.f - w2) + (w / juce::jmax (0.01f, Q)) * (w / juce::jmax (0.01f, Q));

    float gain = 1.f;
    switch (filterType)
    {
        case 1: gain = w2 / std::sqrt (denom); break; // HP
        case 2: gain = (w / Q) / std::sqrt (denom); break; // BP
        case 3: gain = std::abs (1.f - w2) / std::sqrt (denom); break; // Notch
        default: gain = 1.f / std::sqrt (denom); break; // LP
    }

    return juce::Decibels::gainToDecibels (gain, kMinDb);
}

float FilterCurveGraph::freqToX (float hz, juce::Rectangle<float> plot) const
{
    const float logMin = std::log10 (kMinHz);
    const float logMax = std::log10 (kMaxHz);
    const float logF   = std::log10 (juce::jlimit (kMinHz, kMaxHz, hz));
    return plot.getX() + (logF - logMin) / (logMax - logMin) * plot.getWidth();
}

float FilterCurveGraph::xToFreq (float x, juce::Rectangle<float> plot) const
{
    const float logMin = std::log10 (kMinHz);
    const float logMax = std::log10 (kMaxHz);
    const float t = juce::jlimit (0.f, 1.f, (x - plot.getX()) / juce::jmax (1.f, plot.getWidth()));
    return std::pow (10.f, logMin + t * (logMax - logMin));
}

float FilterCurveGraph::dbToY (float db, juce::Rectangle<float> plot) const
{
    const float norm = juce::jlimit (0.f, 1.f, (db - kMinDb) / (kMaxDb - kMinDb));
    return plot.getBottom() - norm * plot.getHeight();
}

juce::Path FilterCurveGraph::buildResponsePath (juce::Rectangle<float> plot) const
{
    const float cutoff = readCutoffHz();
    const float Q      = resonanceToQ (readResonance());
    const int type     = readFilterType();

    juce::Path path;
    const int steps = juce::jmax (64, getWidth() * 2);

    for (int i = 0; i <= steps; ++i)
    {
        const float t    = static_cast<float> (i) / static_cast<float> (steps);
        const float freq = kMinHz * std::pow (kMaxHz / kMinHz, t);
        const float x    = plot.getX() + t * plot.getWidth();
        const float y    = dbToY (magnitudeDb (freq, cutoff, Q, type), plot);

        if (i == 0)
            path.startNewSubPath (x, y);
        else
            path.lineTo (x, y);
    }

    return path;
}

juce::Point<float> FilterCurveGraph::handlePosition (juce::Rectangle<float> plot) const
{
    const float cutoff = readCutoffHz();
    const float Q      = resonanceToQ (readResonance());
    const int type     = readFilterType();
    return { freqToX (cutoff, plot),
             dbToY (magnitudeDb (cutoff, cutoff, Q, type), plot) };
}

void FilterCurveGraph::setCutoffFromFreq (float hz)
{
    if (auto* p = apvtsRef.getParameter (cutoffId))
    {
        const float clamped = juce::jlimit (kMinHz, kMaxHz, hz);
        p->setValueNotifyingHost (p->getNormalisableRange().convertTo0to1 (clamped));
    }
}

void FilterCurveGraph::setResonanceNorm (float norm)
{
    if (auto* p = apvtsRef.getParameter (resonanceId))
        p->setValueNotifyingHost (juce::jlimit (0.f, 1.f, norm));
}

void FilterCurveGraph::applyDrag (juce::Point<float> pos)
{
    const auto plot = plotArea();
    setCutoffFromFreq (xToFreq (pos.x, plot));

    const float resoNorm = 1.f - (pos.y - plot.getY()) / juce::jmax (1.f, plot.getHeight());
    setResonanceNorm (resoNorm);
}

juce::String FilterCurveGraph::formatCutoff (float hz) const
{
    if (hz >= 1000.f)
        return juce::String (hz / 1000.f, 1) + "k";
    return juce::String (juce::roundToInt (hz));
}

juce::String FilterCurveGraph::formatResonance (float reso) const
{
    return juce::String (juce::roundToInt (reso * 100.f)) + "%";
}

void FilterCurveGraph::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    const auto plot   = plotArea();
    const auto gold   = AviatorTokens::champagneGold();
    const auto cyan   = AdvancedWidgets::kCyanWave();

    g.setColour (juce::Colour (0xff060d1a));
    g.fillRoundedRectangle (bounds.reduced (0.5f), 3.f);
    g.setColour (cyan.withAlpha (0.18f));
    g.drawRoundedRectangle (bounds.reduced (0.5f), 3.f, 0.75f);

    // Subtle log-frequency grid
    g.setColour (juce::Colour (0xff142840).withAlpha (0.65f));
    for (const float hz : { 100.f, 1000.f, 10000.f })
    {
        const float x = freqToX (hz, plot);
        g.drawVerticalLine (juce::roundToInt (x), plot.getY(), plot.getBottom());
    }
    for (const float db : { -24.f, -12.f, 0.f })
    {
        const float y = dbToY (db, plot);
        g.drawHorizontalLine (juce::roundToInt (y), plot.getX(), plot.getRight());
    }

    const auto curve = buildResponsePath (plot);

    // Fill under curve
    juce::Path fill = curve;
    fill.lineTo (plot.getRight(), plot.getBottom());
    fill.lineTo (plot.getX(), plot.getBottom());
    fill.closeSubPath();

    juce::ColourGradient fillGrad (gold.withAlpha (0.14f), plot.getX(), plot.getY(),
                                   gold.withAlpha (0.f), plot.getX(), plot.getBottom(), false);
    g.setGradientFill (fillGrad);
    g.fillPath (fill);

    // Glow layers
    for (int layer = 3; layer >= 0; --layer)
    {
        g.setColour (gold.withAlpha (0.06f + 0.04f * (float) layer));
        g.strokePath (curve, juce::PathStrokeType (1.2f + (float) layer * 2.2f,
                                                    juce::PathStrokeType::curved,
                                                    juce::PathStrokeType::rounded));
    }

    g.setColour (gold.withAlpha (0.92f));
    g.strokePath (curve, juce::PathStrokeType (1.4f,
                                                juce::PathStrokeType::curved,
                                                juce::PathStrokeType::rounded));

    const auto handle = handlePosition (plot);
    const float r = dragging ? 7.f : 5.5f;

    g.setColour (gold.withAlpha (0.25f));
    g.fillEllipse (handle.x - r * 1.8f, handle.y - r * 1.8f, r * 3.6f, r * 3.6f);
    g.setColour (juce::Colours::white.withAlpha (0.95f));
    g.fillEllipse (handle.x - r, handle.y - r, r * 2.f, r * 2.f);
    g.setColour (gold);
    g.drawEllipse (handle.x - r, handle.y - r, r * 2.f, r * 2.f, 1.f);

    g.setFont (AviatorTokens::hud (8.f));
    g.setColour (AviatorTokens::textMuted());
    g.drawText ("20", juce::Rectangle<int> (juce::roundToInt (plot.getX()),
                                            juce::roundToInt (plot.getBottom() + 2.f), 20, 10),
                juce::Justification::topLeft);
    g.drawText ("20k", juce::Rectangle<int> (juce::roundToInt (plot.getRight() - 22.f),
                                             juce::roundToInt (plot.getBottom() + 2.f), 22, 10),
                juce::Justification::topLeft);

    g.setColour (cyan);
    g.drawText (formatCutoff (readCutoffHz()) + "  " + formatResonance (readResonance()),
                getLocalBounds().removeFromBottom (12),
                juce::Justification::centred);
}

void FilterCurveGraph::mouseDown (const juce::MouseEvent& e)
{
    dragging = plotArea().contains (e.position);
    if (dragging)
        applyDrag (e.position);
}

void FilterCurveGraph::mouseDrag (const juce::MouseEvent& e)
{
    if (dragging)
        applyDrag (e.position);
}

void FilterCurveGraph::mouseUp (const juce::MouseEvent&)
{
    dragging = false;
    repaint();
}
