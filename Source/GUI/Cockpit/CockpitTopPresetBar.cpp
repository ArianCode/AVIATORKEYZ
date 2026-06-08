#include "CockpitTopPresetBar.h"
#include "../AviatorTokens.h"

namespace
{
constexpr int kBarPad = 8;
constexpr int kCategoryRowH = 28;
constexpr int kLabelRowH = 16;
constexpr int kCategoryGapX = 4;

class CategoryTabButton : public juce::TextButton
{
public:
    explicit CategoryTabButton (const juce::String& text)
        : juce::TextButton (text)
    {
    }

    void setActiveState (bool a)
    {
        active = a;
        applyLook();
    }

    void mouseEnter (const juce::MouseEvent&) override
    {
        if (! active)
            setColour (juce::TextButton::textColourOffId, AviatorTokens::champagneGold().withAlpha (0.85f));
    }

    void mouseExit (const juce::MouseEvent&) override
    {
        applyLook();
    }

private:
    void applyLook()
    {
        if (active)
        {
            setColour (juce::TextButton::buttonColourId, AviatorTokens::champagneGold());
            setColour (juce::TextButton::textColourOffId, juce::Colour (0xff0a1628));
        }
        else
        {
            setColour (juce::TextButton::buttonColourId, juce::Colour (0xff0d1f33));
            setColour (juce::TextButton::textColourOffId, AviatorTokens::textMuted());
        }
        setConnectedEdges (0);
    }

    void paintButton (juce::Graphics& g, bool highlighted, bool down) override
    {
        juce::ignoreUnused (highlighted, down);
        auto bounds = getLocalBounds().toFloat();
        g.setColour (findColour (juce::TextButton::buttonColourId));
        g.fillRect (bounds);

        g.setFont (AviatorTokens::hudBold (11.f * AviatorTokens::scaleFor (*this)));
        g.setColour (findColour (juce::TextButton::textColourOffId));
        g.drawText (getButtonText(), bounds, juce::Justification::centred);
    }

    bool active { false };
};
} // namespace

CockpitTopPresetBar::CockpitTopPresetBar()
{
    addAndMakeVisible (buildStamp);
}

void CockpitTopPresetBar::styleCategoryButton (juce::TextButton& btn, bool active)
{
    if (auto* tab = dynamic_cast<CategoryTabButton*> (&btn))
    {
        tab->setActiveState (active);
        return;
    }

    if (active)
    {
        btn.setColour (juce::TextButton::buttonColourId, AviatorTokens::champagneGold());
        btn.setColour (juce::TextButton::textColourOffId, juce::Colour (0xff0a1628));
    }
    else
    {
        btn.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff0d1f33));
        btn.setColour (juce::TextButton::textColourOffId, AviatorTokens::textMuted());
    }
    btn.setConnectedEdges (0);
}

void CockpitTopPresetBar::setCategories (const juce::StringArray& categories)
{
    categoryButtons.clear();

    for (const auto& cat : categories)
    {
        auto btn = std::make_unique<CategoryTabButton> (cat.toUpperCase());
        btn->setActiveState (false);
        btn->onClick = [this, cat] {
            if (onCategorySelected)
                onCategorySelected (cat);
        };
        addAndMakeVisible (*btn);
        categoryButtons.push_back (std::move (btn));
    }

    resized();
    repaint();
}

void CockpitTopPresetBar::setActiveCategory (const juce::String& category)
{
    activeCategory = category;

    for (auto& btn : categoryButtons)
    {
        const bool active = btn->getButtonText().equalsIgnoreCase (category);
        styleCategoryButton (*btn, active);
    }
}

void CockpitTopPresetBar::setBuildStampText (const juce::String& text)
{
    buildStamp.setText (text, juce::dontSendNotification);
}

void CockpitTopPresetBar::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    g.setColour (juce::Colour (0xff060c18));
    g.fillRect (bounds);
    g.setColour (AviatorTokens::instrumentCyan().withAlpha (0.55f));
    g.drawHorizontalLine (getHeight() - 1, 0.f, (float) getWidth());

    const float sc = AviatorTokens::scaleFor (*this);
    g.setFont (AviatorTokens::hudBold (11.f * sc));
    g.setColour (AviatorTokens::instrumentCyan().withAlpha (0.9f));
    g.drawText ("FACTORY PRESETS", getLocalBounds().reduced (kBarPad, 4).removeFromTop (kLabelRowH),
                juce::Justification::centredLeft);
}

void CockpitTopPresetBar::resized()
{
    const float sc = AviatorTokens::scaleFor (*this);
    auto area = getLocalBounds().reduced (kBarPad, 4);
    area.removeFromTop (kLabelRowH);

    const int stampW = AviatorTokens::scaledFor (*this, 88);
    auto catBlock = area.removeFromTop (kCategoryRowH);

    buildStamp.setBounds (catBlock.removeFromRight (stampW));
    buildStamp.setJustificationType (juce::Justification::centredRight);
    buildStamp.setFont (AviatorTokens::hudBold (10.f * sc));
    buildStamp.setColour (juce::Label::textColourId, AviatorTokens::textMuted());

    const int n = (int) categoryButtons.size();
    if (n > 0)
    {
        const int totalGap = kCategoryGapX * (n - 1);
        const int btnW = juce::jmax (48, (catBlock.getWidth() - totalGap) / n);

        for (int i = 0; i < n; ++i)
            categoryButtons[(size_t) i]->setBounds (catBlock.getX() + i * (btnW + kCategoryGapX),
                                                      catBlock.getY(),
                                                      btnW,
                                                      kCategoryRowH);
    }
}
