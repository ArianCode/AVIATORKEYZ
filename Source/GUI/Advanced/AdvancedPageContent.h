#pragma once

#include "EffectCell.h"
#include "EffectCellChoice.h"
#include "EffectParamBox.h"
#include "PerformanceMacroStrip.h"
#include "../../State/StateSchema.h"
#include "../AviatorTokens.h"
#include <array>
#include <memory>
#include <unordered_map>
#include <vector>

/** PERFORMANCE / TEXTURE ENGINE tab — sample chop, motion, texture, macros. */
class AdvancedPageContent : public juce::Component
{
public:
    static constexpr int kDesignWidth  = 1366;
    static constexpr int kDesignHeight = 860;

    explicit AdvancedPageContent (juce::AudioProcessorValueTreeState& apvts);

    void paint (juce::Graphics& g) override;
    void resized() override;

    void setMacroLabels (const std::array<juce::String, 4>& labels);

private:
    using Fmt = EffectCell::Format;

    enum class Section { sourceChop, textureMotion, macrosSpace };

    void addCell (const char* id, const juce::String& title, const juce::String& tooltip, Fmt fmt);
    void addChoice (const char* id, const juce::String& title, const juce::String& tooltip,
                    juce::StringArray choiceList = {});
    void setSection (Section section);
    void styleSectionTab (juce::TextButton& btn, bool active) const;
    void layoutSection (juce::Rectangle<int> area);

    juce::AudioProcessorValueTreeState& apvtsRef;
    std::vector<std::unique_ptr<EffectCell>> cells;
    std::vector<std::unique_ptr<EffectCellChoice>> choiceCells;
    std::unordered_map<std::string, juce::Component*> cellMap;
    std::unique_ptr<PerformanceMacroStrip> macroStrip;

    juce::TextButton tabSource { "SOURCE + CHOP" };
    juce::TextButton tabTexture { "TEXTURE + MOTION" };
    juce::TextButton tabMacros { "MACROS + SPACE" };
    Section currentSection { Section::sourceChop };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AdvancedPageContent)
};
