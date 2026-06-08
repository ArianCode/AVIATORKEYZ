#pragma once

#include "../../DSP/ModMatrix.h"
#include "../../State/StateSchema.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <optional>

/** Maps APVTS param IDs ↔ mod-matrix rows and destinations. */
class ModRoutingHub
{
public:
    struct RowIds
    {
        const char* on;
        const char* source;
        const char* dest;
        const char* amount;
    };

    static constexpr int kNumRows = AviatorKeyz::ParamID::MOD_MATRIX_ROWS;

    static constexpr RowIds kRows[kNumRows] {
        { AviatorKeyz::ParamID::MOD0_ON, AviatorKeyz::ParamID::MOD0_SOURCE, AviatorKeyz::ParamID::MOD0_DEST, AviatorKeyz::ParamID::MOD0_AMOUNT },
        { AviatorKeyz::ParamID::MOD1_ON, AviatorKeyz::ParamID::MOD1_SOURCE, AviatorKeyz::ParamID::MOD1_DEST, AviatorKeyz::ParamID::MOD1_AMOUNT },
        { AviatorKeyz::ParamID::MOD2_ON, AviatorKeyz::ParamID::MOD2_SOURCE, AviatorKeyz::ParamID::MOD2_DEST, AviatorKeyz::ParamID::MOD2_AMOUNT },
        { AviatorKeyz::ParamID::MOD3_ON, AviatorKeyz::ParamID::MOD3_SOURCE, AviatorKeyz::ParamID::MOD3_DEST, AviatorKeyz::ParamID::MOD3_AMOUNT },
        { AviatorKeyz::ParamID::MOD4_ON, AviatorKeyz::ParamID::MOD4_SOURCE, AviatorKeyz::ParamID::MOD4_DEST, AviatorKeyz::ParamID::MOD4_AMOUNT },
        { AviatorKeyz::ParamID::MOD5_ON, AviatorKeyz::ParamID::MOD5_SOURCE, AviatorKeyz::ParamID::MOD5_DEST, AviatorKeyz::ParamID::MOD5_AMOUNT },
        { AviatorKeyz::ParamID::MOD6_ON, AviatorKeyz::ParamID::MOD6_SOURCE, AviatorKeyz::ParamID::MOD6_DEST, AviatorKeyz::ParamID::MOD6_AMOUNT },
        { AviatorKeyz::ParamID::MOD7_ON, AviatorKeyz::ParamID::MOD7_SOURCE, AviatorKeyz::ParamID::MOD7_DEST, AviatorKeyz::ParamID::MOD7_AMOUNT },
    };

    static std::optional<ModDest> modDestForParam (const juce::String& paramId);
    static juce::String paramIdForModDest (ModDest dest);

    static std::optional<int> rowForDest (const juce::AudioProcessorValueTreeState& apvts, ModDest dest);
    static std::optional<int> rowForParam (const juce::AudioProcessorValueTreeState& apvts, const juce::String& paramId);

    static void assignRow (juce::AudioProcessorValueTreeState& apvts,
                           int row,
                           int sourceIndex,
                           ModDest dest,
                           float amount,
                           bool enabled = true);

    static void clearRow (juce::AudioProcessorValueTreeState& apvts, int row);

    static juce::Colour colourForSource (int sourceIndex);

    struct KnobModState
    {
        bool active { false };
        int row { -1 };
        int sourceIndex { 0 };
        float amount { 0.f };
        juce::Colour colour;
    };

    static KnobModState stateForParam (const juce::AudioProcessorValueTreeState& apvts,
                                       const juce::String& paramId);
};
