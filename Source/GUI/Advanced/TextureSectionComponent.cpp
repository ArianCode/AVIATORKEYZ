#include "TextureSectionComponent.h"

namespace
{
namespace P = AviatorKeyz::ParamID;

constexpr int kPadDesign       = 10;
constexpr int kHeaderHDesign   = 14;
constexpr int kBoxHDesign      = 56;
constexpr int kBoxMinWDesign   = 60;
constexpr int kRowGapDesign    = 10;
constexpr int kColGapDesign    = 8;
constexpr int kToggleWDesign   = 58;
constexpr int kToggleHDesign   = 18;

std::unique_ptr<BracketValueBox> makeBox (juce::AudioProcessorValueTreeState& apvts,
                                          const char* id,
                                          const juce::String& label,
                                          BracketValueBox::Format fmt)
{
    return std::make_unique<BracketValueBox> (apvts, id, label, fmt, AdvancedWidgets::kGold());
}
} // namespace

TextureSectionComponent::TextureSectionComponent (juce::AudioProcessorValueTreeState& apvts)
    : apvtsRef (apvts)
{
    visualizer = std::make_unique<TextureVisualizerComponent>();
    addAndMakeVisible (*visualizer);

    freezeToggle  = std::make_unique<AdvancedWidgets::FlatToggle> (apvts, P::TEX_FREEZE,  "FREEZE", "FREEZE");
    reverseToggle = std::make_unique<AdvancedWidgets::FlatToggle> (apvts, P::TEX_REVERSE, "REV", "REV");
    addAndMakeVisible (*freezeToggle);
    addAndMakeVisible (*reverseToggle);

    grainBoxes.push_back (makeBox (apvts, P::TEX_AMOUNT,        "AMT",     Fmt::percent));
    grainBoxes.push_back (makeBox (apvts, P::TEX_WIDTH,         "WIDTH",   Fmt::percent));
    grainBoxes.push_back (makeBox (apvts, P::TEX_GRAIN_SCAN,    "SCAN",    Fmt::percent));
    grainBoxes.push_back (makeBox (apvts, P::TEX_GRAIN_RATE,    "RATE",    Fmt::percent));
    grainBoxes.push_back (makeBox (apvts, P::TEX_GRAIN_SIZE,    "SIZE",    Fmt::percent));
    grainBoxes.push_back (makeBox (apvts, P::TEX_GRAIN_DENSITY, "DENSITY", Fmt::percent));
    grainBoxes.push_back (makeBox (apvts, P::TEX_GRAIN_SPREAD,  "SPREAD",  Fmt::percent));
    grainBoxes.push_back (makeBox (apvts, P::TEX_GRAIN_PITCH,   "PITCH",   Fmt::semitones));
    grainBoxes.push_back (makeBox (apvts, P::TEX_GRAIN_PAN,     "PAN",     Fmt::pan));

    motionBoxes.push_back (makeBox (apvts, P::TEX_MOTION, "MOTION", Fmt::percent));
    motionBoxes.push_back (makeBox (apvts, P::TEX_DRIFT,  "DRIFT",  Fmt::percent));
    motionBoxes.push_back (makeBox (apvts, P::TEX_AIR,    "AIR",    Fmt::percent));

    for (auto& box : grainBoxes)
        addAndMakeVisible (*box);
    for (auto& box : motionBoxes)
        addAndMakeVisible (*box);
}

void TextureSectionComponent::paint (juce::Graphics& g)
{
    const float sc = AviatorTokens::scaleFor (*this);
    auto bounds = getLocalBounds();

    g.setColour (juce::Colour (0xff0a1424));
    g.fillRect (bounds);

    g.setColour (AviatorTokens::instrumentCyan().withAlpha (0.55f));
    g.fillRect (bounds.getX(), bounds.getY(), 1, bounds.getHeight());

    g.setFont (AviatorTokens::hudBold (8.f * sc));
    g.setColour (AdvancedWidgets::kGold().withAlpha (0.92f));
    g.drawText ("GRAIN CONTROLS", grainHeaderArea, juce::Justification::centredLeft);

    g.setColour (AdvancedWidgets::kCyanWave().withAlpha (0.35f));
    g.fillRect (subgroupDividerArea.withHeight (1));

    g.setColour (AdvancedWidgets::kGold().withAlpha (0.92f));
    g.drawText ("MOTION CONTROLS", motionHeaderArea, juce::Justification::centredLeft);
}

