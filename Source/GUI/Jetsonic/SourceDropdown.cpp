#include "SourceDropdown.h"
#include "JetsonicIcons.h"
#include "JetsonicTheme.h"

SourceDropdown::SourceDropdown()
{
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

void SourceDropdown::setSourceText (const juce::String& text)
{
    if (sourceText != text)
    {
        sourceText = text;
        repaint();
    }
}

void SourceDropdown::mouseDown (const juce::MouseEvent&)
{
    if (onClicked)
        onClicked();
}

void SourceDropdown::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (1.0f);

    juce::ColourGradient grad (juce::Colour (0xf20b1620), r.getX(), r.getY(),
                               juce::Colour (0xf2050c13), r.getX(), r.getBottom(), false);
    g.setGradientFill (grad);
    g.fillRoundedRectangle (r, 9.0f);

    g.setColour (Jetsonic::gold().withAlpha (hovered ? 0.95f : 0.65f));
    g.drawRoundedRectangle (r, 9.0f, 1.1f);
    Jetsonic::topSpecular (g, r, 0.05f);

    // waveform icon
    JetsonicIcons::fill (g, JetsonicIcons::waveform(),
                         { r.getX() + 12.0f, r.getCentreY() - 8.0f, 26.0f, 16.0f },
                         Jetsonic::textPrimary().withAlpha (0.9f));

    // chevron
    JetsonicIcons::stroke (g, JetsonicIcons::chevronDown(),
                           { r.getRight() - 28.0f, r.getCentreY() - 6.0f, 13.0f, 12.0f },
                           Jetsonic::textPrimary().withAlpha (hovered ? 1.0f : 0.8f), 1.6f);

    // source text
    const juce::Rectangle<int> textArea ((int) r.getX() + 46, (int) r.getY(),
                                         (int) r.getWidth() - 82, (int) r.getHeight());
    g.setFont (Jetsonic::body (13.0f));
    g.setColour (Jetsonic::textPrimary());
    const auto width_f = [&g] (const juce::String& s)
    { return juce::GlyphArrangement::getStringWidth (g.getCurrentFont(), s); };
    auto fitted = sourceText;
    if (width_f (fitted) > (float) textArea.getWidth())
    {
        while (fitted.isNotEmpty() && width_f (fitted + "...") > (float) textArea.getWidth())
            fitted = fitted.dropLastCharacters (1);
        fitted += "...";
    }
    g.drawText (fitted, textArea, juce::Justification::centredLeft);
}
