#include "AviationGauge.h"

namespace
{
constexpr float kStartDeg = 225.f;
constexpr float kEndDeg   = -45.f;
} // namespace

AviationGauge::AviationGauge (juce::AudioProcessorValueTreeState& apvts,
                              const juce::String& id,
                              const juce::String& gaugeName,
                              const juce::String& destinationHint,
                              ValueFormat format,
                              bool freezeToggleMode)
    : apvtsRef (apvts)
    , paramId (id)
    , nameText (gaugeName.toUpperCase())
    , hintText (destinationHint)
    , valueFormat (format)
    , freezeToggle (freezeToggleMode)
{
    setOpaque (false);
    hiddenSlider.setSliderStyle (juce::Slider::LinearBarVertical);
    hiddenSlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    hiddenSlider.setVisible (false);
    addChildComponent (hiddenSlider);

    attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, id, hiddenSlider);
    apvtsRef.addParameterListener (paramId, this);
    syncNeedleTarget();
    needleAngle = targetAngle;
    startTimerHz (45);
}

AviationGauge::~AviationGauge()
{
    apvtsRef.removeParameterListener (paramId, this);
}

float AviationGauge::needleAngleForNormalised (float norm) const
{
    const float deg = juce::jmap (juce::jlimit (0.f, 1.f, norm), 0.f, 1.f, kStartDeg, kEndDeg);
    return juce::degreesToRadians (deg - 90.f);
}

float AviationGauge::readNormalised() const
{
    if (freezeToggle)
    {
        if (auto* raw = apvtsRef.getRawParameterValue (paramId))
            return raw->load() > 0.5f ? 1.f : 0.f;
        return 0.f;
    }

    if (auto* p = dynamic_cast<juce::RangedAudioParameter*> (apvtsRef.getParameter (paramId)))
        return p->getValue();

    return (float) hiddenSlider.getValue();
}

void AviationGauge::syncNeedleTarget()
{
    targetAngle = needleAngleForNormalised (readNormalised());
}

void AviationGauge::parameterChanged (const juce::String& id, float)
{
    if (id != paramId)
        return;

    juce::MessageManager::callAsync ([self = juce::Component::SafePointer<AviationGauge> (this)]
    {
        if (self != nullptr)
            self->syncNeedleTarget();
    });
}

void AviationGauge::timerCallback()
{
    if (freezeToggle)
    {
        needleAngle = targetAngle;
        repaint();
        return;
    }

    const float delta = targetAngle - needleAngle;
    if (std::abs (delta) > 0.0005f)
    {
        needleAngle += delta * 0.22f;
        repaint();
    }
}

juce::String AviationGauge::formatValueText() const
{
    if (freezeToggle)
        return readNormalised() > 0.5f ? "ON" : "OFF";

    if (auto* p = dynamic_cast<juce::RangedAudioParameter*> (apvtsRef.getParameter (paramId)))
    {
        const float v = p->convertFrom0to1 (p->getValue());
        switch (valueFormat)
        {
            case ValueFormat::glideSeconds:
                return v >= 1000.f ? juce::String (v / 1000.f, 2) + "s"
                                   : juce::String (juce::roundToInt (v)) + "ms";
            case ValueFormat::toneDb:
                return (v >= 0.f ? "+" : "") + juce::String (v, 1);
            case ValueFormat::decibels:
                return (v >= 0.f ? "+" : "") + juce::String (v, 1) + "dB";
            case ValueFormat::stereoWidth:
                return juce::String (juce::roundToInt (v * 100.f)) + "%";
            case ValueFormat::percent:
            default:
            {
                const float norm = p->getValue();
                return juce::String (juce::roundToInt (norm * 100.f)) + "%";
            }
        }
    }

    return juce::String (juce::roundToInt (readNormalised() * 100.f)) + "%";
}

