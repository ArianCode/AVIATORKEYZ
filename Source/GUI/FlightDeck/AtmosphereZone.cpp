#include "AtmosphereZone.h"
#include "../../PluginProcessor.h"
#include "../../State/StateSchema.h"

namespace
{
namespace P = AviatorKeyz::ParamID;
} // namespace

// =============================================================================
//  GrainCloud — particle field: count follows DENSITY, size follows GRAIN,
//  drift speed follows SMEAR, vertical spread follows PITCH SPREAD, and the
//  whole cloud brightens with the post-texture level meter. Frozen texture
//  pins the particles in place.
// =============================================================================
class AtmosphereZone::GrainCloud : public juce::Component,
                                   private juce::Timer
{
public:
    GrainCloud (AviatorKeyzProcessor& p, juce::AudioProcessorValueTreeState& a)
        : processor (p), apvts (a)
    {
        setInterceptsMouseClicks (false, false);
        juce::Random rng (0x47524149);
        for (auto& g : grains)
        {
            g.x = rng.nextFloat();
            g.y = rng.nextFloat();
            g.vx = (rng.nextFloat() - 0.5f) * 0.004f;
            g.r = rng.nextFloat() * 2.4f + 1.0f;
            g.phase = rng.nextFloat() * juce::MathConstants<float>::twoPi;
            g.gold = rng.nextFloat() > 0.8f;
        }
        startTimerHz (30);
    }

    ~GrainCloud() override { stopTimer(); }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        Deck::paintScreen (g, r);

        g.setColour (Deck::screenLine());
        for (int i = 1; i < 4; ++i)
            g.drawHorizontalLine ((int) (r.getY() + r.getHeight() * (float) i / 4.0f), r.getX() + 1.0f, r.getRight() - 1.0f);

        const bool on = apvts.getRawParameterValue (P::PTEX_ON)->load() > 0.5f;
        const float mix = apvts.getRawParameterValue (P::PTEX_MIX)->load();
        const float density = apvts.getRawParameterValue (P::PTEX_DENSITY)->load();
        const float size = apvts.getRawParameterValue (P::PTEX_GRAIN_SIZE)->load();
        const float spread = apvts.getRawParameterValue (P::PTEX_PITCH_SPREAD)->load();
        const float position = apvts.getRawParameterValue (P::PTEX_POSITION)->load();
        const bool frozen = apvts.getRawParameterValue (P::PTEX_FREEZE)->load() > 0.5f;

        const int count = juce::jlimit (8, kMaxGrains, (int) (12 + density * (kMaxGrains - 12)));
        const float level = juce::jlimit (0.0f, 1.0f, meter * 3.0f);
        const float baseAlpha = (on ? 0.35f : 0.12f) + 0.4f * level * (on ? 1.0f : 0.3f);
        const float band = 0.25f + 0.75f * spread;   // vertical band the cloud occupies
        const float centre = 0.5f;

        auto inner = r.reduced (6.0f);
        for (int i = 0; i < count; ++i)
        {
            const auto& gr = grains[(size_t) i];
            const float y = centre + (gr.y - 0.5f) * band;
            const float a = baseAlpha * (0.45f + 0.55f * std::abs (std::sin (gr.phase)));
            const float radius = gr.r * (0.6f + size * 1.2f);
            g.setColour ((gr.gold ? Aviation::gold() : Aviation::cyan()).withAlpha (a));
            g.fillEllipse (inner.getX() + gr.x * inner.getWidth() - radius,
                           inner.getY() + y * inner.getHeight() - radius,
                           radius * 2.0f, radius * 2.0f);
        }

        // read-position marker
        const float px = inner.getX() + position * inner.getWidth();
        g.setColour (Aviation::goldBright().withAlpha (on ? 0.7f : 0.25f));
        g.drawLine (px, inner.getY(), px, inner.getBottom(), 1.0f);

        g.setFont (Deck::mono (8.0f));
        g.setColour (Aviation::textDim());
        g.drawText (frozen ? juce::String::fromUTF8 ("GRAIN CLOUD \xc2\xb7 FROZEN") : (on ? juce::String::fromUTF8 ("GRAIN CLOUD \xc2\xb7 LIVE") : juce::String::fromUTF8 ("GRAIN CLOUD \xc2\xb7 OFF")),
                    9, 5, getWidth() - 18, 12, juce::Justification::centredLeft);
        g.setColour (on ? Aviation::cyan() : Aviation::textDim());
        g.drawText ("MIX " + juce::String (juce::roundToInt (mix * 100.0f)) + "%",
                    9, 5, getWidth() - 18, 12, juce::Justification::centredRight);
    }

