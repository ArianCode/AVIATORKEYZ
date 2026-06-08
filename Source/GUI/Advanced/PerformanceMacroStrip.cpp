#include "PerformanceMacroStrip.h"
#include "AdvancedWidgets.h"
#include "../AviatorTokens.h"

namespace
{
namespace P = AviatorKeyz::ParamID;

const char* kMacroIds[] { P::PERF_MACRO_1, P::PERF_MACRO_2, P::PERF_MACRO_3, P::PERF_MACRO_4 };
} // namespace

PerformanceMacroStrip::PerformanceMacroStrip (juce::AudioProcessorValueTreeState& apvts)
{
    for (int i = 0; i < 4; ++i)
    {
        macros[(size_t) i] = std::make_unique<PrecisionKnob> (apvts,
                                                              kMacroIds[i],
                                                              "MACRO " + juce::String (i + 1),
                                                              "Performance",
                                                              PrecisionKnob::ValueFormat::percent);
        addAndMakeVisible (*macros[(size_t) i]);
    }
}

void PerformanceMacroStrip::paint (juce::Graphics& g)
{
    const float sc = AviatorTokens::scaleFor (*this);
    auto bounds = getLocalBounds();

    g.setColour (juce::Colour (0xff0a1424));
    g.fillRect (bounds);

    g.setColour (AdvancedWidgets::kCyanWave().withAlpha (0.35f));
    g.fillRect (bounds.getX(), bounds.getY(), bounds.getWidth(), 1);

    g.setFont (AviatorTokens::hudBold (8.f * sc));
    g.setColour (AdvancedWidgets::kGold().withAlpha (0.9f));
    g.drawText ("PERFORMANCE MACROS",
                bounds.reduced (AviatorTokens::scaledFor (*this, 10), 4).removeFromTop (AviatorTokens::scaledFor (*this, 14)),
                juce::Justification::centredLeft);
}

void PerformanceMacroStrip::resized()
{
    auto area = getLocalBounds().reduced (AviatorTokens::scaledFor (*this, 10), 8);
    area.removeFromTop (AviatorTokens::scaledFor (*this, 18));

    const int gap = AviatorTokens::scaledFor (*this, 12);
    const int knobW = juce::jmax (AviatorTokens::scaledFor (*this, 88),
                                  (area.getWidth() - gap * 3) / 4);

    for (auto& macro : macros)
    {
        macro->setBounds (area.removeFromLeft (knobW));
        area.removeFromLeft (gap);
    }
}
