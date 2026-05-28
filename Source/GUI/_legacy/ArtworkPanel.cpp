#include "ArtworkPanel.h"
#include "DesignTokens.h"

namespace
{
    juce::Rectangle<float> windowInset (juce::Rectangle<float> panel, float s)
    {
        return panel.reduced (13.f * s, 9.f * s);
    }
}

uint32_t ArtworkPanel::seededRng (uint32_t& state)
{
    state ^= state >> 16;
    state *= 0x45d9f3bu;
    state ^= state >> 16;
    state *= 0x45d9f3bu;
    state ^= state >> 16;
    return state;
}

ArtworkPanel::ArtworkPanel()
{
    setOpaque (true);
    buildScenery();
    startTimerHz (30);

    prevButton.setColour (juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    prevButton.setColour (juce::TextButton::textColourOffId, DesignTokens::textPrimary().withAlpha (0.25f));
    nextButton.setColour (juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    nextButton.setColour (juce::TextButton::textColourOffId, DesignTokens::textPrimary().withAlpha (0.25f));

    prevButton.onClick = [this] {
        if (onPreviousPreset)
            onPreviousPreset();
    };
    nextButton.onClick = [this] {
        if (onNextPreset)
            onNextPreset();
    };

    addAndMakeVisible (prevButton);
    addAndMakeVisible (nextButton);
}

ArtworkPanel::~ArtworkPanel()
{
    stopTimer();
}

void ArtworkPanel::buildScenery()
{
    auto addLights = [this] (uint32_t seed, int count, float x0, float xSpread, float y0, float ySpread,
                             float rMin, float rMax, float aMin, float aMax, float warmChance) {
        uint32_t state = seed;
        for (int i = 0; i < count; ++i)
        {
            const float u1 = (float) seededRng (state) / (float) UINT32_MAX;
            const float u2 = (float) seededRng (state) / (float) UINT32_MAX;
            const float u3 = (float) seededRng (state) / (float) UINT32_MAX;
            const float u4 = (float) seededRng (state) / (float) UINT32_MAX;
            const float u5 = (float) seededRng (state) / (float) UINT32_MAX;
            const float u6 = (float) seededRng (state) / (float) UINT32_MAX;
            const float u7 = (float) seededRng (state) / (float) UINT32_MAX;

            cityLights.push_back ({
                x0 + u1 * xSpread,
                y0 + u2 * ySpread,
                rMin + u3 * (rMax - rMin),
                aMin + u4 * (aMax - aMin),
                u5 < warmChance,
                u6 * juce::MathConstants<float>::twoPi,
                0.002f + u7 * 0.01f
            });
        }
    };

    addLights (0xAB12CDu, 420, 0.52f, 0.28f, 0.76f, 0.28f, 0.55f, 2.05f, 0.35f, 1.f, 0.82f);
    addLights (0xAB12CEu, 220, 0.04f, 0.30f, 0.78f, 0.20f, 0.4f, 1.4f, 0.15f, 0.5f, 0.65f);
    addLights (0xAB12CFu, 220, 0.68f, 0.30f, 0.78f, 0.20f, 0.4f, 1.4f, 0.15f, 0.5f, 0.65f);
    addLights (0xAB12D0u, 340, 0.f, 1.f, 0.80f, 0.19f, 0.3f, 1.0f, 0.06f, 0.24f, 0.55f);

    uint32_t starSeed = 0xF00BA2u;
    for (int i = 0; i < 160; ++i)
    {
        const float u1 = (float) seededRng (starSeed) / (float) UINT32_MAX;
        const float u2 = (float) seededRng (starSeed) / (float) UINT32_MAX;
        const float u3 = (float) seededRng (starSeed) / (float) UINT32_MAX;
        const float u4 = (float) seededRng (starSeed) / (float) UINT32_MAX;
        const float u5 = (float) seededRng (starSeed) / (float) UINT32_MAX;
        const float u6 = (float) seededRng (starSeed) / (float) UINT32_MAX;
        const float u7 = (float) seededRng (starSeed) / (float) UINT32_MAX;

        stars.push_back ({
            u1,
            u2 * 0.68f,
            0.25f + u3 * 1.f,
            0.15f + u4 * 0.75f,
            u5 < 0.1f,
            u6 * juce::MathConstants<float>::twoPi,
            0.007f + u7 * 0.018f,
            (u1 - 0.5f) * 0.000035f
        });
    }

    uint32_t wispSeed = 0xDEADu;
    for (int i = 0; i < 5; ++i)
    {
        const float u1 = (float) seededRng (wispSeed) / (float) UINT32_MAX;
        const float u2 = (float) seededRng (wispSeed) / (float) UINT32_MAX;
        const float u3 = (float) seededRng (wispSeed) / (float) UINT32_MAX;
        const float u4 = (float) seededRng (wispSeed) / (float) UINT32_MAX;
        const float u5 = (float) seededRng (wispSeed) / (float) UINT32_MAX;
        const float u6 = (float) seededRng (wispSeed) / (float) UINT32_MAX;

        wisps.push_back ({
            0.1f + u1 * 0.8f,
            0.08f + u2 * 0.4f,
            90.f + u3 * 130.f,
            18.f + u4 * 30.f,
            0.012f + u5 * 0.022f,
            (u6 - 0.5f) * 0.000025f
        });
    }
}

void ArtworkPanel::setPresetInfo (const PresetDisplayInfo& info)
{
    displayInfo = info;
    repaint();
}

void ArtworkPanel::triggerPresetFlash()
{
    flashAlpha = 1.f;
}

void ArtworkPanel::timerCallback()
{
    artTime += 1.f / 30.f;

    for (auto& s : stars)
    {
        s.x += s.drift;
        if (s.x < -0.01f) s.x = 1.01f;
        if (s.x > 1.01f) s.x = -0.01f;
        s.phase += s.speed;
    }

    for (auto& w : wisps)
    {
        w.x += w.drift;
        if (w.x < -0.15f) w.x = 1.15f;
        if (w.x > 1.15f) w.x = -0.15f;
    }

    for (auto& cl : cityLights)
        cl.phase += cl.speed;

    if (flashAlpha > 0.f)
        flashAlpha = juce::jmax (0.f, flashAlpha - 0.055f);

    repaint();
}

void ArtworkPanel::resized()
{
    const float s = DesignTokens::scaleFactorFor (*this);
    auto bounds = getLocalBounds();

    prevButton.setBounds (DesignTokens::scaled (24, s), bounds.getCentreY() - DesignTokens::scaled (14, s),
                          DesignTokens::scaled (28, s), DesignTokens::scaled (28, s));
    nextButton.setBounds (bounds.getWidth() - DesignTokens::scaled (52, s),
                          bounds.getCentreY() - DesignTokens::scaled (14, s),
                          DesignTokens::scaled (28, s), DesignTokens::scaled (28, s));

    sceneBufferValid = false;
    rebuildSceneBuffer();
}

void ArtworkPanel::rebuildSceneBuffer()
{
    const float s = DesignTokens::scaleFactorFor (*this);
    const auto window = windowInset (getLocalBounds().toFloat(), s);
    const int w = juce::roundToInt (window.getWidth());
    const int h = juce::roundToInt (window.getHeight());

    if (w <= 0 || h <= 0)
    {
        sceneBufferValid = false;
        return;
    }

    sceneBuffer = juce::Image (juce::Image::ARGB, w, h, true);
    juce::Graphics g (sceneBuffer);
    drawStaticScenery (g, { 0.f, 0.f, (float) w, (float) h });
    sceneBufferValid = true;
}

void ArtworkPanel::paint (juce::Graphics& g)
{
    const float s = DesignTokens::scaleFactorFor (*this);
    const auto panel = getLocalBounds().toFloat();
    g.fillAll (DesignTokens::surface());

    const auto window = windowInset (panel, s);

    {
        juce::Graphics::ScopedSaveState clip (g);
        g.reduceClipRegion (window.toNearestInt());

        if (sceneBufferValid)
            g.drawImageAt (sceneBuffer, (int) window.getX(), (int) window.getY());
        else
            drawStaticScenery (g, window);

        drawTwinkleOverlay (g, window);

        if (flashAlpha > 0.f)
        {
            g.setColour (DesignTokens::champagne().withAlpha (flashAlpha * 0.035f));
            g.fillRect (window);
        }
    }

    g.setColour (DesignTokens::champagne().withAlpha (0.20f));
    g.drawRoundedRectangle (window, 16.f * s, 1.f * s);

    juce::ColourGradient vignette (
        juce::Colours::transparentBlack, window.getCentreX(), window.getY(),
        juce::Colour (0x4008080e), window.getCentreX(), window.getBottom(), false);
    vignette.addColour (0.78, juce::Colour (0x9006060c));
    vignette.addColour (1.0, juce::Colour (0xc006060c));
    g.setGradientFill (vignette);
    g.fillRoundedRectangle (window, 16.f * s);

    const int hudX = (int) window.getRight() - DesignTokens::scaled (26, s);
    g.setFont (DesignTokens::monoFont (7.f * s));
    g.setColour (DesignTokens::champagne().withAlpha (0.22f));
    g.drawText (displayInfo.hudAlt, hudX - DesignTokens::scaled (180, s),
                (int) window.getY() + DesignTokens::scaled (11, s),
                DesignTokens::scaled (60, s), DesignTokens::scaled (12, s),
                juce::Justification::centredRight);
    g.drawText (displayInfo.hudHdg, hudX - DesignTokens::scaled (110, s),
                (int) window.getY() + DesignTokens::scaled (11, s),
                DesignTokens::scaled (60, s), DesignTokens::scaled (12, s),
                juce::Justification::centredRight);
    g.drawText (displayInfo.hudRoute, hudX - DesignTokens::scaled (10, s),
                (int) window.getY() + DesignTokens::scaled (11, s),
                DesignTokens::scaled (100, s), DesignTokens::scaled (12, s),
                juce::Justification::centredRight);

    const int overlayBottom = (int) window.getBottom() - DesignTokens::scaled (18, s);
    g.setFont (DesignTokens::labelFont (7.f * s, juce::Font::bold));
    g.setColour (DesignTokens::champagne().withAlpha (0.7f));
    g.drawText (displayInfo.categoryTag, (int) window.getX() + DesignTokens::scaled (14, s),
                overlayBottom - DesignTokens::scaled (58, s), DesignTokens::scaled (400, s),
                DesignTokens::scaled (10, s), juce::Justification::bottomLeft);

    g.setFont (DesignTokens::heroFont (46.f * s));
    g.setColour (DesignTokens::textPrimary());
    g.drawText (displayInfo.heroName, (int) window.getX() + DesignTokens::scaled (14, s),
                overlayBottom - DesignTokens::scaled (52, s), DesignTokens::scaled (500, s),
                DesignTokens::scaled (50, s), juce::Justification::bottomLeft);

    g.setFont (DesignTokens::labelFont (8.f * s));
    g.setColour (DesignTokens::textPrimary().withAlpha (0.3f));
    g.drawText (displayInfo.subtitle, (int) window.getX() + DesignTokens::scaled (14, s),
                overlayBottom - DesignTokens::scaled (4, s), DesignTokens::scaled (500, s),
                DesignTokens::scaled (12, s), juce::Justification::bottomLeft);
}

void ArtworkPanel::drawStaticScenery (juce::Graphics& g, juce::Rectangle<float> bounds)
{
    const float W = bounds.getWidth();
    const float H = bounds.getHeight();

    juce::ColourGradient sky (juce::Colour (0xff03030c), bounds.getX(), bounds.getY(),
                              juce::Colour (0xff110c09), bounds.getX(), bounds.getBottom(), false);
    sky.addColour (0.45, juce::Colour (0xff050511));
    sky.addColour (0.65, juce::Colour (0xff070714));
    sky.addColour (0.72, juce::Colour (0xff0a0810));
    sky.addColour (0.80, juce::Colour (0xff0e0a0c));
    g.setGradientFill (sky);
    g.fillRect (bounds);

    const float cityY = bounds.getY() + H * 0.72f;
    juce::ColourGradient dome (juce::Colour (0x12d29b37), bounds.getCentreX(), cityY,
                               juce::Colours::transparentBlack,
                               bounds.getCentreX(), cityY + W * 0.55f, true);
    g.setGradientFill (dome);
    g.fillRect (bounds);

    juce::ColourGradient horizon (juce::Colours::transparentBlack,
                                  bounds.getX(), cityY,
                                  juce::Colours::transparentBlack,
                                  bounds.getRight(), cityY, false);
    horizon.addColour (0.5, DesignTokens::champagne().withAlpha (0.18f));
    g.setGradientFill (horizon);
    g.drawLine (bounds.getX(), cityY, bounds.getRight(), cityY, 0.5f);

    const float vpX = bounds.getCentreX();
    const float vpY = cityY;
    const float roadSpreads[] = { -0.52f, -0.26f, 0.f, 0.26f, 0.52f };

    for (float spread : roadSpreads)
    {
        const float bx = bounds.getX() + W * (0.5f + spread * 1.15f);
        const float by = bounds.getBottom();
        g.setColour (juce::Colour (215, 175, 85).withAlpha (0.04f));
        g.drawLine (vpX, vpY, bx, by, 0.8f);
    }

    g.setColour (DesignTokens::champagne().withAlpha (0.03f));
    g.fillRect (bounds.getX() + W * 0.25f, bounds.getY() + H * 0.22f, W * 0.5f, H * 0.125f);
}

void ArtworkPanel::drawTwinkleOverlay (juce::Graphics& g, juce::Rectangle<float> bounds)
{
    const float W = bounds.getWidth();
    const float H = bounds.getHeight();

    for (const auto& w : wisps)
    {
        const float wx = bounds.getX() + w.x * W;
        const float wy = bounds.getY() + w.y * H;
        const float rx = w.rx * (W / (float) DesignTokens::kDesignWidth);
        const float ry = w.ry * (W / (float) DesignTokens::kDesignWidth);
        juce::ColourGradient wg (juce::Colour (0x1c6e69af).withAlpha (w.a),
                                 wx, wy,
                                 juce::Colours::transparentBlack,
                                 wx + rx, wy, false);
        g.setGradientFill (wg);
        g.fillEllipse (wx - rx, wy - ry, rx * 2.f, ry * 2.f);
    }

    for (const auto& s : stars)
    {
        const float a = s.a * (0.72f + 0.28f * std::sin (s.phase));
        const auto col = s.warm ? juce::Colour (255, 225, 160).withAlpha (a)
                                : juce::Colour (210, 220, 255).withAlpha (a);
        const float sr = s.r * (W / (float) DesignTokens::kDesignWidth);
        const float sx = bounds.getX() + s.x * W;
        const float sy = bounds.getY() + s.y * H;
        g.setColour (col);
        g.fillEllipse (sx - sr, sy - sr, sr * 2.f, sr * 2.f);
    }

    for (const auto& cl : cityLights)
    {
        const float a = cl.a * (0.82f + 0.18f * std::sin (cl.phase));
        const auto col = cl.warm ? juce::Colour (225, 175, 75).withAlpha (a)
                                 : juce::Colour (155, 175, 225).withAlpha (a * 0.55f);
        const float lr = cl.r * (W / (float) DesignTokens::kDesignWidth);
        const float lx = bounds.getX() + cl.x * W;
        const float ly = bounds.getY() + cl.y * H;
        g.setColour (col);
        g.fillEllipse (lx - lr, ly - lr, lr * 2.f, lr * 2.f);
    }
}
