#include "TextureBlendEngine.h"

void TextureBlendEngine::prepare (const juce::dsp::ProcessSpec& spec)
{
    textureEngine.prepare (spec);
    dryCopy.setSize (2, juce::jmax (1, static_cast<int> (spec.maximumBlockSize)), false, true, true);
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

    if (buffer.getNumChannels() < 2)
        return; // TextureEngine requires stereo; nothing to blend for mono

    const int pitchSemis = static_cast<int> (settings.pitchSpread * 12.f);
    const float dry = 1.f - settings.mix;
    const int capacity = dryCopy.getNumSamples();

    // Never resize on the audio thread. If the host delivers a block larger
    // than the prepared maximum, process it in capacity-sized chunks through
    // a stack AudioBuffer view (no allocation).
    for (int offset = 0; offset < n; offset += capacity)
    {
        const int len = juce::jmin (capacity, n - offset);
        float* chans[2] = { buffer.getWritePointer (0) + offset,
                            buffer.getWritePointer (1) + offset };
        juce::AudioBuffer<float> sub (chans, 2, len);

        for (int ch = 0; ch < 2; ++ch)
            dryCopy.copyFrom (ch, 0, sub, ch, 0, len);

        textureEngine.process (sub,
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

        for (int ch = 0; ch < 2; ++ch)
            sub.addFrom (ch, 0, dryCopy, ch, 0, len, dry);
    }
}
