#include "FlightDeckView.h"
#include "../../PluginProcessor.h"
#include "../../State/SampleAnalysis.h"
#include "../../State/StateSchema.h"
#include "../Cockpit/PresetDisplayUtils.h"

namespace
{
namespace P = AviatorKeyz::ParamID;
} // namespace

FlightDeckView::FlightDeckView (AviatorKeyzProcessor& p)
    : processorRef (p)
{
    setOpaque (true);

    slotA = std::make_unique<MfxSlotPanel> (processorRef, 0);
    slotB = std::make_unique<MfxSlotPanel> (processorRef, 1);
    maneuverZone = std::make_unique<ManeuverZone> (processorRef);
    cargoZone = std::make_unique<CargoHoldZone> (processorRef);
    samplerStrip = std::make_unique<RollingSamplerStrip> (processorRef);
    // A region sent to cargo becomes the active sound: refresh that waveform.
    samplerStrip->onRegionLoaded = [this] { cargoZone->refreshFromProcessor(); };

    for (juce::Component* c : { (juce::Component*) slotA.get(), (juce::Component*) maneuverZone.get(),
                                (juce::Component*) slotB.get(), (juce::Component*) cargoZone.get(),
                                (juce::Component*) samplerStrip.get() })
        addAndMakeVisible (c);

    setSize (Deck::kDesignW, Deck::kDesignH);
    refreshPresetUI();
}

FlightDeckView::~FlightDeckView() = default;

void FlightDeckView::refreshPresetUI()
{
    cargoZone->refreshFromProcessor();
}


juce::Rectangle<int> FlightDeckView::titleStripBounds() const
{
    const auto page = Deck::pageBounds();
    return { page.getX() + 12, page.getY() + 6, page.getWidth() - 24, 42 };
}

void FlightDeckView::resized()
{
    // Fill the page below the shared header: title strip, then the zone grid
    // spanning the full width. The left column (slot A + maneuver) and slot B
    // keep the prototype's 788:530 width ratio; cargo runs the full width.
    const auto page = Deck::pageBounds();
    const auto strip = titleStripBounds();
    const int gap = 12;
    const int x0 = strip.getX();
    const int totalW = strip.getWidth();
    const int zonesTop = strip.getBottom() + 8;
    const int zonesBottom = page.getBottom() - 8;

    const int leftW = (totalW - gap) * 788 / 1318;
    const int rightW = totalW - gap - leftW;

    const int cargoH = 240;
    const int upperH = zonesBottom - zonesTop - gap - cargoH;
    const int maneuverH = 150;
    const int slotAH = upperH - gap - maneuverH;

    samplerStrip->setBounds (strip);
    slotA->setBounds (x0, zonesTop, leftW, slotAH);
    maneuverZone->setBounds (x0, zonesTop + slotAH + gap, leftW, maneuverH);
    slotB->setBounds (x0 + leftW + gap, zonesTop, rightW, upperH);
    cargoZone->setBounds (x0, zonesBottom - cargoH, totalW, cargoH);
}

void FlightDeckView::paint (juce::Graphics& g)
{
    // deep cockpit night — radial glow at the top centre
    g.fillAll (Aviation::bgBlack());
    juce::ColourGradient glow (juce::Colour (0xff0c1824), (float) getWidth() * 0.5f, -80.0f,
                               juce::Colour (0xff02070c), (float) getWidth() * 0.5f, (float) getHeight() * 0.75f, true);
    glow.addColour (0.55, juce::Colour (0xff040a11));
    g.setGradientFill (glow);
    g.fillRect (getLocalBounds());

    // The title strip is the RollingSamplerStrip's own component — the
    // "FLIGHT DECK / PERFORMANCE · AVIATORKEYZ" watermark used to live here and
    // was dropped in favour of the sampler, which also carries the readout.
}