private:
    struct Grain { float x, y, vx, r, phase; bool gold; };
    static constexpr int kMaxGrains = 70;

    void timerCallback() override
    {
        const bool frozen = apvts.getRawParameterValue (P::PTEX_FREEZE)->load() > 0.5f;
        const float smear = apvts.getRawParameterValue (P::PTEX_SMEAR)->load();
        const float speed = frozen ? 0.0f : 0.4f + smear * 1.6f;
        const float lvl = processor.getTextureLevel();
        meter = meter * 0.8f + lvl * 0.2f;

        for (auto& g : grains)
        {
            g.x += g.vx * speed;
            if (g.x < 0.0f) g.x += 1.0f;
            if (g.x > 1.0f) g.x -= 1.0f;
            g.phase += frozen ? 0.01f : 0.06f;
        }
        repaint();
    }

    AviatorKeyzProcessor& processor;
    juce::AudioProcessorValueTreeState& apvts;
    Grain grains[kMaxGrains] {};
    float meter { 0.0f };
};

// =============================================================================
AtmosphereZone::AtmosphereZone (AviatorKeyzProcessor& p)
    : processorRef (p), apvts (p.getAPVTS())
{
    cloud = std::make_unique<GrainCloud> (processorRef, apvts);
    addAndMakeVisible (*cloud);

    texturePad = std::make_unique<DeckPad> (apvts, P::PTEX_ON, "TEXTURE", "GRAIN ENGINE", Aviation::cyan());
    freezePad = std::make_unique<DeckPad> (apvts, P::PTEX_FREEZE, "FREEZE", "HOLD BUFFER", Aviation::cyan());
    addAndMakeVisible (*texturePad);
    addAndMakeVisible (*freezePad);

    const struct { const char* id; const char* label; DeckKnob::Format fmt; bool gold; } tex[] = {
        { P::PTEX_GRAIN_SIZE,   "GRAIN",    DeckKnob::Format::percent, false },
        { P::PTEX_DENSITY,      "DENSITY",  DeckKnob::Format::percent, false },
        { P::PTEX_POSITION,     "POSITION", DeckKnob::Format::percent, false },
        { P::PTEX_PITCH_SPREAD, "SPREAD",   DeckKnob::Format::percent, false },
        { P::PTEX_SMEAR,        "SMEAR",    DeckKnob::Format::percent, false },
        { P::PTEX_WIDTH,        "WIDTH",    DeckKnob::Format::percent, false },
        { P::PTEX_MIX,          "TEX MIX",  DeckKnob::Format::percent, true  },
    };
    for (const auto& k : tex)
    {
        auto knob = std::make_unique<DeckKnob> (apvts, k.id, k.label, 52, k.fmt, k.gold);
        addAndMakeVisible (*knob);
        textureKnobs.push_back (std::move (knob));
    }

    const struct { const char* id; const char* label; DeckKnob::Format fmt; bool gold; const char* enable; } fx[] = {
        { P::TONE,          "TONE",   DeckKnob::Format::text,     false, nullptr },
        { P::FX_DIST_DRIVE, "DRIVE",  DeckKnob::Format::percent,  false, P::FX_DIST_ON },
        { P::FX_DELAY_MIX,  "DELAY",  DeckKnob::Format::percent,  false, P::FX_DELAY_ON },
        { P::REVERB_AMOUNT, "REVERB", DeckKnob::Format::percent,  false, P::FX_REVERB_ON },
        { P::OUTPUT_GAIN,   "OUTPUT", DeckKnob::Format::decibels, true,  nullptr },
    };
    for (const auto& k : fx)
    {
        auto knob = std::make_unique<DeckKnob> (apvts, k.id, k.label, 44, k.fmt, k.gold,
                                                k.enable != nullptr ? juce::String (k.enable) : juce::String());
        addAndMakeVisible (*knob);
        fxKnobs.push_back (std::move (knob));
    }
}

