#include "LayerMixPanel.h"
#include "AviationTheme.h"
#include "../../State/StateSchema.h"
#include "../../DSP/Mfx/MfxDescriptors.h"

LayerMixPanel::LayerMixPanel (juce::AudioProcessorValueTreeState& apvts)
    : apvtsRef (apvts)
{
    namespace P = AviatorKeyz::ParamID;

    // GRN used to be the ATMOSPHERE grain mix; that engine now lives in MFX
    // slot B, so the fourth fader is slot B's level.
    const std::pair<juce::String, const char*> specs[] = {
        { P::OSC1_LEVEL, "OSC1" },
        { P::OSC2_LEVEL, "OSC2" },
        { P::TEX_AMOUNT, "TEX" },
        { Mfx::levelId (1), "FX B" },
    };
    for (const auto& [id, label] : specs)
    {
        auto fader = std::make_unique<MiniFader> (apvtsRef, id, label);
        addAndMakeVisible (*fader);
        faders.push_back (std::move (fader));
    }

    texToggle = std::make_unique<MiniToggle> (apvtsRef, P::TEX_ENABLED, "TEX");
    addAndMakeVisible (*texToggle);
    grainToggle = std::make_unique<MiniToggle> (apvtsRef, Mfx::onId (1), "FX B");
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
    Aviation::fillGlassScreen (g, r, 6.0f, 0.28f);

    g.setFont (Aviation::label (9.5f, 0.12f));
    g.setColour (Aviation::gold().withAlpha (0.92f));
    g.drawText ("LAYER MIX", r.toNearestInt().removeFromTop (18), juce::Justification::centred);
}
