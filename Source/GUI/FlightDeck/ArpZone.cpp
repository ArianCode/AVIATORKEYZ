#include "ArpZone.h"
#include "../../PluginProcessor.h"
#include "../../DSP/ArpPattern.h"
#include "../../State/StateSchema.h"

namespace
{
namespace P = AviatorKeyz::ParamID;
constexpr int kColumns = 16;
} // namespace

// =============================================================================
//  StepViz — 16-column lane grid. The sequence is rebuilt from the held-note
//  mask + current settings with the same ArpPattern code the DSP uses, so the
//  picture is the pattern the engine is actually stepping through.
// =============================================================================
class ArpZone::StepViz : public juce::Component,
                         private juce::Timer
{
public:
    StepViz (AviatorKeyzProcessor& p, juce::AudioProcessorValueTreeState& a)
        : processor (p), apvts (a)
    {
        setInterceptsMouseClicks (false, false);
        startTimerHz (30);
    }

    ~StepViz() override { stopTimer(); }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        Deck::paintScreen (g, r);

        const auto& ui = processor.getArpUiState();
        const bool on = apvts.getRawParameterValue (P::ARP_ON)->load() > 0.5f;

        // held notes + sequence
        Arp::HeldNote held[Arp::kMaxHeld];
        int numHeld = 0;
        const uint64_t lo = ui.heldMaskLo.load (std::memory_order_relaxed);
        const uint64_t hi = ui.heldMaskHi.load (std::memory_order_relaxed);
        for (int n = 0; n < 128 && numHeld < Arp::kMaxHeld; ++n)
        {
            const bool set = n < 64 ? ((lo >> n) & 1u) : ((hi >> (n - 64)) & 1u);
            if (set)
                held[numHeld++] = { n, 1.f, numHeld };
        }

        const auto mode = static_cast<Arp::Mode> (juce::jlimit (0, 4, (int) apvts.getRawParameterValue (P::ARP_MODE)->load()));
        const int octaves = juce::jlimit (1, 4, (int) apvts.getRawParameterValue (P::ARP_OCTAVES)->load());
        Arp::Step seq[Arp::kMaxSequence];
        const int seqLen = Arp::buildSequence (held, numHeld, mode, octaves, seq, Arp::kMaxSequence);

        // lanes = distinct notes sorted ascending
        std::vector<int> lanes;
        for (int i = 0; i < seqLen; ++i)
            if (std::find (lanes.begin(), lanes.end(), seq[i].note) == lanes.end())
                lanes.push_back (seq[i].note);
        std::sort (lanes.begin(), lanes.end());
        const int numLanes = juce::jmax (3, (int) lanes.size());

        auto grid = r.reduced (1.0f).withTrimmedTop (20.0f).withTrimmedBottom (8.0f);
        const float cw = grid.getWidth() / (float) kColumns;
        const float lh = grid.getHeight() / (float) numLanes;

        g.setColour (Deck::screenLine());
        for (int i = 0; i <= kColumns; ++i)
            g.drawVerticalLine ((int) (grid.getX() + i * cw), grid.getY(), grid.getBottom());
        for (int l = 0; l <= numLanes; ++l)
            g.drawHorizontalLine ((int) (grid.getY() + l * lh), grid.getX(), grid.getRight());

        // running column highlight
        const int running = ui.running.load (std::memory_order_relaxed) && on ? displayColumn : -1;
        if (running >= 0)
        {
            g.setColour (Aviation::goldBright().withAlpha (0.08f));
            g.fillRect (juce::Rectangle<float> (grid.getX() + running * cw, grid.getY(), cw, grid.getHeight()));
        }

        if (seqLen > 0)
        {
            const int current = ui.currentIndex.load (std::memory_order_relaxed);
            for (int col = 0; col < kColumns; ++col)
            {
                const int idx = (col + columnOffset) % seqLen;
                const auto& step = seq[idx];
                const int lane = (int) (std::find (lanes.begin(), lanes.end(), step.note) - lanes.begin());
                const bool active = running >= 0 && col == running && idx == current;
                const float y = grid.getBottom() - (float) (lane + 1) * lh;
                juce::Rectangle<float> cell (grid.getX() + col * cw + 3.0f, y + 2.0f, cw - 6.0f, lh - 4.0f);

                if (active)
                {
                    g.setColour (Aviation::goldBright().withAlpha (0.35f));
                    g.fillRoundedRectangle (cell.expanded (3.0f), 4.0f);
                    g.setColour (Aviation::goldBright());
                }
                else
                {
                    g.setColour (Aviation::cyan().withAlpha ((step.octave % 2) ? 0.85f : 0.5f).withAlpha (on ? 1.0f : 0.35f));
                }
                g.fillRoundedRectangle (cell, 3.0f);
            }
        }

        // readouts
        g.setFont (Deck::mono (8.0f));
        g.setColour (Aviation::textDim());
        juce::String heldText = "HELD: ";
        if (numHeld == 0)
            heldText << juce::String::fromUTF8 ("\xe2\x80\x94") << "   PLAY A CHORD";
        else
        {
            std::sort (held, held + numHeld, [] (const Arp::HeldNote& a, const Arp::HeldNote& b) { return a.note < b.note; });
            for (int i = 0; i < juce::jmin (numHeld, 6); ++i)
                heldText << (i ? juce::String::fromUTF8 (" \xc2\xb7 ") : "") << Deck::noteName (held[i].note);
            if (numHeld > 6) heldText << " +" << juce::String (numHeld - 6);
        }
        g.drawText (heldText, 9, 5, getWidth() / 2, 12, juce::Justification::centredLeft);

        static const char* modeNames[] = { "UP", "DOWN", "UP-DN", "RANDOM", "AS PLAYED" };
        static const char* rateNames[] = { "1/4", "1/8", "1/16", "1/32" };
        static const char* feelNames[] = { "", "T", "D" };
        const int rate = juce::jlimit (0, 3, (int) apvts.getRawParameterValue (P::ARP_RATE)->load());
        const int feel = juce::jlimit (0, 2, (int) apvts.getRawParameterValue (P::ARP_FEEL)->load());
        juce::String read = juce::String (modeNames[(int) mode]) + juce::String::fromUTF8 (" \xc2\xb7 ") + juce::String (octaves) + juce::String::fromUTF8 (" OCT \xc2\xb7 ")
                            + rateNames[rate] + feelNames[feel];
        if (! on)
            read = juce::String::fromUTF8 ("STANDBY \xc2\xb7 ") + read;
        g.setColour (on ? Aviation::cyan() : Aviation::textDim());
        g.drawText (read, getWidth() / 2, 5, getWidth() / 2 - 9, 12, juce::Justification::centredRight);
    }

