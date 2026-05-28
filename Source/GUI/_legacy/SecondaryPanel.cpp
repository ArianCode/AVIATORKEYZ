#include "SecondaryPanel.h"
#include "../PluginProcessor.h"
#include "../State/StateSchema.h"
#include "DesignTokens.h"

namespace
{
    juce::String formatMs (float v)
    {
        if (v >= 1000.f)
            return juce::String (v / 1000.f, 1) + " s";
        return juce::String (juce::roundToInt (v)) + " ms";
    }

    juce::String formatPercent (float v)
    {
        return juce::String (juce::roundToInt (v * 100.f)) + "%";
    }

    juce::String formatDb (float v)
    {
        if (std::abs (v) < 0.05f)
            return "0.0";
        return juce::String (v, 1);
    }

    juce::String formatPan (float v)
    {
        if (std::abs (v) < 0.01f)
            return "C";
        return juce::String (v, 2);
    }
}

SecondaryPanel::SecondaryPanel (AviatorKeyzProcessor& processor)
    : attackFader (processor.getAPVTS(), AviatorKeyz::ParamID::ENV_ATTACK, "Atk", formatMs)
    , releaseFader (processor.getAPVTS(), AviatorKeyz::ParamID::ENV_RELEASE, "Rel", formatMs)
    , roomFader (processor.getAPVTS(), AviatorKeyz::ParamID::REVERB_AMOUNT, "Room", formatPercent)
    , widthFader (processor.getAPVTS(), AviatorKeyz::ParamID::STEREO_WIDTH, "Width",
                  [] (float v) { return juce::String (juce::roundToInt (v * 50.f)) + "%"; })
    , levelFader (processor.getAPVTS(), AviatorKeyz::ParamID::OUTPUT_GAIN, "Level", formatDb)
    , panFader (processor.getAPVTS(), AviatorKeyz::ParamID::PAN, "Pan", formatPan)
{
    setOpaque (true);
    for (auto* l : { &sectionAmp, &sectionSpace, &sectionOutput })
    {
        l->setFont (DesignTokens::labelFont (7.f, juce::Font::bold));
        l->setColour (juce::Label::textColourId, DesignTokens::textFaint());
        addAndMakeVisible (*l);
    }

    sectionAmp.setText ("AMP", juce::dontSendNotification);
    sectionSpace.setText ("SPACE", juce::dontSendNotification);
    sectionOutput.setText ("OUTPUT", juce::dontSendNotification);

    for (auto* f : { &attackFader, &releaseFader, &roomFader, &widthFader, &levelFader, &panFader })
        addAndMakeVisible (*f);
}

void SecondaryPanel::paint (juce::Graphics& g)
{
    juce::ColourGradient bg (juce::Colour (0xff0a0910), 0.f, 0.f,
                             juce::Colour (0xff08080d), 0.f, (float) getHeight(), false);
    g.setGradientFill (bg);
    g.fillAll();

    const int colW = getWidth() / 3;
    g.setColour (DesignTokens::border0());
    g.drawVerticalLine (colW, 0.f, (float) getHeight());
    g.drawVerticalLine (colW * 2, 0.f, (float) getHeight());
}

void SecondaryPanel::resized()
{
    const float s = DesignTokens::scaleFactorFor (*this);
    const int padH = DesignTokens::scaled (22, s);
    const int padV = DesignTokens::scaled (13, s);
    const int labelH = DesignTokens::scaled (12, s);
    const int faderH = DesignTokens::scaled (22, s);
    const int gap = DesignTokens::scaled (8, s);

    sectionAmp.setFont (DesignTokens::labelFont (7.f * s, juce::Font::bold));
    sectionSpace.setFont (DesignTokens::labelFont (7.f * s, juce::Font::bold));
    sectionOutput.setFont (DesignTokens::labelFont (7.f * s, juce::Font::bold));

    const int colW = getWidth() / 3;
    auto col1 = getLocalBounds().removeFromLeft (colW).reduced (padH, padV);
    auto col2 = getLocalBounds().removeFromLeft (colW).reduced (padH, padV);
    auto col3 = getLocalBounds().reduced (padH, padV);

    sectionAmp.setBounds (col1.removeFromTop (labelH));
    attackFader.setBounds (col1.removeFromTop (faderH));
    col1.removeFromTop (gap);
    releaseFader.setBounds (col1.removeFromTop (faderH));

    sectionSpace.setBounds (col2.removeFromTop (labelH));
    roomFader.setBounds (col2.removeFromTop (faderH));
    col2.removeFromTop (gap);
    widthFader.setBounds (col2.removeFromTop (faderH));

    sectionOutput.setBounds (col3.removeFromTop (labelH));
    levelFader.setBounds (col3.removeFromTop (faderH));
    col3.removeFromTop (gap);
    panFader.setBounds (col3.removeFromTop (faderH));
}
