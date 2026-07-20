#include "EffectCellFormat.h"

namespace EffectCellFormat
{
juce::String formatPan (float v)
{
    if (std::abs (v) < 0.005f)
        return "C";
    return juce::String (std::abs (v), 2) + (v < 0.f ? " L" : " R");
}

juce::String formatValue (juce::AudioProcessorValueTreeState& apvts,
                          const juce::String& paramId,
                          Format format,
                          float v)
{
    switch (format)
    {
        case Format::percent:
            if (auto* p = dynamic_cast<juce::RangedAudioParameter*> (apvts.getParameter (paramId)))
            {
                const float norm = p->getNormalisableRange().convertTo0to1 (v);
                if (paramId.contains ("level") || paramId.contains ("amount") || paramId.contains ("mix")
                    || paramId.contains ("depth") || paramId.contains ("sustain") || paramId.contains ("shape")
                    || paramId.contains ("blend") || paramId.contains ("spread") || paramId.contains ("width")
                    || paramId.contains ("density") || paramId.contains ("motion") || paramId.contains ("drift")
                    || paramId.contains ("air") || paramId.contains ("scan") || paramId.contains ("size")
                    || (paramId.contains ("rate") && ! paramId.contains ("lfo") && ! paramId.contains ("chorus"))
                    || paramId.contains ("smear") || paramId.contains ("reverb_amount") || paramId.contains ("lofi")
                    || paramId.contains ("dist") || paramId.contains ("damp") || paramId.contains ("feedback")
                    || paramId.contains ("resonance")
                    || (paramId.contains ("drive") && ! paramId.contains ("filter"))
                    || (paramId.contains ("mod_") && paramId.contains ("amount")))
                    return juce::String (juce::roundToInt (norm * 100.f)) + "%";
            }
            return juce::String (juce::roundToInt (v * 100.f)) + "%";

        case Format::bipolarPercent:
            return juce::String (juce::roundToInt (v * 100.f)) + "%";

        case Format::hz:
            return juce::String (v, v < 10.f ? 2 : 1) + " Hz";

        case Format::ms:
            if (v >= 1000.f)
                return juce::String (v / 1000.f, 2) + "s";
            return juce::String (juce::roundToInt (v)) + " ms";

        case Format::seconds:
            if (v >= 1.f)
                return juce::String (v, 2) + "s";
            return juce::String (v * 1000.f, 1) + " ms";

        case Format::semitones:
            return juce::String (juce::roundToInt (v)) + " st";

        case Format::cents:
            return juce::String (juce::roundToInt (v)) + " ct";

        case Format::pan:
            return formatPan (v);

        case Format::cutoff:
            if (v >= 1000.f)
                return juce::String (v / 1000.f, 1) + "k";
            return juce::String (juce::roundToInt (v));

        case Format::decibels:
            return juce::String (v, v > -10.f && v < 10.f ? 1 : 0) + " dB";

        case Format::integer:
            return juce::String (juce::roundToInt (v));

        case Format::plain:
        default:
            if (v == juce::roundToInt (v))
                return juce::String (juce::roundToInt (v));
            return juce::String (v, 2);
    }
}
} // namespace EffectCellFormat
