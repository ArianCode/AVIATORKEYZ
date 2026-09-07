#include "DeckWidgets.h"

// =============================================================================
//  Deck painters
// =============================================================================
namespace Deck
{
void paintZone (juce::Graphics& g, juce::Rectangle<int> bounds,
                const juce::String& title, const juce::String& tag)
{
    auto r = bounds.toFloat();

    // drop shadow
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.fillRoundedRectangle (r.translated (0.0f, 6.0f).expanded (1.0f), 9.0f);

    juce::ColourGradient grad (juce::Colour (0xee0a1622), r.getX(), r.getY(),
                               juce::Colour (0xee060e17), r.getX(), r.getBottom(), false);
    g.setGradientFill (grad);
    g.fillRoundedRectangle (r, 8.0f);
    g.setColour (zoneBorder());
    g.drawRoundedRectangle (r, 8.0f, 1.0f);
    g.setColour (juce::Colours::white.withAlpha (0.05f));
    g.fillRect (juce::Rectangle<float> (r.getX() + 3.0f, r.getY() + 1.0f, r.getWidth() - 6.0f, 1.0f));

    auto head = bounds.removeFromTop (kZoneHeaderH);
    g.setColour (juce::Colour (0xff12222f));
    g.fillRect (head.getX() + 1, head.getBottom() - 1, head.getWidth() - 2, 1);

    auto inner = head.reduced (14, 0);
    g.setFont (Aviation::label (11.0f, 0.28f));
    g.setColour (Aviation::gold());
    g.drawText (title, inner, juce::Justification::centredLeft);

    g.setFont (mono (8.5f));
    g.setColour (Aviation::textDim());
    g.drawText (tag, inner, juce::Justification::centredRight);
}

void paintScreen (juce::Graphics& g, juce::Rectangle<float> r)
{
    g.setColour (screenBg());
    g.fillRoundedRectangle (r, 6.0f);
    g.setColour (juce::Colour (0xff10222f));
    g.drawRoundedRectangle (r, 6.0f, 1.0f);
}

void paintPad (juce::Graphics& g, juce::Rectangle<float> r, bool lit,
               juce::Colour accent, bool hover, bool greenTint)
{
    const float lift = lit ? 1.0f : 4.0f;
    auto face = r.withTrimmedBottom (4.0f).translated (0.0f, lit ? 3.0f : 0.0f);

    // base / shadow
    g.setColour (juce::Colour (0xff030910));
    g.fillRoundedRectangle (face.translated (0.0f, lift), 8.0f);
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.fillRoundedRectangle (face.translated (0.0f, lift + 3.0f).expanded (1.0f), 9.0f);

    juce::Colour top = lit ? (greenTint ? juce::Colour (0xff1d3d2b) : juce::Colour (0xff173049))
                           : juce::Colour (0xff122334);
    juce::Colour bottom = lit ? (greenTint ? juce::Colour (0xff0a1a12) : juce::Colour (0xff0b1c2e))
                              : juce::Colour (0xff081320);
    juce::ColourGradient grad (top, face.getCentreX(), face.getY(), bottom, face.getCentreX(), face.getBottom(), false);
    g.setGradientFill (grad);
    g.fillRoundedRectangle (face, 8.0f);

    g.setColour (lit ? accent : (hover ? accent.withAlpha (0.55f) : padBorder()));
    g.drawRoundedRectangle (face, 8.0f, 1.0f);
    g.setColour (juce::Colours::white.withAlpha (0.08f));
    g.fillRect (juce::Rectangle<float> (face.getX() + 4.0f, face.getY() + 1.0f, face.getWidth() - 8.0f, 1.0f));
}

juce::String noteName (int midiNote)
{
    static const char* names[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    midiNote = juce::jlimit (0, 127, midiNote);
    return juce::String (names[midiNote % 12]) + juce::String (midiNote / 12 - 1);
}
} // namespace Deck

// =============================================================================
//  DeckKnob
// =============================================================================
DeckKnob::DeckKnob (juce::AudioProcessorValueTreeState& apvts,
                    const juce::String& id,
                    const juce::String& label,
                    int diam,
                    Format fmt,
                    bool gold,
                    const juce::String& enableParamId)
    : apvtsRef (apvts), paramId (id), labelText (label), enableId (enableParamId),
      diameter (diam), format (fmt), isGold (gold)
{
    slider.setAlpha (0.0f);
    slider.setWantsKeyboardFocus (false);
    addAndMakeVisible (slider);
    AviationMini::configureAttachment (apvtsRef, paramId, slider, attachment);
    slider.onValueChange = [this] { repaint(); };

    if (enableId.isNotEmpty())
    {
        if (auto* p = apvtsRef.getParameter (enableId))
        {
            enableAttachment = std::make_unique<juce::ParameterAttachment> (*p, [this] (float v)
            {
                enabledState = v > 0.5f;
                repaint();
            });
            enableAttachment->sendInitialUpdate();
        }
    }
}

DeckKnob::~DeckKnob() = default;

void DeckKnob::resized()
{
    const int d = juce::jmin (diameter, getWidth(), getHeight() - 28);
    slider.setBounds ((getWidth() - d) / 2, 2, d, d);
}

juce::Rectangle<int> DeckKnob::ledArea() const
{
    return { getWidth() - 10, 0, 8, 8 };
}

void DeckKnob::mouseDown (const juce::MouseEvent& e)
{
    if (enableAttachment != nullptr && ledArea().expanded (4).contains (e.getPosition()))
        enableAttachment->setValueAsCompleteGesture (enabledState ? 0.0f : 1.0f);
}

juce::String DeckKnob::valueText() const
{
    auto* p = apvtsRef.getParameter (paramId);
    if (p == nullptr)
        return {};
    const float v = p->convertFrom0to1 (p->getValue());
    switch (format)
    {
        case Format::percent:   return juce::String (juce::roundToInt (v * 100.f)) + "%";
        case Format::raw:       return juce::String (v, 2);
        case Format::ms:        return v >= 1000.f ? juce::String (v / 1000.f, 2) + " s" : juce::String (juce::roundToInt (v)) + " ms";
        case Format::semitones: return (v >= 0.f ? "+" : "") + juce::String (v, 1) + " st";
        case Format::decibels:  return juce::String (v, 1) + " dB";
        case Format::text:
        default:                return p->getCurrentValueAsText();
    }
}

void DeckKnob::paint (juce::Graphics& g)
{
    const auto knob = slider.getBounds().toFloat();
    const float cx = knob.getCentreX();
    const float cy = knob.getCentreY();
    const float radius = knob.getWidth() * 0.5f - 4.0f;

    constexpr float a0 = juce::MathConstants<float>::pi * 1.25f;
    constexpr float a1 = juce::MathConstants<float>::pi * 2.75f;
    const float norm = (float) juce::jmap (slider.getValue(), slider.getMinimum(), slider.getMaximum(), 0.0, 1.0);
    const float angle = a0 + norm * (a1 - a0);
    const bool hover = slider.isMouseOverOrDragging();
    const float dim = enabledState ? 1.0f : 0.45f;

    const auto accent = isGold ? Aviation::gold() : Aviation::cyan();
    const auto pointer = isGold ? Aviation::goldBright() : Aviation::cyanBright();

    // face
    g.setColour (juce::Colour (0xff0a1826));
    g.fillEllipse (cx - radius + 3.0f, cy - radius + 3.0f, (radius - 3.0f) * 2.0f, (radius - 3.0f) * 2.0f);
    g.setColour (juce::Colour (0xff1c3145));
    g.drawEllipse (cx - radius + 3.0f, cy - radius + 3.0f, (radius - 3.0f) * 2.0f, (radius - 3.0f) * 2.0f, 1.0f);

    // track + value arc
    juce::Path track, arc;
    track.addCentredArc (cx, cy, radius, radius, 0.0f, a0, a1, true);
    g.setColour (juce::Colour (0xff122233));
    g.strokePath (track, juce::PathStrokeType (2.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    if (norm > 0.001f)
    {
        arc.addCentredArc (cx, cy, radius, radius, 0.0f, a0, angle, true);
        g.setColour (accent.withAlpha ((hover ? 1.0f : 0.85f) * dim));
        g.strokePath (arc, juce::PathStrokeType (2.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // pointer
    const float r1 = radius - 6.0f;
    g.setColour (pointer.withAlpha (dim));
    g.drawLine (cx, cy, cx + r1 * std::sin (angle), cy - r1 * std::cos (angle), 1.6f);

    // label / value
    g.setFont (Aviation::label (8.0f, 0.18f));
    g.setColour (Aviation::textSecondary().withAlpha (dim));
    g.drawText (labelText, 0, (int) knob.getBottom() + 3, getWidth(), 11, juce::Justification::centred);
    g.setFont (Deck::mono (9.0f));
    g.setColour ((isGold ? Aviation::goldBright() : Aviation::cyan()).withAlpha (dim));
    g.drawText (valueText(), 0, (int) knob.getBottom() + 14, getWidth(), 12, juce::Justification::centred);

    // enable LED
    if (enableAttachment != nullptr)
    {
        auto led = ledArea().toFloat();
        g.setColour (enabledState ? Deck::green() : juce::Colour (0xff20323f));
        g.fillEllipse (led);
        if (enabledState)
        {
            g.setColour (Deck::green().withAlpha (0.35f));
            g.drawEllipse (led.expanded (2.0f), 1.0f);
        }
    }
}

// =============================================================================
//  DeckSegment
// =============================================================================
DeckSegment::DeckSegment (juce::AudioProcessorValueTreeState& apvts,
                          const juce::String& paramId,
                          std::vector<Option> opts,
                          const juce::String& cap)
    : apvtsRef (apvts), options (std::move (opts)), caption (cap)
{
    param = apvtsRef.getParameter (paramId);
    if (param != nullptr)
    {
        attachment = std::make_unique<juce::ParameterAttachment> (*param, [this] (float v)
        {
            currentValue = juce::roundToInt (v);
            repaint();
        });
        attachment->sendInitialUpdate();
    }
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

int DeckSegment::preferredWidth (int cellPadding) const
{
    int w = 2;
    const auto f = Aviation::label (8.0f, 0.18f);
    for (const auto& o : options)
        w += juce::roundToInt (juce::GlyphArrangement::getStringWidth (f, o.first)) + cellPadding * 2;
    return w;
}

juce::Rectangle<int> DeckSegment::segArea() const
{
    return getLocalBounds().withHeight (kSegH);
}

int DeckSegment::cellAt (juce::Point<int> p) const
{
    const auto area = segArea();
    if (! area.contains (p) || options.empty())
        return -1;
    const int n = (int) options.size();
    return juce::jlimit (0, n - 1, (p.x - area.getX()) * n / juce::jmax (1, area.getWidth()));
}

void DeckSegment::mouseDown (const juce::MouseEvent& e)
{
    const int cell = cellAt (e.getPosition());
    if (cell < 0 || attachment == nullptr)
        return;
    attachment->setValueAsCompleteGesture ((float) options[(size_t) cell].second);
}

void DeckSegment::mouseMove (const juce::MouseEvent& e)
{
    const int cell = cellAt (e.getPosition());
    if (cell != hovered)
    {
        hovered = cell;
        repaint();
    }
}

void DeckSegment::paint (juce::Graphics& g)
{
    const auto area = segArea().toFloat();
    g.setColour (Deck::segBorder());
    g.drawRoundedRectangle (area, 4.0f, 1.0f);

    const int n = (int) options.size();
    if (n == 0)
        return;
    const float cellW = area.getWidth() / (float) n;
    for (int i = 0; i < n; ++i)
    {
        juce::Rectangle<float> cell (area.getX() + cellW * (float) i, area.getY(), cellW, area.getHeight());
        const bool on = options[(size_t) i].second == currentValue;
        if (on)
        {
            g.setColour (Deck::segOnBg());
            g.fillRoundedRectangle (cell.reduced (1.0f), 3.0f);
        }
        if (i > 0)
        {
            g.setColour (Deck::segBorder());
            g.fillRect (juce::Rectangle<float> (cell.getX(), cell.getY() + 4.0f, 1.0f, cell.getHeight() - 8.0f));
        }
        g.setFont (Aviation::label (8.0f, 0.18f));
        g.setColour (on ? Aviation::cyan() : (hovered == i ? Aviation::textPrimary() : Aviation::textDim()));
        g.drawText (options[(size_t) i].first, cell.toNearestInt(), juce::Justification::centred);
    }

    if (caption.isNotEmpty())
    {
        g.setFont (Aviation::label (8.0f, 0.18f));
        g.setColour (Aviation::textSecondary());
        g.drawText (caption, 0, kSegH + 2, getWidth(), kCaptionH, juce::Justification::centred);
    }
}

// =============================================================================
//  DeckPad
// =============================================================================
DeckPad::DeckPad (juce::AudioProcessorValueTreeState& apvts,
                  const juce::String& paramId,
                  const juce::String& label,
                  const juce::String& sublabel,
                  juce::Colour litColour,
                  bool momentaryOnShift)
    : apvtsRef (apvts), labelText (label), subText (sublabel), accent (litColour), momentary (momentaryOnShift)
{
    param = apvtsRef.getParameter (paramId);
    if (param != nullptr)
    {
        attachment = std::make_unique<juce::ParameterAttachment> (*param, [this] (float v)
        {
            lit = v > 0.5f;
            repaint();
        });
        attachment->sendInitialUpdate();
    }
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

void DeckPad::mouseDown (const juce::MouseEvent& e)
{
    if (attachment == nullptr)
        return;
    if (momentary && e.mods.isShiftDown())
    {
        momentaryActive = true;
        attachment->setValueAsCompleteGesture (1.0f);
        return;
    }
    attachment->setValueAsCompleteGesture (lit ? 0.0f : 1.0f);
}

void DeckPad::mouseUp (const juce::MouseEvent&)
{
    if (momentaryActive && attachment != nullptr)
    {
        momentaryActive = false;
        attachment->setValueAsCompleteGesture (0.0f);
    }
}

void DeckPad::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    const bool greenTint = accent == Deck::green();
    Deck::paintPad (g, r, lit, accent, hover, greenTint);

    auto face = r.withTrimmedBottom (4.0f).translated (0.0f, lit ? 3.0f : 0.0f);

    // LED
    juce::Rectangle<float> led (face.getRight() - 14.0f, face.getY() + 7.0f, 6.0f, 6.0f);
    g.setColour (lit ? accent : juce::Colour (0xff20323f));
    g.fillEllipse (led);
    if (lit)
    {
        g.setColour (accent.withAlpha (0.4f));
        g.drawEllipse (led.expanded (2.5f), 1.0f);
    }

    g.setFont (Aviation::label (10.0f, 0.24f));
    g.setColour (lit ? accent : Aviation::textSecondary());
    g.drawText (labelText, face.toNearestInt().withTrimmedBottom (subText.isNotEmpty() ? 12 : 0),
                juce::Justification::centred);
    if (subText.isNotEmpty())
    {
        g.setFont (Deck::mono (7.5f));
        g.setColour (Aviation::textDim());
        g.drawText (subText, face.toNearestInt().removeFromBottom (18).withTrimmedBottom (4),
                    juce::Justification::centred);
    }
}

// =============================================================================
//  ModePads
// =============================================================================
ModePads::ModePads (juce::AudioProcessorValueTreeState& apvts, const juce::String& paramId)
    : apvtsRef (apvts)
{
    param = apvtsRef.getParameter (paramId);
    if (param != nullptr)
    {
        attachment = std::make_unique<juce::ParameterAttachment> (*param, [this] (float v)
        {
            current = juce::roundToInt (v);
            repaint();
        });
        attachment->sendInitialUpdate();
    }
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

juce::Rectangle<int> ModePads::padArea (int index) const
{
    const int gap = 8;
    const int w = (getWidth() - gap * 4) / 5;
    return { index * (w + gap), 0, w, getHeight() };
}

juce::Path ModePads::glyphFor (int mode)
{
    juce::Path p;
    switch (mode)
    {
        case 0: p.startNewSubPath (2, 12); p.lineTo (8, 8.5f); p.lineTo (14, 5); p.lineTo (22, 1.5f); break;
        case 1: p.startNewSubPath (2, 2);  p.lineTo (8, 5.5f); p.lineTo (14, 9); p.lineTo (22, 12.5f); break;
        case 2: p.startNewSubPath (2, 12); p.lineTo (7, 8); p.lineTo (12, 2); p.lineTo (17, 8); p.lineTo (22, 12); break;
        case 3: p.startNewSubPath (2, 9);  p.lineTo (7, 4); p.lineTo (11, 11); p.lineTo (16, 2); p.lineTo (22, 9); break;
        default:
            p.startNewSubPath (3, 10); p.lineTo (7, 10);
            p.startNewSubPath (10, 5); p.lineTo (14, 5);
            p.startNewSubPath (17, 8); p.lineTo (21, 8);
            break;
    }
    return p;
}

void ModePads::mouseDown (const juce::MouseEvent& e)
{
    for (int i = 0; i < 5; ++i)
        if (padArea (i).contains (e.getPosition()) && attachment != nullptr)
            attachment->setValueAsCompleteGesture ((float) i);
}

void ModePads::mouseMove (const juce::MouseEvent& e)
{
    int h = -1;
    for (int i = 0; i < 5; ++i)
        if (padArea (i).contains (e.getPosition()))
            h = i;
    if (h != hovered)
    {
        hovered = h;
        repaint();
    }
}

void ModePads::paint (juce::Graphics& g)
{
    static const char* labels[] = { "UP", "DOWN", "UP-DN", "RANDOM", "AS PLAYED" };
    for (int i = 0; i < 5; ++i)
    {
        auto r = padArea (i).toFloat();
        const bool on = current == i;
        Deck::paintPad (g, r, on, Aviation::cyan(), hovered == i);
        auto face = r.withTrimmedBottom (4.0f).translated (0.0f, on ? 3.0f : 0.0f);

        auto glyph = glyphFor (i);
        const auto gb = glyph.getBounds();
        const float scale = 1.0f;
        glyph.applyTransform (juce::AffineTransform::translation (-gb.getX(), -gb.getY())
                                  .scaled (scale)
                                  .translated (face.getCentreX() - gb.getWidth() * 0.5f, face.getY() + 12.0f));
        g.setColour (Aviation::cyan().withAlpha (on ? 1.0f : 0.8f));
        g.strokePath (glyph, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        g.setFont (Aviation::label (8.5f, 0.18f));
        g.setColour (on ? Aviation::cyan() : Aviation::textSecondary());
        g.drawText (labels[i], face.toNearestInt().removeFromBottom (20).withTrimmedBottom (5),
                    juce::Justification::centred);
    }
}

// =============================================================================
//  OctaveStepper
// =============================================================================
OctaveStepper::OctaveStepper (juce::AudioProcessorValueTreeState& apvts, const juce::String& paramId,
                              const juce::String& suffix)
    : apvtsRef (apvts), suffixText (suffix)
{
    param = apvtsRef.getParameter (paramId);
    if (param != nullptr)
    {
        attachment = std::make_unique<juce::ParameterAttachment> (*param, [this] (float v)
        {
            current = juce::roundToInt (v);
            repaint();
        });
        attachment->sendInitialUpdate();
    }
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

void OctaveStepper::mouseDown (const juce::MouseEvent& e)
{
    if (attachment == nullptr || param == nullptr)
        return;
    const auto& range = param->getNormalisableRange();
    const int lo = (int) range.start, hi = (int) range.end;
    if (e.x < 24)
        attachment->setValueAsCompleteGesture ((float) juce::jlimit (lo, hi, current - 1));
    else if (e.x > getWidth() - 24)
        attachment->setValueAsCompleteGesture ((float) juce::jlimit (lo, hi, current + 1));
}

void OctaveStepper::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (juce::Colour (0xff081420));
    g.fillRoundedRectangle (r, 4.0f);
    g.setColour (Deck::segBorder());
    g.drawRoundedRectangle (r, 4.0f, 1.0f);

    g.setFont (Deck::mono (13.0f));
    g.setColour (Aviation::cyan());
    g.drawText (juce::String::fromUTF8 ("\xe2\x88\x92"), 0, 0, 24, getHeight(), juce::Justification::centred);
    g.drawText ("+", getWidth() - 24, 0, 24, getHeight(), juce::Justification::centred);

    g.setFont (Deck::mono (10.5f));
    g.setColour (Aviation::goldBright());
    g.drawText (juce::String (current) + suffixText, 24, 0, getWidth() - 48, getHeight(), juce::Justification::centred);
}

// =============================================================================
//  DeckChip
// =============================================================================
DeckChip::DeckChip (const juce::String& label, const juce::String& value, juce::Colour valueColour)
    : labelText (label), valueText (value), valueCol (valueColour)
{
}

void DeckChip::setValue (const juce::String& v, juce::Colour colour)
{
    if (v != valueText || colour != valueCol)
    {
        valueText = v;
        valueCol = colour;
        repaint();
    }
}

void DeckChip::setLabel (const juce::String& l)
{
    if (l != labelText)
    {
        labelText = l;
        repaint();
    }
}

int DeckChip::preferredWidth() const
{
    const auto f = Deck::mono (8.0f);
    const float w = juce::GlyphArrangement::getStringWidth (f, labelText + " " + valueText);
    return juce::roundToInt (w) + 20;
}

void DeckChip::mouseDown (const juce::MouseEvent&)
{
    dragAccum = 0;
    dragged = false;
}

void DeckChip::mouseDrag (const juce::MouseEvent& e)
{
    if (! onDragTicks)
        return;
    const int ticks = -e.getDistanceFromDragStartY() / 6;
    if (ticks != dragAccum)
    {
        onDragTicks (ticks - dragAccum);
        dragAccum = ticks;
        dragged = true;
    }
}

void DeckChip::mouseUp (const juce::MouseEvent&)
{
    if (! dragged && onClick)
        onClick();
}

void DeckChip::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    const bool interactive = onClick || onDragTicks;
    g.setColour (juce::Colour (0xff17293a).withAlpha (interactive && hover ? 1.0f : 0.8f));
    g.drawRoundedRectangle (r, 3.0f, 1.0f);
    if (interactive && hover)
    {
        g.setColour (Aviation::cyan().withAlpha (0.06f));
        g.fillRoundedRectangle (r, 3.0f);
    }

    g.setFont (Deck::mono (8.0f));
    auto area = getLocalBounds().reduced (9, 0);
    const int labelW = juce::roundToInt (juce::GlyphArrangement::getStringWidth (Deck::mono (8.0f), labelText));
    g.setColour (Aviation::textDim());
    g.drawText (labelText, area.removeFromLeft (labelW + 4), juce::Justification::centredLeft);
    g.setColour (valueCol);
    g.drawText (valueText, area, juce::Justification::centredLeft);
}
