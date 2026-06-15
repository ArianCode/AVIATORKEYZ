#include "SynthPanelComponent.h"
#include "../../State/StateSchema.h"

namespace
{
void styleTitle (juce::Label& l, const juce::String& t)
{
    l.setText (t, juce::dontSendNotification);
    l.setFont (AviatorTokens::hudBold (9.f));
    l.setColour (juce::Label::textColourId, AviatorTokens::champagneGold());
}
} // namespace

SynthPanelComponent::SynthPanelComponent (juce::AudioProcessorValueTreeState& apvts)
{
    using PF = PrecisionKnob::ValueFormat;
    namespace P = AviatorKeyz::ParamID;

    for (auto* l : { &filterTitle, &ampTitle, &voiceTitle })
        addAndMakeVisible (l);
    styleTitle (filterTitle, "FILTER");
    styleTitle (ampTitle, "AMP ENVELOPE");
    styleTitle (voiceTitle, "VOICE");

    knobs.push_back (AdvancedKnobHelpers::makeKnob (apvts, P::FILTER_CUTOFF,    "CUTOFF", {}, PF::percent, *this));
    knobs.push_back (AdvancedKnobHelpers::makeKnob (apvts, P::FILTER_RESONANCE, "RESO",   {}, PF::percent, *this));
    knobs.push_back (AdvancedKnobHelpers::makeKnob (apvts, P::FILTER_DRIVE,     "DRIVE",  {}, PF::percent, *this));
    knobs.push_back (AdvancedKnobHelpers::makeKnob (apvts, P::VELOCITY_SENSITIVITY, "VEL", "0=Full", PF::percent, *this));
    knobs.push_back (AdvancedKnobHelpers::makeKnob (apvts, P::ENV_ATTACK,       "ATTACK", {}, PF::envelopeMs, *this));
    knobs.push_back (AdvancedKnobHelpers::makeKnob (apvts, P::ENV_AMP_DECAY,    "DECAY",  {}, PF::envelopeMs, *this));
    knobs.push_back (AdvancedKnobHelpers::makeKnob (apvts, P::ENV_AMP_SUSTAIN,  "SUSTAIN",{}, PF::percent, *this));
    knobs.push_back (AdvancedKnobHelpers::makeKnob (apvts, P::ENV_RELEASE,      "RELEASE",{}, PF::envelopeMs, *this));
    knobs.push_back (AdvancedKnobHelpers::makeKnob (apvts, P::GLIDE_TIME,       "GLIDE",  "ms", PF::glideSeconds, *this));
    knobs.push_back (AdvancedKnobHelpers::makeKnob (apvts, P::ENV_FLT_AMOUNT,   "FLT AMT",{}, PF::percent, *this));
}

void SynthPanelComponent::paint (juce::Graphics& g)
{
    AdvancedKnobHelpers::paintPageBackground (g, *this);
}

void SynthPanelComponent::resized()
{
    AdvancedKnobHelpers::layoutKnobs (*this, knobs, 4);
    const int pad = AviatorTokens::scaledFor (*this, 8);
    filterTitle.setBounds (pad, pad, 120, 12);
    ampTitle.setBounds (pad, pad + AviatorTokens::scaledFor (*this, 100), 120, 12);
    voiceTitle.setBounds (pad, pad + AviatorTokens::scaledFor (*this, 200), 120, 12);
}
