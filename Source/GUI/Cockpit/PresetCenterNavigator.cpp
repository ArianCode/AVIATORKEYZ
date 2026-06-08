#include "PresetCenterNavigator.h"

namespace
{
constexpr int kArrowDesign = 20;
constexpr int kArrowGapDesign = 16;
constexpr int kPillPadHDesign = 14;
constexpr int kPillPadVDesign = 6;
constexpr int kCatRowDesign = 14;
} // namespace

PresetCenterNavigator::PresetCenterNavigator()
{
    addAndMakeVisible (prevBtn);
    addAndMakeVisible (nextBtn);
    addAndMakeVisible (favBtn);
    prevBtn.onClick = [this] { if (onPrevPreset) onPrevPreset(); };
    nextBtn.onClick = [this] { if (onNextPreset) onNextPreset(); };
    favBtn.onClick = [this] {
        favBtn.setFavourited (! favBtn.isFavourited());
        if (onFavoriteToggled)
            onFavoriteToggled (favBtn.isFavourited());
    };
    setInterceptsMouseClicks (true, true);
}

void PresetCenterNavigator::setPreset (const juce::String& category, const juce::String& fullPresetName)
{
    categoryText = category.toUpperCase();
    displayName  = PresetDisplayUtils::shortenDisplayName (fullPresetName);
    resized();
    repaint();
}

void PresetCenterNavigator::setFavourited (bool favourited)
{
    favBtn.setFavourited (favourited);
}

void PresetCenterNavigator::resized()
{
    const int arrowW = AviatorTokens::scaledFor (*this, kArrowDesign);
    const int arrowH = AviatorTokens::scaledFor (*this, kArrowDesign);
    const int gap = AviatorTokens::scaledFor (*this, kArrowGapDesign);
    const int favW = AviatorTokens::scaledFor (*this, 22);
    const int padH = AviatorTokens::scaledFor (*this, kPillPadHDesign);
    const int padV = AviatorTokens::scaledFor (*this, kPillPadVDesign);
    const int maxPillW = AviatorTokens::scaledFor (*this, kMaxPillDesignW);

    const juce::Font nameFont = AviatorTokens::hudBold (20.f * AviatorTokens::scaleFor (*this));
    const int textW = juce::jmin (maxPillW - padH * 2 - favW - 4,
                                  juce::roundToInt (nameFont.getStringWidth (displayName)));
    const int pillW = juce::jmin (maxPillW, textW + padH * 2 + favW + 4);
    const int pillH = AviatorTokens::scaledFor (*this, 34);
    const int catH = AviatorTokens::scaledFor (*this, kCatRowDesign);
    const int rowH = juce::jmax (pillH, arrowH);
    const int totalH = catH + 4 + rowH;
    const int totalW = arrowW + gap + pillW + gap + arrowW;

    const juce::Rectangle<int> block (getWidth() / 2 - totalW / 2,
                                      getHeight() / 2 - totalH / 2,
                                      totalW,
                                      totalH);

    auto row = block.withTrimmedTop (catH + 4);
    prevBtn.setBounds (row.removeFromLeft (arrowW).withSizeKeepingCentre (arrowW, arrowH));
    row.removeFromLeft (gap);
    nextBtn.setBounds (row.removeFromRight (arrowW).withSizeKeepingCentre (arrowW, arrowH));
    row.removeFromRight (gap);

    auto pill = row.withSizeKeepingCentre (pillW, pillH);
    pillBounds = pill;
    favBtn.setBounds (pill.removeFromRight (favW).reduced (2, 4));
}

void PresetCenterNavigator::paint (juce::Graphics& g)
{
    const float sc = AviatorTokens::scaleFor (*this);
    const int catH = AviatorTokens::scaledFor (*this, kCatRowDesign);

    juce::Font catFont = AviatorTokens::hudBold (10.f * sc);
    catFont.setExtraKerningFactor (0.18f);
    g.setFont (catFont);
    g.setColour (AviatorTokens::instrumentCyan().withAlpha (0.9f));
    g.drawText (categoryText,
                getWidth() / 2 - AviatorTokens::scaledFor (*this, kMaxPillDesignW) / 2,
                pillBounds.getY() - catH - 4,
                AviatorTokens::scaledFor (*this, kMaxPillDesignW),
                catH,
                juce::Justification::centred);

    g.setColour (juce::Colour (0x99000000));
    g.fillRoundedRectangle (pillBounds.toFloat(), 6.f);

    const int textAreaW = pillBounds.getWidth() - favBtn.getWidth() - 4;
    juce::Font nameFont = AviatorTokens::hudBold (20.f * sc);
    const auto ellipsized = PresetDisplayUtils::ellipsize (displayName, nameFont, textAreaW - 8);

    g.setFont (nameFont);
    g.setColour (juce::Colours::white);
    g.drawText (ellipsized,
                pillBounds.getX() + 4,
                pillBounds.getY(),
                textAreaW,
                pillBounds.getHeight(),
                juce::Justification::centred);
}