void AviationGauge::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds();
    const int lineH = juce::jmax (9, bounds.getHeight() / 10);
    const int nameH = lineH;
    const int valueH = lineH;
    const int hintH = lineH;
    const int textBlockH = nameH + valueH + hintH + 2;
    const int dialSize = juce::jmin (bounds.getWidth(), bounds.getHeight() - textBlockH);
    const juce::Rectangle<int> dialArea (bounds.getCentreX() - dialSize / 2,
                                         bounds.getY(),
                                         dialSize,
                                         dialSize);

    const float cx = dialArea.getCentreX();
    const float cy = dialArea.getCentreY();
    const float r  = (float) dialArea.getWidth() * 0.5f - 2.f;
    const bool frozen = freezeToggle && readNormalised() > 0.5f;

    if (frozen)
    {
        g.setColour (AviatorTokens::champagneGold().withAlpha (0.35f));
        g.drawEllipse (cx - r - 3.f, cy - r - 3.f, (r + 3.f) * 2.f, (r + 3.f) * 2.f, 3.f);
    }

    juce::ColourGradient faceGrad (juce::Colour (0xff0a1e3d), cx, cy,
                                   juce::Colour (0xff020408), cx, cy - r, true);
    g.setGradientFill (faceGrad);
    g.fillEllipse (cx - r, cy - r, r * 2.f, r * 2.f);

    g.setColour (AviatorTokens::champagneGold().withAlpha (0.85f));
    g.drawEllipse (cx - r, cy - r, r * 2.f, r * 2.f, 2.5f);

    g.setColour (AviatorTokens::champagneGold().withAlpha (0.25f));
    for (int i = 0; i <= 10; ++i)
    {
        const float t = (float) i / 10.f;
        const float deg = juce::jmap (t, 0.f, 1.f, kStartDeg, kEndDeg);
        const float a = juce::degreesToRadians (deg - 90.f);
        const float inner = r - 6.f;
        const float outer = r - 2.f;
        g.drawLine (cx + std::cos (a) * inner, cy + std::sin (a) * inner,
                    cx + std::cos (a) * outer, cy + std::sin (a) * outer, i % 5 == 0 ? 1.4f : 0.8f);
    }

    const float nx = cx + std::cos (needleAngle) * (r - 10.f);
    const float ny = cy + std::sin (needleAngle) * (r - 10.f);
    g.setColour (AviatorTokens::champagneGold());
    g.drawLine (cx, cy, nx, ny, 1.6f);
    g.fillEllipse (cx - 2.5f, cy - 2.5f, 5.f, 5.f);

    int textY = dialArea.getBottom();
    g.setFont (AviatorTokens::hudBold (8.f));
    g.setColour (AviatorTokens::champagneGold().withAlpha (0.9f));
    g.drawText (nameText, bounds.getX(), textY, bounds.getWidth(), nameH, juce::Justification::centred);
    textY += nameH;

    g.setFont (AviatorTokens::hud (8.f));
    g.setColour (juce::Colours::white.withAlpha (0.9f));
    g.drawText (formatValueText(), bounds.getX(), textY, bounds.getWidth(), valueH, juce::Justification::centred);
    textY += valueH;

    g.setFont (AviatorTokens::hud (7.f));
    g.setColour (AviatorTokens::instrumentCyan().withAlpha (0.85f));
    g.drawText (hintText, bounds.getX(), textY, bounds.getWidth(), hintH, juce::Justification::centred);
}

void AviationGauge::mouseDown (const juce::MouseEvent& e)
{
    if (freezeToggle)
    {
        if (auto* p = apvtsRef.getParameter (paramId))
            p->setValueNotifyingHost (readNormalised() > 0.5f ? 0.f : 1.f);
        syncNeedleTarget();
        needleAngle = targetAngle;
        repaint();
        return;
    }

    dragStartNorm = readNormalised();
    dragStartY = e.y;
}

void AviationGauge::mouseDrag (const juce::MouseEvent& e)
{
    if (freezeToggle)
        return;

    const float delta = (float) (dragStartY - e.y) * 0.006f;
    const float next = juce::jlimit (0.f, 1.f, dragStartNorm + delta);

    if (auto* p = apvtsRef.getParameter (paramId))
        p->setValueNotifyingHost (next);
}

void AviationGauge::mouseUp (const juce::MouseEvent&) {}

void AviationGauge::mouseDoubleClick (const juce::MouseEvent&)
{
    if (freezeToggle)
        return;

    showTextEditor();
}

void AviationGauge::showTextEditor()
{
    auto* ed = new juce::TextEditor();
    ed->setText (formatValueText().retainCharacters ("0123456789.-+"));
    ed->setFont (AviatorTokens::hud (11.f));
    ed->setColour (juce::TextEditor::backgroundColourId, juce::Colour (0xff0a1628));
    ed->setColour (juce::TextEditor::textColourId, AviatorTokens::textPrimary());
    ed->setJustification (juce::Justification::centred);
    ed->setBounds (getLocalBounds());
    ed->onReturnKey = ed->onFocusLost = [this, ed]
    {
        const float typed = ed->getText().getFloatValue();
        if (auto* p = dynamic_cast<juce::RangedAudioParameter*> (apvtsRef.getParameter (paramId)))
        {
            const auto range = p->getNormalisableRange();
            p->setValueNotifyingHost (range.convertTo0to1 (typed));
        }
        removeChildComponent (ed);
        delete ed;
    };
    addAndMakeVisible (ed);
    ed->grabKeyboardFocus();
    ed->selectAll();
}