private:
    void timerCallback() override
    {
        const auto& ui = processor.getArpUiState();
        const int idx = ui.currentIndex.load (std::memory_order_relaxed);
        const int len = ui.sequenceLength.load (std::memory_order_relaxed);
        if (idx != lastIndex)
        {
            if (idx >= 0)
            {
                displayColumn = (displayColumn + 1) % kColumns;
                // keep column↔index aligned when the sequence wraps
                if (len > 0)
                    columnOffset = ((idx - displayColumn) % len + len) % len;
            }
            lastIndex = idx;
        }
        if (! ui.running.load (std::memory_order_relaxed))
        {
            displayColumn = -1;
            columnOffset = 0;
        }
        repaint();
    }

    AviatorKeyzProcessor& processor;
    juce::AudioProcessorValueTreeState& apvts;
    int lastIndex { -1 };
    int displayColumn { -1 };
    int columnOffset { 0 };
};

// =============================================================================
ArpZone::ArpZone (AviatorKeyzProcessor& p)
    : processorRef (p), apvts (p.getAPVTS())
{
    viz = std::make_unique<StepViz> (processorRef, apvts);
    addAndMakeVisible (*viz);

    modePads = std::make_unique<ModePads> (apvts, P::ARP_MODE);
    addAndMakeVisible (*modePads);

    holdPad = std::make_unique<DeckPad> (apvts, P::ARP_HOLD, "HOLD", "LATCH CHORD", Deck::green());
    addAndMakeVisible (*holdPad);
    engagePad = std::make_unique<DeckPad> (apvts, P::ARP_ON, "ENGAGE", "ARP ON / OFF", Aviation::gold());
    addAndMakeVisible (*engagePad);

    rateSeg = std::make_unique<DeckSegment> (apvts, P::ARP_RATE,
        std::vector<DeckSegment::Option> { { "1/4", 0 }, { "1/8", 1 }, { "1/16", 2 }, { "1/32", 3 } }, "RATE");
    feelSeg = std::make_unique<DeckSegment> (apvts, P::ARP_FEEL,
        std::vector<DeckSegment::Option> { { "STR", 0 }, { "TRIP", 1 }, { "DOT", 2 } }, "FEEL");
    targetSeg = std::make_unique<DeckSegment> (apvts, P::ARP_TARGET,
        std::vector<DeckSegment::Option> { { "SLICES", 0 }, { "NOTES", 1 } }, "ARP TARGET");
    for (auto* s : { rateSeg.get(), feelSeg.get(), targetSeg.get() })
        addAndMakeVisible (*s);

    octaves = std::make_unique<OctaveStepper> (apvts, P::ARP_OCTAVES);
    addAndMakeVisible (*octaves);

    const struct { const char* id; const char* label; bool gold; } knobSpecs[] = {
        { P::ARP_GATE,       "GATE",       false },
        { P::ARP_SWING,      "SWING",      false },
        { P::ARP_HUMANIZE,   "HUMANIZE",   false },
        { P::ARP_OCT_SPREAD, "OCT SPREAD", true  },
    };
    for (const auto& k : knobSpecs)
    {
        auto knob = std::make_unique<DeckKnob> (apvts, k.id, k.label, 44, DeckKnob::Format::percent, k.gold);
        addAndMakeVisible (*knob);
        knobs.push_back (std::move (knob));
    }
}

