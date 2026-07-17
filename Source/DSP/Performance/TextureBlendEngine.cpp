#include "TextureBlendEngine.h"

void TextureBlendEngine::prepare (const juce::dsp::ProcessSpec& spec)
{
    textureEngine.prepare (spec);
    dryCopy.setSize (2, static_cast<int> (spec.maximumBlockSize), false, true, true);
    prepared = true;
}

void TextureBlendEngine::reset()
{
    textureEngine.reset();
}

void TextureBlendEngine::process (juce::AudioBuffer<float>& buffer,
                                   const TextureSettings& settings,
                                   double hostBpm) noexcept
{
    if (! prepared)
        return;

    const int n = buffer.getNumSamples();
    if (n <= 0)
        return;

    if (! settings.enabled || settings.mix < 0.001f)
        return;

    dryCopy.setSize (buffer.getNumChannels(), n, false, false, true);
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        dryCopy.copyFrom (ch, 0, buffer, ch, 0, n);

    const int pitchSemis = static_cast<int> (settings.pitchSpread * 12.f);
    textureEngine.process (buffer,
                           true,
                           settings.mix,
                           settings.freeze,
                           juce::jlimit (0.f, 1.f, settings.density),
                           settings.grainSize,
                           pitchSemis,
                           settings.density,
                           settings.pitchSpread,
                           0.f,
                           settings.smear,
                           0.f,
                           0.f,
                           false,
                           settings.width,
                           settings.position,
                           hostBpm);

    const float dry = 1.f - settings.mix;
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        buffer.addFrom (ch, 0, dryCopy, ch, 0, n, dry);
    }
}
