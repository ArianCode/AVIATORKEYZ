#include "EffectParamBox.h"
#include "AdvancedWidgets.h"

EffectParamBox::EffectParamBox (juce::AudioProcessorValueTreeState& apvts,
                                const juce::String& title,
                                std::vector<EffectParamRowSpec> rows)
    : titleText (title)
{
    setOpaque (false);

    for (auto& spec : rows)
    {
        auto row = std::make_unique<EffectParamRow> (apvts,
                                                     spec.paramId,
                                                     spec.label,
                                                     spec.tooltip,
                                                     spec.format,
                                                     spec.choices);
        addAndMakeVisible (*row);
        paramRows.push_back (std::move (row));
    }
}

void EffectParamBox::paint (juce::Graphics& g)
{
    const float s = AviatorTokens::scaleFor (*this);
    g.reduceClipRegion (getLocalBounds());

    auto bounds = getLocalBounds().toFloat().reduced (0.5f);
    const float corner = 5.f * s;

    juce::ColourGradient bg (juce::Colour (0xff0e151d), bounds.getX(), bounds.getY(),
                             juce::Colour (0xff070a0e), bounds.getX(), bounds.getBottom(), false);
    g.setGradientFill (bg);
    g.fillRoundedRectangle (bounds, corner);

    g.setColour (AviatorTokens::instrumentCyan().withAlpha (0.28f));
    g.drawRoundedRectangle (bounds, corner, 1.f * s);

    const int headerH = AviatorTokens::scaled (16, s);
    auto header = bounds.removeFromTop ((float) headerH).reduced (6.f * s, 2.f * s);
    g.setFont (AviatorTokens::hudBold (9.f * s));
    g.setColour (AdvancedWidgets::kGold().withAlpha (0.92f));
    g.drawText (titleText, header, juce::Justification::centredLeft);

    const int railW = AviatorTokens::scaled (10, s);
    const int railX = getWidth() - railW - AviatorTokens::scaled (4, s);
    const int contentTop = headerH + AviatorTokens::scaled (2, s);
    const int contentBottom = getHeight() - AviatorTokens::scaled (2, s);
    const int contentH = juce::jmax (1, contentBottom - contentTop);

    const int trackX = railX + (railW - AviatorTokens::scaled (3, s)) / 2;
    const int trackW = AviatorTokens::scaled (3, s);
    g.setColour (juce::Colours::white.withAlpha (0.10f));
    g.fillRoundedRectangle ((float) trackX, (float) contentTop,
                            (float) trackW, (float) contentH, 1.5f * s);

    const int rowCount = (int) paramRows.size();
    if (rowCount <= 0)
        return;

    const int rowH = contentH / rowCount;
    for (int i = 0; i < rowCount; ++i)
    {
        auto* row = paramRows[(size_t) i].get();
        if (row == nullptr || row->isChoiceRow())
            continue;

        juce::Rectangle<int> rowBounds (0, contentTop + i * rowH, getWidth(), rowH);
        row->paintThumb (g, rowBounds, s);
    }
}

void EffectParamBox::resized()
{
    const float s = AviatorTokens::scaleFor (*this);
    const int headerH = AviatorTokens::scaled (16, s);
    const int railW = AviatorTokens::scaled (10, s);
    const int railX = getWidth() - railW - AviatorTokens::scaled (4, s);

    auto content = getLocalBounds().withTrimmedTop (headerH + AviatorTokens::scaled (2, s));
    const int rowCount = (int) paramRows.size();
    if (rowCount <= 0)
        return;

    const int rowH = juce::jmax (1, content.getHeight() / rowCount);
    for (int i = 0; i < rowCount; ++i)
    {
        auto rowBounds = content.removeFromTop (rowH);
        paramRows[(size_t) i]->setRailColumnX (railX);
        paramRows[(size_t) i]->setBounds (rowBounds);
    }
}
