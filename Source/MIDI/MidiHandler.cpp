#include "MidiHandler.h"
#include "../DSP/SamplerEngine.h"

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
            keyHeld.fill (false);
            sustainPedal = false;
        }
    }

    midiBuffer.clear();
}

void MidiHandler::process (juce::MidiBuffer& midiBuffer,
                            SamplerEngine& samplerEngine,
                            bool reverse,
                            float glideTimeMs)
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
            {
                keyHeld[static_cast<size_t> (note)] = true;
                samplerEngine.noteOn (note,
                                      message.getFloatVelocity(),
                                      reverse,
                                      glideTimeMs);
            }
            else
            {
                keyHeld[static_cast<size_t> (note)] = false;
                if (! sustainPedal)
                    samplerEngine.noteOff (note);
            }
        }
        else if (message.isNoteOff())
        {
            const int note = message.getNoteNumber();
            keyHeld[static_cast<size_t> (note)] = false;
            if (! sustainPedal)
                samplerEngine.noteOff (note);
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
                    samplerEngine.noteOff (n);
            }
        }
        else if (message.isAllNotesOff() || message.isAllSoundOff())
        {
            keyHeld.fill (false);
            sustainPedal = false;
            if (message.isAllSoundOff())
                samplerEngine.allSoundOff();
            else
                samplerEngine.allNotesOff();
        }
    }

    midiBuffer.clear();
}
