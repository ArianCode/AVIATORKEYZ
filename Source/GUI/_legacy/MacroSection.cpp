#include "MacroSection.h"
#include "../PluginProcessor.h"
#include "../State/StateSchema.h"
#include "DesignTokens.h"

MacroSection::MacroSection (AviatorKeyzProcessor& processor)
    : reverseToggle (processor.getAPVTS(), AviatorKeyz::ParamID::REVERSE)
    , glideKnob (processor.getAPVTS(),
                 AviatorKeyz::ParamID::GLIDE_TIME,
                 "Glide",
                 "Pitch Continuity",
                 PrecisionKnob::ValueFormat::glideSeconds)
    , smearKnob (processor.getAPVTS(),
                 AviatorKeyz::ParamID::SMEAR,
                 "Smear",
                 "Envelope Blur",
                 PrecisionKnob::ValueFormat::percent)
    , toneKnob (processor.getAPVTS(),
                AviatorKeyz::ParamID::TONE,
                "Tone",
                "Spectral Tilt",
                PrecisionKnob::ValueFormat::toneDb)
{
    setOpaque (true);
    addAndMakeVisible (reverseToggle);
    addAndMakeVisible (glideKnob);
    addAndMakeVisible (smearKnob);
    addAndMakeVisible (toneKnob);
}

void MacroSection::paint (juce::Graphics& g)
{
    const float s = DesignTokens::scaleFactorFor (*this);

    juce::ColourGradient bg (juce::Colour (0xff0e0d15), 0.f, 0.f,
                             juce::Colour (0xff0c0b12), 0.f, (float) getHeight(), false);
    g.setGradientFill (bg);
    g.fillAll();

    g.setColour (DesignTokens::border1());
    g.drawHorizontalLine (getHeight() - 1, 0.f, (float) getWidth());

    g.setFont (DesignTokens::labelFont (7.f * s, juce::Font::bold));
    g.setColour (DesignTokens::textFaint());
    g.drawText ("CHARACTER", DesignTokens::scaled (28, s), DesignTokens::scaled (20, s),
                DesignTokens::scaled (120, s), DesignTokens::scaled (10, s),
                juce::Justification::centredLeft);

    g.setColour (DesignTokens::champagneMid().withAlpha (0.45f));
    g.drawText ("MACRO CONTROLS", getWidth() - DesignTokens::scaled (148, s),
                DesignTokens::scaled (20, s), DesignTokens::scaled (120, s),
                DesignTokens::scaled (10, s), juce::Justification::centredRight);
}

void MacroSection::resized()
{
    const float s = DesignTokens::scaleFactorFor (*this);

    auto grid = getLocalBounds().reduced (DesignTokens::scaled (28, s), DesignTokens::scaled (20, s));
    grid.removeFromTop (DesignTokens::scaled (28, s));

    const int colW = grid.getWidth() / 4;
    reverseToggle.setBounds (grid.removeFromLeft (DesignTokens::scaled (88, s)));
    grid.removeFromLeft (colW - DesignTokens::scaled (88, s));

    glideKnob.setBounds (grid.removeFromLeft (colW));
    smearKnob.setBounds (grid.removeFromLeft (colW));
    toneKnob.setBounds (grid);
}
