#include "FilterPanel.h"
#include "AviationTheme.h"
#include "../../State/StateSchema.h"

namespace
{
namespace P = AviatorKeyz::ParamID;

constexpr int kTypeLP = 0;
constexpr int kTypeHP = 1;
constexpr int kChipW = 26;
constexpr int kChipH = 13;

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
    return getLocalBounds().toFloat().reduced (10.0f).withTrimmedTop (16.0f).withTrimmedBottom (14.0f);
}

juce::Rectangle<int> FilterPanel::hpChipArea() const
{
    return { 8, getHeight() - kChipH - 5, kChipW, kChipH };
}

juce::Rectangle<int> FilterPanel::lpChipArea() const
{
    return { 8 + kChipW + 4, getHeight() - kChipH - 5, kChipW, kChipH };
}

void FilterPanel::setEnabledParam (bool on)
{
    if (auto* enabled = apvtsRef.getParameter (P::FILTER_ENABLED))
    {
        if ((enabled->getValue() > 0.5f) == on)
            return;
        enabled->beginChangeGesture();
        enabled->setValueNotifyingHost (on ? 1.0f : 0.0f);
        enabled->endChangeGesture();
    }
}

void FilterPanel::setTypeParam (int typeIndex)
{
    if (auto* type = apvtsRef.getParameter (P::FILTER_TYPE))
    {
        type->beginChangeGesture();
        type->setValueNotifyingHost (type->convertTo0to1 ((float) typeIndex));
        type->endChangeGesture();
    }
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
        const bool on = paramValue (apvtsRef, P::FILTER_ENABLED, 0.0f) > 0.5f;
        setEnabledParam (! on);
        return;
    }

    // HP / LP mode chips — selecting a mode also switches the filter on.
    if (hpChipArea().contains (e.getPosition()))
    {
        setTypeParam (kTypeHP);
        setEnabledParam (true);
        return;
    }
    if (lpChipArea().contains (e.getPosition()))
    {
        setTypeParam (kTypeLP);
        setEnabledParam (true);
        return;
    }

    // Dragging the curve engages the filter; a right / alt drag flips HP <-> LP
    // so the alternate slope is one gesture away.
    const int type = (int) paramValue (apvtsRef, P::FILTER_TYPE, (float) kTypeHP);
    if (e.mods.isRightButtonDown() || e.mods.isAltDown())
        setTypeParam (type == kTypeHP ? kTypeLP : kTypeHP);
    else if (type != kTypeHP && type != kTypeLP)
        setTypeParam (kTypeHP); // BP / Notch only exist on the advanced page

    setEnabledParam (true);

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

void FilterPanel::mouseMove (const juce::MouseEvent& e)
{
    const int chip = hpChipArea().contains (e.getPosition()) ? 0
                   : lpChipArea().contains (e.getPosition()) ? 1 : -1;
    if (chip != hoveredChip)
    {
        hoveredChip = chip;
        repaint();
    }
}

void FilterPanel::mouseExit (const juce::MouseEvent&)
{
    if (hoveredChip != -1)
    {
        hoveredChip = -1;
        repaint();
    }
}

void FilterPanel::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    Aviation::fillGlassScreen (g, r, 6.0f, 0.30f);

    const bool enabled = paramValue (apvtsRef, P::FILTER_ENABLED, 0.0f) > 0.5f;
    const int  type    = (int) paramValue (apvtsRef, P::FILTER_TYPE, (float) kTypeHP);

    g.setFont (Aviation::label (9.5f, 0.12f));
    g.setColour (enabled ? Aviation::gold() : Aviation::gold().withAlpha (0.5f));
    g.drawText ("FILTER", r.toNearestInt().removeFromTop (18), juce::Justification::centred);

    // power LED next to the title
    {
        const float d = 5.0f;
        juce::Rectangle<float> led (r.getRight() - 14.0f, 6.5f, d, d);
        g.setColour (enabled ? Aviation::activeGreen() : juce::Colour (0xff20323f));
        g.fillEllipse (led);
        if (enabled)
        {
            g.setColour (Aviation::activeGreen().withAlpha (0.35f));
            g.drawEllipse (led.expanded (2.0f), 1.0f);
        }
    }

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

    const float cutoffX = graph.getX()
        + graph.getWidth() * (std::log (juce::jlimit (20.0f, 20000.0f, cutoffHz) / 20.0f) / std::log (1000.0f));
    const float baseY  = graph.getY() + graph.getHeight() * 0.35f;
    const float peak   = graph.getHeight() * 0.30f * reso;

    juce::Path curve;
    const float bottom = graph.getBottom() - 2.0f;
    switch (type)
    {
        case kTypeHP:
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

    // cutoff marker
    g.setColour (Aviation::gold().withAlpha (0.55f * alpha));
    g.drawLine (cutoffX, graph.getY(), cutoffX, graph.getBottom(), 0.6f);

    // HP / LP chips
    auto drawChip = [&] (juce::Rectangle<int> area, const char* text, bool on, bool hover)
    {
        auto a = area.toFloat();
        g.setColour (on ? juce::Colour (0xff12293b) : juce::Colour (0xff081420));
        g.fillRoundedRectangle (a, 2.5f);
        g.setColour (on || hover ? Aviation::cyan().withAlpha (0.9f) : juce::Colour (0xff1a2e40));
        g.drawRoundedRectangle (a, 2.5f, 1.0f);
        g.setFont (Aviation::label (7.5f, 0.16f));
        g.setColour (on ? Aviation::cyanBright() : (hover ? Aviation::textPrimary() : Aviation::textDim()));
        g.drawText (text, area, juce::Justification::centred);
    };
    drawChip (hpChipArea(), "HP", type == kTypeHP, hoveredChip == 0);
    drawChip (lpChipArea(), "LP", type == kTypeLP, hoveredChip == 1);

    // cutoff readout
    g.setFont (Aviation::value (8.5f));
    g.setColour (Aviation::textSecondary());
    const juce::String hzText = cutoffHz >= 1000.0f ? juce::String (cutoffHz / 1000.0f, 1) + "k"
                                                    : juce::String ((int) cutoffHz);
    g.drawText (hzText + " Hz", getWidth() - 50, getHeight() - kChipH - 5, 42, kChipH, juce::Justification::centredRight);
}
