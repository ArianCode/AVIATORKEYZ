#include "CockpitPhotoBackground.h"

#include <BinaryData.h>

namespace
{
juce::Image loadCockpitPhoto()
{
    int size = 0;
    const char* data = AviatorKeyzBinary::getNamedResource ("cockpit_photo_1x_jpg", size);
    if (data == nullptr || size <= 0)
        return {};

    auto stream = std::make_unique<juce::MemoryInputStream> (data, (size_t) size, false);
    return juce::ImageFileFormat::loadFrom (*stream);
}
} // namespace

CockpitPhotoBackground::CockpitPhotoBackground()
{
    setOpaque (false);
    cockpitImage = loadCockpitPhoto();
}

void CockpitPhotoBackground::paint (juce::Graphics& g)
{
    if (! photoVisible || ! cockpitImage.isValid())
        return;

    g.drawImage (cockpitImage, getLocalBounds().toFloat(),
                 juce::RectanglePlacement::fillDestination);
}

void CockpitPhotoBackground::resized() {}
