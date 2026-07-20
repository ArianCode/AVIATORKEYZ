#include "MacroDeck.h"
#include "AviationIcons.h"
#include "AviationTheme.h"
#include "../../State/StateSchema.h"

namespace
{
constexpr int kCellW = 172;
constexpr int kBrandHalfW = 78;
} // namespace

MacroDeck::MacroDeck (juce::AudioProcessorValueTreeState& apvts)
{
    namespace P = AviatorKeyz::ParamID;

    struct Spec { const char* id; const char* title; const char* sub; };
    const Spec specs[] = {
        { P::GLIDE_TIME,    "THROTTLE",   "Glide" },
        { P::INPUT_GAIN,    "ENGINE",     "Gain" },
        { P::STEREO_WIDTH,  "WINGS",      "Brightness" },
        { P::REVERB_AMOUNT, "ALTITUDE",   "Reverb" },
        { P::TONE,          "CABIN",      "Tone" },
        { P::SMEAR,         "TURBULENCE", "Filter" },
        { P::ENV_ATTACK,    "ATTACK",     "Attack" },
        { P::ENV_RELEASE,   "RELEASE",    "Release" },
    };

    for (const auto& spec : specs)
    {
        auto knob = std::make_unique<MacroKnob> (apvts, spec.id, spec.title, spec.sub);
        addAndMakeVisible (*knob);
        knobs.push_back (std::move (knob));
    }
}

void MacroDeck::resized()
{
    // Four knob cells mirrored on each side of the central brand block.
    const float centre = (float) getWidth() * 0.5f;
    const float offsets[] = { -670.0f, -500.0f, -330.0f, -160.0f,
                               160.0f,  330.0f,  500.0f,  670.0f };
    const int y = 6;
    const int h = getHeight() - 10;

    for (size_t i = 0; i < knobs.size(); ++i)
    {
        const int cx = juce::roundToInt (centre + offsets[i]);
        knobs[i]->setBounds (cx - kCellW / 2, y, kCellW, h);
    }
}

void MacroDeck::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();

    // Continuous black leather/metal panel.
    {
        juce::ColourGradient grad (juce::Colour (0xff10161d), r.getX(), r.getY(),
                                   juce::Colour (0xff05080c), r.getX(), r.getBottom(), false);
        grad.addColour (0.12, juce::Colour (0xff0b1117));
        g.setGradientFill (grad);
        g.fillRoundedRectangle (r, 9.0f);

        // leather-like texture: sparse darker stipple rows
        g.setColour (juce::Colours::black.withAlpha (0.14f));
        for (int row = 0; row < 6; ++row)
        {
            const float y = r.getY() + 16.0f + (float) row * 28.0f;
            for (float x = r.getX() + 10.0f + (float) ((row % 2) * 9); x < r.getRight() - 8.0f; x += 18.0f)
                g.fillEllipse (x, y, 1.6f, 1.6f);
        }

        g.setColour (Aviation::goldDeep().withAlpha (0.40f));
        g.drawRoundedRectangle (r.reduced (0.5f), 9.0f, 1.0f);
        Aviation::topSpecular (g, r, 0.07f);
    }

    const float cx = r.getCentreX();

    // fine gold dividers between sections (stronger around the brand block)
    g.setColour (Aviation::gold().withAlpha (0.16f));
    for (float offset : { 245.0f, 415.0f, 585.0f })
    {
        g.fillRect (juce::Rectangle<float> (cx - offset, r.getY() + 18.0f, 1.0f, r.getHeight() - 36.0f));
        g.fillRect (juce::Rectangle<float> (cx + offset, r.getY() + 18.0f, 1.0f, r.getHeight() - 36.0f));
    }
    g.setColour (Aviation::gold().withAlpha (0.35f));
    g.fillRect (juce::Rectangle<float> (cx - (float) kBrandHalfW, r.getY() + 12.0f, 1.0f, r.getHeight() - 24.0f));
    g.fillRect (juce::Rectangle<float> (cx + (float) kBrandHalfW, r.getY() + 12.0f, 1.0f, r.getHeight() - 24.0f));

    // --- Center brand section ------------------------------------------------
    AviationIcons::fill (g, AviationIcons::wingLogo(),
                         { cx - 34.0f, r.getY() + 26.0f, 68.0f, 52.0f },
                         Aviation::goldBright());

    g.setFont (Aviation::label (15.0f, 0.30f));
    g.setColour (Aviation::goldBright());
    g.drawText ("AVIATION", (int) cx - kBrandHalfW, (int) r.getY() + 86,
                kBrandHalfW * 2, 18, juce::Justification::centred);

    g.setFont (Aviation::label (8.5f, 0.32f));
    g.setColour (Aviation::gold().withAlpha (0.8f));
    g.drawText ("FLAGSHIP SERIES", (int) cx - kBrandHalfW, (int) r.getY() + 106,
                kBrandHalfW * 2, 12, juce::Justification::centred);
}
