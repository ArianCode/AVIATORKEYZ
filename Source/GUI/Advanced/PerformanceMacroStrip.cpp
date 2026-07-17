#include "PerformanceMacroStrip.h"
#include "AdvancedWidgets.h"
#include "EffectCellGrid.h"

namespace
{
namespace P = AviatorKeyz::ParamID;

const char* kMacroIds[] { P::PERF_MACRO_1, P::PERF_MACRO_2, P::PERF_MACRO_3, P::PERF_MACRO_4 };
} // namespace

PerformanceMacroStrip::PerformanceMacroStrip (juce::AudioProcessorValueTreeState& apvts)
{
    for (int i = 0; i < 4; ++i)
    {
        macros[(size_t) i] = std::make_unique<EffectCell> (
            apvts, kMacroIds[i], "MACRO " + juce::String (i + 1), "^v DRAG",
            "Drag up/down — this macro can be assigned to modulate multiple parameters at once.",
            EffectCell::Format::percent);
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

    const int tileH = EffectCellGrid::scaledBoxH (*this);
    const int colGap = AviatorTokens::scaledFor (*this, EffectCellGrid::kColGapDesign);
    const int tileW = EffectCellGrid::scaledBoxW (*this, area.getWidth(), 4);

    std::vector<juce::Component*> ptrs;
    for (auto& macro : macros)
        ptrs.push_back (macro.get());

    EffectCellGrid::layoutRow (area, ptrs, tileW, tileH, colGap);
}

void PerformanceMacroStrip::setMacroLabels (const std::array<juce::String, 4>& labels)
{
    for (int i = 0; i < 4; ++i)
    {
        if (macros[(size_t) i] != nullptr)
            macros[(size_t) i]->setTitle (labels[(size_t) i].isNotEmpty()
                                              ? labels[(size_t) i]
                                              : "MACRO " + juce::String (i + 1));
    }
    repaint();
}
