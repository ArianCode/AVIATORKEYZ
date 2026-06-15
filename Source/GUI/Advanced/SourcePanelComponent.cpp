#include "SourcePanelComponent.h"

namespace
{
void styleTitle (juce::Label& l, const juce::String& t)
{
    l.setText (t, juce::dontSendNotification);
    l.setFont (AviatorTokens::hudBold (9.f));
    l.setColour (juce::Label::textColourId, AviatorTokens::champagneGold());
}
} // namespace

SourcePanelComponent::SourcePanelComponent (juce::AudioProcessorValueTreeState& apvts)
{
    using PF = PrecisionKnob::ValueFormat;
    namespace P = AviatorKeyz::ParamID;

    for (auto* l : { &osc1Title, &osc2Title, &sampleTitle })
        addAndMakeVisible (l);
    styleTitle (osc1Title, "OSCILLATOR 1");
    styleTitle (osc2Title, "OSCILLATOR 2");
    styleTitle (sampleTitle, "SAMPLE / SOURCE BLEND");

    knobs.push_back (AdvancedKnobHelpers::makeKnob (apvts, P::OSC1_TUNE,  "TUNE",  "Semi", PF::percent, *this));
    knobs.push_back (AdvancedKnobHelpers::makeKnob (apvts, P::OSC1_SHAPE, "SHAPE", {}, PF::percent, *this));
    knobs.push_back (AdvancedKnobHelpers::makeKnob (apvts, P::OSC1_LEVEL,  "LEVEL", {}, PF::percent, *this));
    knobs.push_back (AdvancedKnobHelpers::makeKnob (apvts, P::OSC2_TUNE,  "TUNE",  "Semi", PF::percent, *this));
    knobs.push_back (AdvancedKnobHelpers::makeKnob (apvts, P::OSC2_SHAPE, "SHAPE", {}, PF::percent, *this));
    knobs.push_back (AdvancedKnobHelpers::makeKnob (apvts, P::OSC2_LEVEL,  "LEVEL", {}, PF::percent, *this));
    knobs.push_back (AdvancedKnobHelpers::makeKnob (apvts, P::SOURCE_BLEND, "BLEND", "Sample/Synth", PF::percent, *this));
    knobs.push_back (AdvancedKnobHelpers::makeKnob (apvts, P::INPUT_GAIN, "GAIN", "Sample", PF::decibels, *this));
    knobs.push_back (AdvancedKnobHelpers::makeKnob (apvts, P::VELOCITY_SENSITIVITY, "VEL", "0=Full", PF::percent, *this));
}

void SourcePanelComponent::paint (juce::Graphics& g)
{
    AdvancedKnobHelpers::paintPageBackground (g, *this);
}

void SourcePanelComponent::resized()
{
    const int pad = AviatorTokens::scaledFor (*this, 8);
    auto area = getLocalBounds().reduced (pad);
    osc1Title.setBounds (area.removeFromTop (14));
    area.removeFromTop (2);
    auto row1 = area.removeFromTop (AviatorTokens::scaledFor (*this, AviatorTokens::kKnobCellH));
    for (int i = 0; i < 3 && i < (int) knobs.size(); ++i)
        knobs[(size_t) i]->setBounds (row1.removeFromLeft (AviatorTokens::scaledFor (*this, AviatorTokens::kKnobCellW)));
    area.removeFromTop (6);
    osc2Title.setBounds (area.removeFromTop (14));
    area.removeFromTop (2);
    auto row2 = area.removeFromTop (AviatorTokens::scaledFor (*this, AviatorTokens::kKnobCellH));
    for (int i = 3; i < 6 && i < (int) knobs.size(); ++i)
        knobs[(size_t) i]->setBounds (row2.removeFromLeft (AviatorTokens::scaledFor (*this, AviatorTokens::kKnobCellW)));
    area.removeFromTop (6);
    sampleTitle.setBounds (area.removeFromTop (14));
    area.removeFromTop (2);
    auto row3 = area.removeFromTop (AviatorTokens::scaledFor (*this, AviatorTokens::kKnobCellH));
    for (int i = 6; i < (int) knobs.size(); ++i)
        knobs[(size_t) i]->setBounds (row3.removeFromLeft (AviatorTokens::scaledFor (*this, AviatorTokens::kKnobCellW)));
}