void TextureSectionComponent::layoutParamGrid (juce::Rectangle<int> area,
                                               int boxW,
                                               int boxH,
                                               int rowGap,
                                               int colGap)
{
    grainHeaderArea = area.removeFromTop (AviatorTokens::scaledFor (*this, kHeaderHDesign));
    area.removeFromTop (AviatorTokens::scaledFor (*this, 4));

    const int cols = 3;
    const int grainRows = 3;
    for (int row = 0; row < grainRows; ++row)
    {
        const size_t start = (size_t) row * (size_t) cols;
        std::vector<BV*> rowBoxes;
        for (int c = 0; c < cols; ++c)
        {
            const size_t idx = start + (size_t) c;
            if (idx < grainBoxes.size())
                rowBoxes.push_back (grainBoxes[idx].get());
        }

        auto rowArea = area.removeFromTop (boxH);
        auto r = rowArea;
        for (auto* bx : rowBoxes)
        {
            bx->setBounds (r.removeFromLeft (boxW).withHeight (boxH));
            r.removeFromLeft (colGap);
        }
        area.removeFromTop (rowGap);
    }

    area.removeFromTop (AviatorTokens::scaledFor (*this, 4));
    subgroupDividerArea = area.removeFromTop (1);
    area.removeFromTop (AviatorTokens::scaledFor (*this, 8));

    motionHeaderArea = area.removeFromTop (AviatorTokens::scaledFor (*this, kHeaderHDesign));
    area.removeFromTop (AviatorTokens::scaledFor (*this, 4));

    auto motionRow = area.removeFromTop (boxH);
    auto r = motionRow;
    for (auto& box : motionBoxes)
    {
        box->setBounds (r.removeFromLeft (boxW).withHeight (boxH));
        r.removeFromLeft (colGap);
    }
}

void TextureSectionComponent::resized()
{
    auto bounds = getLocalBounds().reduced (AviatorTokens::scaledFor (*this, kPadDesign));

    const int vizW = juce::roundToInt ((float) bounds.getWidth() * 0.55f);
    auto vizArea = bounds.removeFromLeft (vizW);
    bounds.removeFromLeft (AviatorTokens::scaledFor (*this, kPadDesign));

    const int minVizH = AviatorTokens::scaledFor (*this, kDesignMinHeight);
    visualizer->setBounds (vizArea.withHeight (juce::jmax (minVizH, vizArea.getHeight())));

    auto overlay = visualizer->getBounds();
    const int toggleW = AviatorTokens::scaledFor (*this, kToggleWDesign);
    const int toggleH = AviatorTokens::scaledFor (*this, kToggleHDesign);
    const int overlayPad = AviatorTokens::scaledFor (*this, 8);
    freezeToggle->setBounds (overlay.getX() + overlayPad,
                             overlay.getBottom() - toggleH - overlayPad,
                             toggleW,
                             toggleH);
    reverseToggle->setBounds (overlay.getRight() - toggleW - overlayPad,
                              overlay.getBottom() - toggleH - overlayPad,
                              toggleW,
                              toggleH);

    const int boxW = juce::jmax (AviatorTokens::scaledFor (*this, kBoxMinWDesign),
                                 (bounds.getWidth() - AviatorTokens::scaledFor (*this, kColGapDesign) * 2) / 3);
    const int boxH = AviatorTokens::scaledFor (*this, kBoxHDesign);
    const int rowGap = AviatorTokens::scaledFor (*this, kRowGapDesign);
    const int colGap = AviatorTokens::scaledFor (*this, kColGapDesign);

    layoutParamGrid (bounds, boxW, boxH, rowGap, colGap);

    freezeToggle->toFront (false);
    reverseToggle->toFront (false);
}

void TextureSectionComponent::syncVisualizerFromParams()
{
    if (visualizer == nullptr)
        return;

    auto load = [this] (const char* id) -> float
    {
        if (auto* raw = apvtsRef.getRawParameterValue (id))
            return raw->load();
        return 0.f;
    };

    visualizer->setAmount (load (P::TEX_AMOUNT));
    visualizer->setFrozen (load (P::TEX_FREEZE) > 0.5f);
    visualizer->setReverse (load (P::TEX_REVERSE) > 0.5f);
    visualizer->setRate (load (P::TEX_GRAIN_RATE));
    visualizer->setSize (load (P::TEX_GRAIN_SIZE));
    visualizer->setDensity (load (P::TEX_GRAIN_DENSITY));
    visualizer->setSpread (load (P::TEX_GRAIN_SPREAD));
    visualizer->setMotion (load (P::TEX_MOTION));
    visualizer->setDrift (load (P::TEX_DRIFT));
}
