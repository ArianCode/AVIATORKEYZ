#include "MidiHandler.h"
#include "../DSP/SamplerEngine.h"
#include "../DSP/SynthEngine.h"
#include <cstring>

#if JUCE_DEBUG
 #include <juce_core/juce_core.h>
#endif

namespace
{
bool channelMatches (int midiChannel, const juce::MidiMessage& message) noexcept
{
    return midiChannel == 0 || message.getChannel() == midiChannel;
}
} // namespace

void MidiHandler::processBypassed (juce::MidiBuffer& midiBuffer)
{
    for (const auto metadata : midiBuffer)
    {
        const auto message = metadata.getMessage();

        if (! channelMatches (midiChannel, message))
            continue;

        if (message.isNoteOn())
        {
            const int note = message.getNoteNumber();
            if (message.getVelocity() > 0)
                keyHeld[static_cast<size_t> (note)] = true;
            else
                keyHeld[static_cast<size_t> (note)] = false;
        }
        else if (message.isNoteOff())
        {
            keyHeld[static_cast<size_t> (message.getNoteNumber())] = false;
        }
        else if (message.isSustainPedalOn())
        {
            sustainPedal = true;
        }
        else if (message.isSustainPedalOff())
        {
            sustainPedal = false;
        }
        else if (message.isAllNotesOff() || message.isAllSoundOff())
        {
            std::memset (keyHeld, 0, sizeof (keyHeld));
            sustainPedal = false;
        }
    }

    midiBuffer.clear();
}

void MidiHandler::handleMessage (const juce::MidiMessage& message,
                                 SamplerEngine& samplerEngine,
                                 SynthEngine& synthEngine,
                                 bool reverse,
                                 float glideTimeMs,
                                 float sourceBlend,
                                 bool forceSynth)
{
    const bool useSampler = sourceBlend < 0.999f;
    const bool useSynth = forceSynth || sourceBlend > 0.001f;

    if (! channelMatches (midiChannel, message))
        return;

    if (message.isNoteOn())
    {
        const int note = message.getNoteNumber();
        if (message.getVelocity() > 0)
        {
#if JUCE_DEBUG
            DBG ("Note on: " + juce::String (note)
                 + ", velocity: " + juce::String (message.getFloatVelocity()));
#endif
            keyHeld[static_cast<size_t> (note)] = true;
            lastVelocity = message.getFloatVelocity();
            lastNote = note;
            if (useSampler)
                samplerEngine.noteOn (note, message.getFloatVelocity(), reverse, glideTimeMs);
            if (useSynth)
                synthEngine.noteOn (note, message.getFloatVelocity(), glideTimeMs);
        }
        else
        {
            keyHeld[static_cast<size_t> (note)] = false;
            if (! sustainPedal)
            {
                if (useSampler)
                    samplerEngine.noteOff (note);
                if (useSynth)
                    synthEngine.noteOff (note);
            }
        }
    }
    else if (message.isNoteOff())
    {
        const int note = message.getNoteNumber();
        keyHeld[static_cast<size_t> (note)] = false;
        if (! sustainPedal)
        {
            if (useSampler)
                samplerEngine.noteOff (note);
            if (useSynth)
                synthEngine.noteOff (note);
        }
    }
    else if (message.isController() && message.getControllerNumber() == 1)
    {
        modWheel = (float) message.getControllerValue() / 127.f;
    }
    else if (message.isChannelPressure())
    {
        aftertouch = (float) message.getChannelPressureValue() / 127.f;
    }
    else if (message.isAftertouch())
    {
        aftertouch = (float) message.getAfterTouchValue() / 127.f;
    }
    else if (message.isSustainPedalOn())
    {
        sustainPedal = true;
    }
    else if (message.isSustainPedalOff())
    {
        sustainPedal = false;
        for (int n = 0; n < 128; ++n)
        {
            if (! keyHeld[static_cast<size_t> (n)])
            {
                if (useSampler)
                    samplerEngine.noteOff (n);
                if (useSynth)
                    synthEngine.noteOff (n);
            }
        }
    }
    else if (message.isAllNotesOff() || message.isAllSoundOff())
    {
        std::memset (keyHeld, 0, sizeof (keyHeld));
        sustainPedal = false;
        if (message.isAllSoundOff())
        {
            if (useSampler)
                samplerEngine.allSoundOff();
            if (useSynth)
                synthEngine.allSoundOff();
        }
        else
        {
            if (useSampler)
                samplerEngine.allNotesOff();
            if (useSynth)
                synthEngine.allNotesOff();
        }
    }
}

void MidiHandler::process (juce::MidiBuffer& midiBuffer,
                            SamplerEngine& samplerEngine,
                            SynthEngine& synthEngine,
                            bool reverse,
                            float glideTimeMs,
                            float sourceBlend,
                            bool forceSynth)
{
    for (const auto metadata : midiBuffer)
        handleMessage (metadata.getMessage(), samplerEngine, synthEngine,
                       reverse, glideTimeMs, sourceBlend, forceSynth);

    midiBuffer.clear();
}
