#include "LayerMixPanel.h"
#include "JetsonicTheme.h"
#include "../../State/StateSchema.h"

LayerMixPanel::LayerMixPanel (juce::AudioProcessorValueTreeState& apvts)
    : apvtsRef (apvts)
{
    namespace P = AviatorKeyz::ParamID;

    const std::pair<const char*, const char*> specs[] = {
        { P::OSC1_LEVEL, "OSC1" },
        { P::OSC2_LEVEL, "OSC2" },
        { P::TEX_AMOUNT, "TEX" },
        { P::PTEX_MIX,   "GRN" },
    };
    for (const auto& [id, label] : specs)
    {
        auto fader = std::make_unique<MiniFader> (apvtsRef, id, label);
        addAndMakeVisible (*fader);
        faders.push_back (std::move (fader));
    }

    texToggle = std::make_unique<MiniToggle> (apvtsRef, P::TEX_ENABLED, "TEX");
    addAndMakeVisible (*texToggle);
    grainToggle = std::make_unique<MiniToggle> (apvtsRef, P::PTEX_ON, "GRN");
    addAndMakeVisible (*grainToggle);
}

void LayerMixPanel::resized()
{
    auto r = getLocalBounds().reduced (10, 4);
    r.removeFromTop (18);

    auto toggles = r.removeFromRight (26);
    toggles.removeFromTop (6);
    texToggle->setBounds (toggles.removeFromTop (34));
    toggles.removeFromTop (8);
    grainToggle->setBounds (toggles.removeFromTop (34));

    const int faderW = r.getWidth() / (int) faders.size();
    for (auto& fader : faders)
        fader->setBounds (r.removeFromLeft (faderW).reduced (2, 0));
}

void LayerMixPanel::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    Jetsonic::fillGlassScreen (g, r, 6.0f, 0.28f);

    g.setFont (Jetsonic::label (9.5f, 0.12f));
    g.setColour (Jetsonic::gold().withAlpha (0.92f));
    g.drawText ("LAYER MIX", r.toNearestInt().removeFromTop (18), juce::Justification::centred);
}
