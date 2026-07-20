#include "CockpitBackground.h"
#include "JetsonicTheme.h"

#include <BinaryData.h>

namespace
{
juce::Image loadEmbeddedImage (const char* resourceName)
{
    int size = 0;
    const char* data = AviatorKeyzBinary::getNamedResource (resourceName, size);
    if (data == nullptr || size <= 0)
        return {};

    juce::MemoryInputStream stream (data, (size_t) size, false);
    return juce::ImageFileFormat::loadFrom (stream);
}

juce::Image findCockpitArtwork (bool& sunsetFound)
{
    for (const char* name : { "cockpit_sunset_jpg", "cockpit_sunset_png", "cockpit_sunset_1x_jpg" })
    {
        auto img = loadEmbeddedImage (name);
        if (img.isValid())
        {
            sunsetFound = true;
            return img;
        }
    }

    sunsetFound = false;
    return loadEmbeddedImage ("cockpit_photo_1x_jpg");
}
} // namespace

CockpitBackground::CockpitBackground()
{
    setOpaque (true);
    setInterceptsMouseClicks (false, false);
    photo = findCockpitArtwork (usingSunsetAsset);
}

void CockpitBackground::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.fillAll (Jetsonic::bgBlack());

    if (photo.isValid())
    {
        // Cover placement preserves the cockpit perspective (no distortion).
        g.drawImage (photo, r, juce::RectanglePlacement::fillDestination
                                   | juce::RectanglePlacement::centred);
    }

    if (! usingSunsetAsset)
    {
        // Warm sunset grade: amber horizon glow + warmed highlights, so the
        // blue source photo harmonises with the gold interface chrome.
        juce::ColourGradient horizon (juce::Colour (0x66ff9a3d), r.getCentreX(), r.getY() + r.getHeight() * 0.30f,
                                      juce::Colour (0x00000000), r.getCentreX(), r.getY() + r.getHeight() * 0.62f,
                                      false);
        horizon.addColour (0.35, juce::Colour (0x3dffc069));
        g.setGradientFill (horizon);
        g.fillRect (r.withHeight (r.getHeight() * 0.65f));

        g.setColour (juce::Colour (0x2e2a1706));
        g.fillAll();
    }

    // Dark translucent treatment for overlay readability without killing the image:
    // top band (under the source dropdown), and a lower console wash.
    juce::ColourGradient top (juce::Colours::black.withAlpha (0.42f), r.getCentreX(), r.getY(),
                              juce::Colours::transparentBlack, r.getCentreX(), r.getY() + 70.0f, false);
    g.setGradientFill (top);
    g.fillRect (r.withHeight (70.0f));

    juce::ColourGradient bottom (juce::Colours::transparentBlack, r.getCentreX(), r.getBottom() - 150.0f,
                                 juce::Colours::black.withAlpha (0.5f), r.getCentreX(), r.getBottom(), false);
    g.setGradientFill (bottom);
    g.fillRect (r.withTop (r.getBottom() - 150.0f));

    // soft edge vignette keeps the frame reading as one recessed module
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.drawRect (r, 1.0f);
}
