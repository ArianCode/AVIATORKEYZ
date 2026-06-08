#include "FxAdvancedPanel.h"
#include "../../State/StateSchema.h"

namespace
{
void styleSectionTitle (juce::Label& lbl, const juce::String& text)
{
    lbl.setText (text, juce::dontSendNotification);
    lbl.setFont (AviatorTokens::hudBold (9.f));
    lbl.setColour (juce::Label::textColourId, AviatorTokens::champagneGold());
    lbl.setJustificationType (juce::Justification::centredLeft);
}
} // namespace

FxAdvancedPanel::FxAdvancedPanel (juce::AudioProcessorValueTreeState& apvts)
{
    namespace P = AviatorKeyz::ParamID;

    for (auto* lbl : { &routingTitle, &reverbTitle, &delayTitle, &chorusTitle, &lofiTitle, &distTitle })
        addAndMakeVisible (lbl);

    styleSectionTitle (routingTitle, "FX ROUTING");
    styleSectionTitle (reverbTitle, "REVERB");
    styleSectionTitle (delayTitle, "DELAY");
    styleSectionTitle (chorusTitle, "CHORUS");
    styleSectionTitle (lofiTitle, "LO-FI");
    styleSectionTitle (distTitle, "DISTORT");

    routingBox.addItemList ({ "Dry Only", "Texture Only", "Both" }, 1);
    routingBox.setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff0d1f33));
    routingBox.setColour (juce::ComboBox::textColourId, AviatorTokens::textPrimary());
    routingAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (apvts, P::FX_ROUTING, routingBox);
    addAndMakeVisible (routingBox);

    reverbOnToggle = std::make_unique<ReverseToggle> (apvts, P::FX_REVERB_ON,  "REVERB", "On/Off");
    delayOnToggle  = std::make_unique<ReverseToggle> (apvts, P::FX_DELAY_ON,   "DELAY",  "On/Off");
    chorusOnToggle = std::make_unique<ReverseToggle> (apvts, P::FX_CHORUS_ON,  "CHORUS", "On/Off");
    lofiOnToggle   = std::make_unique<ReverseToggle> (apvts, P::FX_LOFI_ON,    "LO-FI",  "On/Off");
    distOnToggle   = std::make_unique<ReverseToggle> (apvts, P::FX_DIST_ON,    "DIST",   "On/Off");

    for (auto* t : { reverbOnToggle.get(), delayOnToggle.get(), chorusOnToggle.get(), lofiOnToggle.get(), distOnToggle.get() })
        addAndMakeVisible (*t);

    addKnob (apvts, P::REVERB_AMOUNT, "MIX",  juce::String());
    addKnob (apvts, P::REVERB_SIZE,   "SIZE", juce::String());
    addKnob (apvts, P::FX_REVERB_DAMP, "DAMP", juce::String());
    addKnob (apvts, P::FX_DELAY_TIME,     "TIME",     "Sec");
    addKnob (apvts, P::FX_DELAY_FEEDBACK, "FEEDBACK", juce::String());
    addKnob (apvts, P::FX_DELAY_MIX,      "MIX",      juce::String());
    addKnob (apvts, P::FX_CHORUS_RATE,    "RATE",     juce::String());
    addKnob (apvts, P::FX_CHORUS_DEPTH,   "DEPTH",    juce::String());
    addKnob (apvts, P::FX_CHORUS_MIX,      "MIX",      juce::String());
    addKnob (apvts, P::FX_LOFI_AMOUNT,     "AMOUNT",   juce::String());
    addKnob (apvts, P::FX_DIST_DRIVE,      "DRIVE",    juce::String());
}

void FxAdvancedPanel::addKnob (juce::AudioProcessorValueTreeState& apvts,
                               const char* id,
                               const juce::String& name,
                               const juce::String& sub,
                               PrecisionKnob::ValueFormat fmt)
{
    auto knob = std::make_unique<PrecisionKnob> (apvts, id, name, sub, fmt, true, true);
    addAndMakeVisible (*knob);
    knobs.push_back (std::move (knob));
}

void FxAdvancedPanel::paint (juce::Graphics& g)
{
    AdvancedKnobHelpers::paintPageBackground (g, *this);
}

void FxAdvancedPanel::resized()
{
    const int pad = AviatorTokens::scaledFor (*this, AviatorTokens::kPanelPad);
    const int cellW = AviatorTokens::scaledFor (*this, AviatorTokens::kKnobCellW);
    const int cellH = AviatorTokens::scaledFor (*this, AviatorTokens::kKnobCellH);
    const int gap = AviatorTokens::scaledFor (*this, AviatorTokens::kGridGapX);

    auto bounds = getLocalBounds().reduced (pad);
    int knobIdx = 0;

    auto layoutSection = [&] (juce::Label& title, juce::Component* extra, ReverseToggle* toggle, int numKnobs)
    {
        auto section = bounds.removeFromTop (AviatorTokens::scaledFor (*this, 100));
        title.setBounds (section.removeFromTop (14));
        section.removeFromTop (2);

        if (extra != nullptr)
        {
            extra->setBounds (section.removeFromTop (22).removeFromLeft (juce::jmax (140, cellW * 2)));
            section.removeFromTop (4);
        }

        if (toggle != nullptr)
        {
            toggle->setBounds (section.removeFromLeft (cellW).withHeight (cellH));
            section.removeFromLeft (gap);
        }

        for (int k = 0; k < numKnobs && knobIdx < (int) knobs.size(); ++k, ++knobIdx)
        {
            knobs[(size_t) knobIdx]->setBounds (section.removeFromLeft (cellW).withHeight (cellH));
            section.removeFromLeft (gap);
        }
        bounds.removeFromTop (4);
    };

    layoutSection (routingTitle, &routingBox, nullptr, 0);
    layoutSection (reverbTitle, nullptr, reverbOnToggle.get(), 3);
    layoutSection (delayTitle, nullptr, delayOnToggle.get(), 3);
    layoutSection (chorusTitle, nullptr, chorusOnToggle.get(), 3);
    layoutSection (lofiTitle, nullptr, lofiOnToggle.get(), 1);
    layoutSection (distTitle, nullptr, distOnToggle.get(), 1);
}
