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
    for (juce::Component* c : { (juce::Component*) slotA.get(), (juce::Component*) maneuverZone.get(),
                                (juce::Component*) slotB.get(), (juce::Component*) cargoZone.get() })
        addAndMakeVisible (c);

    setSize (Deck::kDesignW, Deck::kDesignH);
    startTimerHz (8);
    refreshPresetUI();
}

FlightDeckView::~FlightDeckView()
{
    stopTimer();
}

void FlightDeckView::refreshPresetUI()
{
    cargoZone->refreshFromProcessor();
    timerCallback();
}

void FlightDeckView::timerCallback()
{
    auto& pm = processorRef.getPresetManager();
    const auto& info = processorRef.getUserSampleInfo();

    static const char* modeNames[] = { "ONE-SHOT", "PHRASE", "CHROMATIC", "STRETCH", "SLICE-PHRASE" };
    const int mode = juce::jlimit (0, 4, (int) processorRef.getAPVTS().getRawParameterValue (P::SRC_PLAYBACK_MODE)->load());

    juce::String src = info.loaded ? info.name.toUpperCase()
                                   : PresetDisplayUtils::shortenDisplayName (pm.getCurrentPresetName()).toUpperCase();
    if (src.length() > 22)
        src = src.substring (0, 21) + juce::String::fromUTF8 ("\xe2\x80\xa6");
    const juce::String key = info.loaded ? SampleAnalysis::keyName (info.keyPitchClass, info.keyMinor).toUpperCase()
                                         : Deck::noteName (pm.getCurrentRootNote());

    juce::String l1 = "SRC " + src + juce::String::fromUTF8 (" \xc2\xb7 KEY ") + key + juce::String::fromUTF8 (" \xc2\xb7 ")
                      + juce::String (juce::roundToInt (processorRef.getLastKnownHostBpm())) + " BPM";
    juce::String l2 = juce::String ("MODE ") + modeNames[mode] + juce::String::fromUTF8 (" \xc2\xb7 VOICES ")
                      + juce::String::formatted ("%02d", processorRef.getActiveVoiceCount());

    if (l1 != sysReadLine1 || l2 != sysReadLine2)
    {
        sysReadLine1 = l1;
        sysReadLine2 = l2;
        repaint (titleStripBounds().withLeft (getWidth() / 2));
    }
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

    // title strip (the shared top header sits above this page)
    auto hdr = titleStripBounds();
    g.setFont (Aviation::label (13.0f, 0.40f));
    g.setColour (Aviation::gold());
    g.drawText ("FLIGHT DECK", hdr.withHeight (22), juce::Justification::centredLeft);
    g.setFont (Aviation::label (8.0f, 0.30f));
    g.setColour (Aviation::textDim());
    g.drawText (juce::String::fromUTF8 ("PERFORMANCE \xc2\xb7 AVIATORKEYZ"), hdr.withTrimmedTop (24).withHeight (12), juce::Justification::centredLeft);

    // system readout — highlight the values in cyan
    auto drawSysLine = [&] (const juce::String& line, juce::Rectangle<int> area)
    {
        // split on " · " so labels stay dim and values light up
        auto tokens = juce::StringArray::fromTokens (line, false);
        juce::GlyphArrangement ga;
        float x = 0.0f;
        const auto f = Deck::mono (9.0f);
        struct Piece { juce::String text; bool value; };
        std::vector<Piece> pieces;
        static const juce::StringArray labels { "SRC", "KEY", "MODE", "VOICES", juce::String::fromUTF8 ("\xc2\xb7") };
        bool nextIsValue = false;
        for (const auto& t : tokens)
        {
            const bool isLabel = labels.contains (t);
            pieces.push_back ({ t + " ", ! isLabel && (nextIsValue || t.containsOnly ("0123456789") || t == "BPM") });
            nextIsValue = isLabel && t != juce::String::fromUTF8 ("\xc2\xb7");
        }
        float total = 0.0f;
        for (const auto& pc : pieces)
            total += juce::GlyphArrangement::getStringWidth (f, pc.text);
        x = (float) area.getRight() - total;
        for (const auto& pc : pieces)
        {
            g.setFont (f);
            g.setColour (pc.value ? Aviation::cyan() : Aviation::textDim());
            const float w = juce::GlyphArrangement::getStringWidth (f, pc.text);
            g.drawText (pc.text, (int) x, area.getY(), (int) w + 2, area.getHeight(), juce::Justification::centredLeft);
            x += w;
        }
        juce::ignoreUnused (ga);
    };
    drawSysLine (sysReadLine1, hdr.withTrimmedTop (4).withHeight (14));
    drawSysLine (sysReadLine2, hdr.withTrimmedTop (20).withHeight (14));
}