ArpZone::~ArpZone() = default;

void ArpZone::resized()
{
    auto body = getLocalBounds().withTrimmedTop (Deck::kZoneHeaderH);

    // visualizer
    viz->setBounds (body.getX() + 14, body.getY() + 10, body.getWidth() - 28, 158);

    // mode pads + hold + engage
    auto ctrls = juce::Rectangle<int> (body.getX() + 14, viz->getBottom() + 8, body.getWidth() - 28, 56);
    auto engage = ctrls.removeFromRight (96);
    ctrls.removeFromRight (10);
    auto hold = ctrls.removeFromRight (112);
    ctrls.removeFromRight (10);
    modePads->setBounds (ctrls);
    holdPad->setBounds (hold);
    engagePad->setBounds (engage);

    // lower row
    auto row = juce::Rectangle<int> (body.getX() + 14, ctrls.getBottom() + 12, body.getWidth() - 28, 76);
    const int segH = DeckSegment::kSegH + DeckSegment::kCaptionH + 2;
    auto placeSeg = [&] (DeckSegment& s, int w) {
        s.setBounds (row.removeFromLeft (w).withHeight (segH).withY (row.getBottom() - segH - 6));
        row.removeFromLeft (11);
    };
    placeSeg (*rateSeg, rateSeg->preferredWidth (10));
    placeSeg (*feelSeg, feelSeg->preferredWidth (10));

    auto oct = row.removeFromLeft (OctaveStepper::kW);
    octaves->setBounds (oct.withHeight (OctaveStepper::kH).withY (row.getBottom() - segH - 6));
    row.removeFromLeft (11);

    // target seg pinned to the right
    auto target = row.removeFromRight (targetSeg->preferredWidth (12));
    targetSeg->setBounds (target.withHeight (segH).withY (row.getBottom() - segH - 6));
    row.removeFromRight (11);

    const int kw = DeckKnob::preferredWidth (44);
    const int kh = DeckKnob::preferredHeight (44);
    for (auto& knob : knobs)
    {
        knob->setBounds (row.removeFromLeft (kw).withHeight (kh).withY (row.getBottom() - kh));
        row.removeFromLeft (8);
    }
}

void ArpZone::paint (juce::Graphics& g)
{
    Deck::paintZone (g, getLocalBounds(), "ARPEGGIATOR",
                     juce::String::fromUTF8 ("MIDI ARP \xc2\xb7 HOST SYNC \xc2\xb7 PLAYS PHRASE SLICES OR CHROMATIC NOTES"));
}
