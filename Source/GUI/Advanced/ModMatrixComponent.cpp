#include "ModMatrixComponent.h"
#include "../../DSP/ModMatrix.h"

namespace
{
void styleCombo (juce::ComboBox& box)
{
    box.setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff0d1f33));
    box.setColour (juce::ComboBox::textColourId, AviatorTokens::textPrimary());
    box.setColour (juce::ComboBox::outlineColourId, AviatorTokens::instrumentCyan().withAlpha (0.25f));
    box.setColour (juce::ComboBox::arrowColourId, AviatorTokens::instrumentCyan());
}

void styleOnButton (juce::TextButton& btn, bool on)
{
    btn.setButtonText (on ? "ON" : "OFF");
    btn.setColour (juce::TextButton::buttonColourId,
                   on ? AviatorTokens::instrumentCyan().withAlpha (0.35f) : juce::Colour (0xff0d1f33));
    btn.setColour (juce::TextButton::textColourOffId,
                   on ? AviatorTokens::textPrimary() : AviatorTokens::textMuted());
}
} // namespace

ModMatrixComponent::ModMatrixComponent (juce::AudioProcessorValueTreeState& apvts)
    : apvtsRef (apvts)
{
    hintLabel.setText ("Double-click any knob to assign mod depth (colored arc). Alt-drag arc to adjust.",
                       juce::dontSendNotification);
    hintLabel.setFont (AviatorTokens::hud (9.f));
    hintLabel.setColour (juce::Label::textColourId, AviatorTokens::textMuted());
    addAndMakeVisible (hintLabel);

    const auto sources = ModMatrix::sourceNames();
    const auto dests   = ModMatrix::destNames();

    for (int i = 0; i < kNumRows; ++i)
    {
        auto& row = rows[(size_t) i];
        const auto& ids = ModRoutingHub::kRows[i];

        row.sourceBox.addItemList (sources, 1);
        styleCombo (row.sourceBox);

        row.destLabel.setFont (AviatorTokens::hud (10.f));
        row.destLabel.setColour (juce::Label::textColourId, AviatorTokens::textPrimary());
        row.destLabel.setJustificationType (juce::Justification::centredLeft);

        row.onButton.setClickingTogglesState (true);
        row.onButton.onClick = [this, i]
        {
            const auto& id = ModRoutingHub::kRows[i].on;
            const bool on = apvtsRef.getRawParameterValue (id)->load() > 0.5f;
            styleOnButton (rows[(size_t) i].onButton, on);
        };

        row.sourceAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (apvts, ids.source, row.sourceBox);
        row.enableAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (apvts, ids.on, row.onButton);

        addAndMakeVisible (row.onButton);
        addAndMakeVisible (row.sourceBox);
        addAndMakeVisible (row.destLabel);
    }

    refreshDestLabels();
    startTimerHz (10);
}

void ModMatrixComponent::refreshDestLabels()
{
    const auto dests = ModMatrix::destNames();
    for (int i = 0; i < kNumRows; ++i)
    {
        auto& row = rows[(size_t) i];
        int di = 0;
        if (auto* p = dynamic_cast<juce::AudioParameterChoice*> (apvtsRef.getParameter (ModRoutingHub::kRows[i].dest)))
            di = p->getIndex();
        row.destLabel.setText (dests[juce::jlimit (0, dests.size() - 1, di)], juce::dontSendNotification);

        const bool on = apvtsRef.getRawParameterValue (ModRoutingHub::kRows[i].on)->load() > 0.5f;
        styleOnButton (row.onButton, on);
    }
}

void ModMatrixComponent::timerCallback()
{
    refreshDestLabels();
}

void ModMatrixComponent::paint (juce::Graphics& g)
{
    const float sc = AviatorTokens::scaleFor (*this);
    AviatorTokens::paintGlassPanel (g, getLocalBounds().toFloat(), false);

    g.setFont (AviatorTokens::hudBold (9.f * sc));
    g.setColour (AviatorTokens::champagneGold());
    const juce::StringArray headers { "ON", "SOURCE", "DESTINATION" };
    const std::array<float, 3> xs { 0.f, 0.1f, 0.42f };
    const int rowH = (getHeight() - 36) / kNumRows;

    for (int h = 0; h < headers.size(); ++h)
        g.drawText (headers[h], (int) (getWidth() * xs[(size_t) h]), 22,
                    (int) (getWidth() * 0.35f), rowH, juce::Justification::centredLeft);
}

void ModMatrixComponent::resized()
{
    const int pad = 4;
    hintLabel.setBounds (pad, 2, getWidth() - pad * 2, 18);

    const int rowH = (getHeight() - 36) / kNumRows;
    const int ew = juce::jmax (36, (int) (getWidth() * 0.08f));
    const int sw = juce::jmax (100, (int) (getWidth() * 0.32f));
    const int dw = getWidth() - ew - sw - pad * 4;

    for (int i = 0; i < kNumRows; ++i)
    {
        const int y = 22 + i * rowH + 1;
        int x = pad;
        auto& row = rows[(size_t) i];
        row.onButton.setBounds (x, y, ew, rowH - 3);  x += ew + pad;
        row.sourceBox.setBounds (x, y, sw, rowH - 3); x += sw + pad;
        row.destLabel.setBounds (x, y, dw, rowH - 3);
    }
}
