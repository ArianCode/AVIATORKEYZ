#pragma once

#include <juce_core/juce_core.h>

namespace PresetDisplayUtils
{
inline juce::String shortenDisplayName (const juce::String& rawName)
{
    if (rawName.isEmpty())
        return {};

    auto parts = juce::StringArray::fromTokens (rawName, "_", "");
    if (parts.isEmpty())
        return rawName;

    int start = 0;
    while (start < parts.size() - 1)
    {
        const auto token = parts[start].toUpperCase();
        if (token.length() <= 6 && token == parts[start].toUpperCase())
        {
            ++start;
            continue;
        }
        break;
    }

    static const juce::StringArray genericSkip {
        "SYNTH", "LEAD", "ONE", "SHOT", "KEYS", "KEY", "FX", "OS", "LP", "DRY", "WET",
        "SFX", "SN", "SS", "SO", "RKU", "KMRBI", "DS", "BOS", "PJ", "MM", "TFH"
    };

    while (start < parts.size() - 1 && genericSkip.contains (parts[start].toUpperCase()))
        ++start;

    if (start >= parts.size())
        start = juce::jmax (0, parts.size() - 3);

    juce::String result;
    for (int i = start; i < parts.size(); ++i)
    {
        auto word = parts[i].toLowerCase();
        if (word.isEmpty())
            continue;

        word = word.substring (0, 1).toUpperCase() + word.substring (1);
        if (result.isNotEmpty())
            result += " ";
        result += word;
    }

    return result.isNotEmpty() ? result : rawName;
}

inline juce::String ellipsize (const juce::String& text, const juce::Font& font, int maxWidthPx)
{
    if (maxWidthPx <= 0 || text.isEmpty())
        return text;

    if (font.getStringWidth (text) <= maxWidthPx)
        return text;

    for (int len = text.length() - 1; len > 0; --len)
    {
        const auto trial = text.substring (0, len).trimEnd() + "...";
        if (font.getStringWidth (trial) <= maxWidthPx)
            return trial;
    }

    return "...";
}

} // namespace PresetDisplayUtils
