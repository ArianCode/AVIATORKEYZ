#include "TexturePanelComponent.h"
#include "../../State/StateSchema.h"
#include "../Cockpit/CockpitLayout.h"

TexturePanelComponent::TexturePanelComponent (juce::AudioProcessorValueTreeState& apvts)
    : apvtsRef (apvts)
{
    using PF = PrecisionKnob::ValueFormat;
    namespace P = AviatorKeyz::ParamID;

    visualizer = std::make_unique<TextureVisualizerComponent>();
    addAndMakeVisible (*visualizer);

    knobs.push_back (AdvancedKnobHelpers::makeKnob (apvts, P::TEX_AMOUNT,        "AMOUNT", {}, PF::percent, *this));
    knobs.push_back (AdvancedKnobHelpers::makeKnob (apvts, P::TEX_WIDTH,         "WIDTH",  {}, PF::percent, *this));
    knobs.push_back (AdvancedKnobHelpers::makeKnob (apvts, P::TEX_GRAIN_SCAN,    "SCAN",   {}, PF::percent, *this));
    knobs.push_back (AdvancedKnobHelpers::makeKnob (apvts, P::TEX_GRAIN_RATE,    "RATE",   {}, PF::percent, *this));
    knobs.push_back (AdvancedKnobHelpers::makeKnob (apvts, P::TEX_GRAIN_SIZE,    "SIZE",   {}, PF::percent, *this));
    knobs.push_back (AdvancedKnobHelpers::makeKnob (apvts, P::TEX_GRAIN_DENSITY, "DENSITY",{}, PF::percent, *this));
    knobs.push_back (AdvancedKnobHelpers::makeKnob (apvts, P::TEX_GRAIN_SPREAD,  "SPREAD", {}, PF::percent, *this));
    knobs.push_back (AdvancedKnobHelpers::makeKnob (apvts, P::TEX_MOTION,        "MOTION", {}, PF::percent, *this));
    knobs.push_back (AdvancedKnobHelpers::makeKnob (apvts, P::TEX_DRIFT,         "DRIFT",  {}, PF::percent, *this));
    knobs.push_back (AdvancedKnobHelpers::makeKnob (apvts, P::TEX_AIR,           "AIR",    {}, PF::percent, *this));

    startTimerHz (20);
}

void TexturePanelComponent::timerCallback()
{
    if (visualizer == nullptr)
        return;

    if (auto* a = apvtsRef.getRawParameterValue (AviatorKeyz::ParamID::TEX_AMOUNT))
        visualizer->setAmount (a->load());
    if (auto* f = apvtsRef.getRawParameterValue (AviatorKeyz::ParamID::TEX_FREEZE))
        visualizer->setFrozen (f->load() > 0.5f);
}

void TexturePanelComponent::paint (juce::Graphics& g)
{
    AdvancedKnobHelpers::paintPageBackground (g, *this);
}

void TexturePanelComponent::resized()
{
    const int pad = AviatorTokens::scaledFor (*this, 8);
    auto area = getLocalBounds().reduced (pad);
    const int vizW = juce::jmax (140, area.getWidth() / 3);
    visualizer->setBounds (area.removeFromLeft (vizW));
    area.removeFromLeft (pad);
    auto knobArea = area;
    std::vector<juce::Component*> ptrs;
    for (const auto& k : knobs)
        ptrs.push_back (k.get());
    const int cellW = AviatorTokens::scaledFor (*this, AviatorTokens::kKnobCellW);
    const int cellH = AviatorTokens::scaledFor (*this, AviatorTokens::kKnobCellH);
    CockpitLayout::layoutGrid (knobArea, ptrs, 3, cellW, cellH,
                               AviatorTokens::scaledFor (*this, AviatorTokens::kGridGapX),
                               AviatorTokens::scaledFor (*this, AviatorTokens::kGridGapY));
}
