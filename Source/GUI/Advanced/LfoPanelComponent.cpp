#include "LfoPanelComponent.h"

namespace
{
juce::StringArray lfoShapeNames()
{
    return { "Sine", "Square", "Triangle", "Ramp Up", "Ramp Down", "Random" };
}
} // namespace

LfoPanelComponent::LfoPanelComponent (juce::AudioProcessorValueTreeState& apvts)
{
    using PF = PrecisionKnob::ValueFormat;

    for (int i = 0; i < 3; ++i)
    {
        auto& strip = strips[(size_t) i];
        const auto& ids = kLfos[i];

        strip.title.setText (ids.label, juce::dontSendNotification);
        strip.title.setFont (AviatorTokens::hudBold (10.f));
        strip.title.setColour (juce::Label::textColourId, AviatorTokens::instrumentCyan());
        addAndMakeVisible (strip.title);

        strip.waveform = std::make_unique<LfoWaveformDisplay> (apvts, ids.shape, ids.phase);
        addAndMakeVisible (*strip.waveform);

        strip.rateKnob = std::make_unique<PrecisionKnob> (apvts, ids.rate, "RATE", "Hz", PF::percent, true, true);
        strip.depthKnob = std::make_unique<PrecisionKnob> (apvts, ids.depth, "DEPTH", juce::String(), PF::percent, true, false);
        strip.phaseKnob = std::make_unique<PrecisionKnob> (apvts, ids.phase, "PHASE", juce::String(), PF::percent, true, false);
        addAndMakeVisible (*strip.rateKnob);
        addAndMakeVisible (*strip.depthKnob);
        addAndMakeVisible (*strip.phaseKnob);

        strip.shapeBox = std::make_unique<juce::ComboBox> ("shape");
        strip.shapeBox->addItemList (lfoShapeNames(), 1);
        strip.shapeBox->setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff0d1f33));
        strip.shapeBox->setColour (juce::ComboBox::textColourId, AviatorTokens::textPrimary());
        strip.shapeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (apvts, ids.shape, *strip.shapeBox);
        addAndMakeVisible (*strip.shapeBox);

        strip.syncButton.setButtonText ("SYNC");
        strip.syncButton.setColour (juce::ToggleButton::textColourId, AviatorTokens::textMuted());
        strip.syncButton.setColour (juce::ToggleButton::tickColourId, AviatorTokens::instrumentCyan());
        strip.syncAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (apvts, ids.sync, strip.syncButton);
        addAndMakeVisible (strip.syncButton);
    }
}

void LfoPanelComponent::paint (juce::Graphics& g)
{
    AdvancedKnobHelpers::paintPageBackground (g, *this);
}

void LfoPanelComponent::resized()
{
    const int pad = AviatorTokens::scaledFor (*this, AviatorTokens::kPanelPad);
    const int stripH = (getHeight() - pad * 2) / 3;
    const int cellW = AviatorTokens::scaledFor (*this, AviatorTokens::kKnobCellW);
    const int cellH = AviatorTokens::scaledFor (*this, AviatorTokens::kKnobCellH);
    const int gap = AviatorTokens::scaledFor (*this, AviatorTokens::kGridGapX);

    for (int i = 0; i < 3; ++i)
    {
        auto area = getLocalBounds().reduced (pad);
        area = area.withY (pad + i * stripH).withHeight (stripH - 4);

        auto titleRow = area.removeFromTop (16);
        strips[(size_t) i].title.setBounds (titleRow);

        auto waveArea = area.removeFromLeft (juce::jmax (100, area.getWidth() / 4));
        strips[(size_t) i].waveform->setBounds (waveArea.reduced (0, 4));
        area.removeFromLeft (gap);

        strips[(size_t) i].rateKnob->setBounds (area.removeFromLeft (cellW).withHeight (cellH));
        area.removeFromLeft (gap);
        strips[(size_t) i].depthKnob->setBounds (area.removeFromLeft (cellW).withHeight (cellH));
        area.removeFromLeft (gap);
        strips[(size_t) i].phaseKnob->setBounds (area.removeFromLeft (cellW).withHeight (cellH));
        area.removeFromLeft (gap);

        auto ctrlCol = area.removeFromLeft (juce::jmax (80, cellW));
        strips[(size_t) i].shapeBox->setBounds (ctrlCol.removeFromTop (22));
        ctrlCol.removeFromTop (4);
        strips[(size_t) i].syncButton.setBounds (ctrlCol.removeFromTop (22));
    }
}
