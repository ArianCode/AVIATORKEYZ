#include "VelocityPanel.h"
#include "AviationTheme.h"
#include "../../State/StateSchema.h"

VelocityPanel::VelocityPanel (juce::AudioProcessorValueTreeState& apvts)
    : apvtsRef (apvts)
{
    slider.setAlpha (0.0f);
    slider.setWantsKeyboardFocus (false);
    addAndMakeVisible (slider);
    AviationMini::configureAttachment (apvtsRef, AviatorKeyz::ParamID::VELOCITY_SENSITIVITY,
                                       slider, attachment);
    slider.onValueChange = [this] { repaint(); };
}

void VelocityPanel::resized()
{
    slider.setBounds (getLocalBounds());
}

void VelocityPanel::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    Aviation::fillGlassScreen (g, r, 6.0f, slider.isMouseOverOrDragging() ? 0.45f : 0.28f);

    g.setFont (Aviation::label (9.5f, 0.12f));
    g.setColour (Aviation::gold().withAlpha (0.92f));
    g.drawText ("VELOCITY CURVE", r.toNearestInt().removeFromTop (18), juce::Justification::centred);

    auto graph = r.reduced (10.0f).withTrimmedTop (16.0f);

    // faint grid
    g.setColour (Aviation::cyan().withAlpha (0.12f));
    for (int i = 1; i < 4; ++i)
    {
        const float fx = graph.getX() + graph.getWidth() * (float) i / 4.0f;
        const float fy = graph.getY() + graph.getHeight() * (float) i / 4.0f;
        g.drawLine (fx, graph.getY(), fx, graph.getBottom(), 0.5f);
        g.drawLine (graph.getX(), fy, graph.getRight(), fy, 0.5f);
    }

    // response: out = (1 - s) + s * v
    const float s = (float) slider.getValue();
    juce::Path curve;
    for (int i = 0; i <= 40; ++i)
    {
        const float v = (float) i / 40.0f;
        const float out = (1.0f - s) + s * v;
        const juce::Point<float> pt (graph.getX() + graph.getWidth() * v,
                                     graph.getBottom() - graph.getHeight() * out);
        if (i == 0) curve.startNewSubPath (pt);
        else        curve.lineTo (pt);
    }

    // glow + line
    g.setColour (Aviation::cyan().withAlpha (0.25f));
    g.strokePath (curve, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved));
    g.setColour (Aviation::cyanBright());
    g.strokePath (curve, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved));
}
