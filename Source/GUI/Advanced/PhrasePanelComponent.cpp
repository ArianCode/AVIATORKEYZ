#include "PhrasePanelComponent.h"
#include "../../State/StateSchema.h"

PhrasePanelComponent::PhrasePanelComponent (juce::AudioProcessorValueTreeState& apvts)
{
    using PF = PrecisionKnob::ValueFormat;
    namespace P = AviatorKeyz::ParamID;

    browserLabel.setFont (AviatorTokens::hud (10.f));
    browserLabel.setColour (juce::Label::textColourId, AviatorTokens::textMuted());
    addAndMakeVisible (browserLabel);

    phraseOn   = std::make_unique<ReverseToggle> (apvts, P::PHRASE_ENABLED,     "PHRASE", "On/Off");
    tempoSync  = std::make_unique<ReverseToggle> (apvts, P::PHRASE_TEMPO_SYNC,  "TEMPO",  "Sync");
    keySync    = std::make_unique<ReverseToggle> (apvts, P::PHRASE_KEY_SYNC,    "KEY",    "Sync");
    loopToggle = std::make_unique<ReverseToggle> (apvts, P::PHRASE_LOOP,        "LOOP",   "Mode");

    for (auto* t : { phraseOn.get(), tempoSync.get(), keySync.get(), loopToggle.get() })
        addAndMakeVisible (*t);

    knobs.push_back (AdvancedKnobHelpers::makeKnob (apvts, P::PHRASE_START,  "START",  {}, PF::percent, *this));
    knobs.push_back (AdvancedKnobHelpers::makeKnob (apvts, P::PHRASE_LENGTH, "LENGTH", {}, PF::percent, *this));
    knobs.push_back (AdvancedKnobHelpers::makeKnob (apvts, P::PHRASE_PITCH,  "PITCH",  "Semi", PF::percent, *this));
}

void PhrasePanelComponent::paint (juce::Graphics& g)
{
    AdvancedKnobHelpers::paintPageBackground (g, *this);
    g.setColour (AviatorTokens::instrumentCyan().withAlpha (0.12f));
    g.drawRect (getLocalBounds().reduced (AviatorTokens::scaledFor (*this, 8)).removeFromTop (60), 1.f);
}

void PhrasePanelComponent::resized()
{
    const int pad = AviatorTokens::scaledFor (*this, 8);
    auto area = getLocalBounds().reduced (pad);
    browserLabel.setBounds (area.removeFromTop (56));
    area.removeFromTop (8);

    const int cellW = AviatorTokens::scaledFor (*this, AviatorTokens::kKnobCellW);
    const int cellH = AviatorTokens::scaledFor (*this, AviatorTokens::kKnobCellH);
    const int gap = AviatorTokens::scaledFor (*this, AviatorTokens::kGridGapX);

    if (phraseOn)   phraseOn->setBounds (area.removeFromLeft (cellW).withHeight (cellH));
    area.removeFromLeft (gap);
    if (tempoSync)  tempoSync->setBounds (area.removeFromLeft (cellW).withHeight (cellH));
    area.removeFromLeft (gap);
    if (keySync)    keySync->setBounds (area.removeFromLeft (cellW).withHeight (cellH));
    area.removeFromLeft (gap);
    if (loopToggle) loopToggle->setBounds (area.removeFromLeft (cellW).withHeight (cellH));

    area.removeFromTop (8);
    AdvancedKnobHelpers::layoutKnobs (*this, knobs, 3);
}