AtmosphereZone::~AtmosphereZone() = default;

void AtmosphereZone::resized()
{
    auto body = getLocalBounds().withTrimmedTop (Deck::kZoneHeaderH);

    cloud->setBounds (body.getX() + 14, body.getY() + 12, body.getWidth() - 28, 120);

    // two rows of four: 7 knobs + FREEZE pad (last cell of row 2); TEXTURE pad
    // sits in the CABIN FX divider row on the right.
    const int kw = DeckKnob::preferredWidth (52);
    const int kh = DeckKnob::preferredHeight (52);
    auto rows = juce::Rectangle<int> (body.getX() + 8, cloud->getBottom() + 10, body.getWidth() - 16, kh * 2 + 10);
    auto row1 = rows.removeFromTop (kh);
    rows.removeFromTop (10);
    auto row2 = rows.removeFromTop (kh);
    const int cellW = row1.getWidth() / 4;

    for (int i = 0; i < 4; ++i)
        textureKnobs[(size_t) i]->setBounds (row1.getX() + cellW * i + (cellW - kw) / 2, row1.getY(), kw, kh);
    for (int i = 4; i < 7; ++i)
        textureKnobs[(size_t) i]->setBounds (row2.getX() + cellW * (i - 4) + (cellW - kw) / 2, row2.getY(), kw, kh);
    freezePad->setBounds (row2.getX() + cellW * 3 + (cellW - 92) / 2, row2.getY() + 8, 92, 60);

    // divider row with TEXTURE engage pad
    auto divider = juce::Rectangle<int> (body.getX() + 14, row2.getBottom() + 10, body.getWidth() - 28, 44);
    texturePad->setBounds (divider.removeFromRight (108).withHeight (44));

    const int fw = DeckKnob::preferredWidth (44);
    const int fh = DeckKnob::preferredHeight (44);
    auto fxRow = juce::Rectangle<int> (body.getX() + 8, divider.getBottom() + 8, body.getWidth() - 16, fh);
    const int fcell = fxRow.getWidth() / 5;
    for (int i = 0; i < 5; ++i)
        fxKnobs[(size_t) i]->setBounds (fxRow.getX() + fcell * i + (fcell - fw) / 2, fxRow.getY(), fw, fh);
}

void AtmosphereZone::paint (juce::Graphics& g)
{
    Deck::paintZone (g, getLocalBounds(), "ATMOSPHERE", "GRANULAR TEXTURE + SPACE FX");

    // CABIN FX divider
    auto body = getLocalBounds().withTrimmedTop (Deck::kZoneHeaderH);
    const int y = texturePad->getY();
    juce::Rectangle<int> divider (body.getX() + 14, y - 8, body.getWidth() - 28 - 116, 1);
    g.setColour (juce::Colour (0xff12222f));
    g.fillRect (divider);
    g.setFont (Aviation::label (9.0f, 0.26f));
    g.setColour (Aviation::goldDeep());
    g.drawText ("CABIN FX", divider.getX(), y + 8, 120, 14, juce::Justification::centredLeft);
    g.setFont (Deck::mono (8.0f));
    g.setColour (Aviation::textDim());
    g.drawText (juce::String::fromUTF8 ("POST-CHAIN SENDS \xc2\xb7 LED = ON"), divider.getX(), y + 8, divider.getWidth(), 14, juce::Justification::centredRight);
}
