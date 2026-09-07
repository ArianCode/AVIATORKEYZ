#include "AdvancedPageContent.h"
#include "AdvancedWidgets.h"

namespace
{
namespace P = AviatorKeyz::ParamID;

const juce::StringArray kOnOff { "OFF", "ON" };
const juce::StringArray kLoopModes { "One Shot", "Loop", "Gate" };
const juce::StringArray kChopRates { "1/4", "1/8", "1/16", "1/32" };
const juce::StringArray kPerfModes {
    "Normal", "Chop", "Gate", "Stutter", "Half Time", "Reverse", "Scatter", "Freeze"
};
} // namespace

AdvancedPageContent::AdvancedPageContent (juce::AudioProcessorValueTreeState& apvts)
    : apvtsRef (apvts)
{
    macroStrip = std::make_unique<PerformanceMacroStrip> (apvts);
    addAndMakeVisible (*macroStrip);

    addCell (P::SRC_START, "START", "Sample window start.", Fmt::percent);
    addCell (P::SRC_END, "END", "Sample window end.", Fmt::percent);
    addCell (P::SRC_TUNE, "TUNE", "Transpose in semitones.", Fmt::semitones);
    addCell (P::SRC_SPEED, "SPEED", "Playback speed (affects pitch).", Fmt::plain);
    addChoice (P::SRC_REVERSE, "REVERSE", "Reverse playback.", kOnOff);
    addChoice (P::SRC_LOOP_MODE, "LOOP", "Loop mode.", kLoopModes);
    addChoice (P::SRC_BPM_SYNC, "BPM SYNC", "Varispeed sync to host tempo.", kOnOff);
    addChoice (P::SRC_KEYTRACK, "KEY TRACK", "Transpose sample pitch with MIDI notes.", kOnOff);
    addCell (P::SRC_ORIGINAL_BPM, "ORIG BPM", "Sample tempo for sync (auto from preset when possible).", Fmt::integer);

    addChoice (P::CHOP_ON, "CHOP", "Enable phrase chopper.", kOnOff);
    addCell (P::CHOP_AMOUNT, "CHOP", "Chop intensity.", Fmt::percent);
    addChoice (P::CHOP_RATE, "RATE", "Chop rate.", kChopRates);
    addCell (P::CHOP_GATE, "GATE", "Gate amount.", Fmt::percent);
    addCell (P::CHOP_SWING, "SWING", "Swing amount.", Fmt::percent);
    addCell (P::CHOP_RANDOM, "RANDOM", "Random step jumps.", Fmt::percent);
    addCell (P::CHOP_REVERSE_CHANCE, "REV %", "Reverse chance.", Fmt::percent);
    addCell (P::CHOP_SMOOTH, "SMOOTH", "Crossfade smoothing.", Fmt::seconds);

    addChoice (P::PTEX_ON, "TEXTURE", "Texture layer on.", kOnOff);
    addChoice (P::PTEX_FREEZE, "FREEZE", "Freeze texture buffer.", kOnOff);
    addCell (P::PTEX_GRAIN_SIZE, "GRAIN", "Grain size.", Fmt::percent);
    addCell (P::PTEX_DENSITY, "DENSITY", "Grain density.", Fmt::percent);
    addCell (P::PTEX_POSITION, "POSITION", "Read position.", Fmt::percent);
    addCell (P::PTEX_PITCH_SPREAD, "SPREAD", "Pitch spread.", Fmt::percent);
    addCell (P::PTEX_SMEAR, "SMEAR", "Texture smear.", Fmt::percent);
    addCell (P::PTEX_WIDTH, "WIDTH", "Stereo width.", Fmt::percent);
    addCell (P::PTEX_MIX, "MIX", "Texture blend.", Fmt::percent);
    addChoice (P::PERF_MODE, "MODE", "Performance mode.", kPerfModes);

    addChoice (P::PERF_FX_STUTTER, "STUTTER", "Stutter FX.", kOnOff);
    addChoice (P::PERF_FX_REVERSE, "REV FX", "Reverse FX.", kOnOff);
    addChoice (P::PERF_FX_HALF_TIME, "HALF", "Half-time FX.", kOnOff);
    addChoice (P::PERF_FX_FREEZE, "FRZ FX", "Freeze FX.", kOnOff);

    addChoice (P::FX_REVERB_ON, "REVERB", "Reverb on.", kOnOff);
    addCell (P::REVERB_AMOUNT, "REV MIX", "Reverb amount.", Fmt::percent);
    addChoice (P::FX_DELAY_ON, "DELAY", "Delay on.", kOnOff);
    addCell (P::FX_DELAY_MIX, "DLY MIX", "Delay mix.", Fmt::percent);

    for (auto* tab : { &tabSource, &tabTexture, &tabMacros })
    {
        tab->setClickingTogglesState (true);
        tab->setRadioGroupId (94001);
        addAndMakeVisible (tab);
    }
    tabSource.onClick = [this] { setSection (Section::sourceChop); };
    tabTexture.onClick = [this] { setSection (Section::textureMotion); };
    tabMacros.onClick = [this] { setSection (Section::macrosSpace); };

    setSection (Section::sourceChop);
}

void AdvancedPageContent::addCell (const char* id, const juce::String& title,
                                   const juce::String& tooltip, Fmt fmt)
{
    auto c = std::make_unique<EffectCell> (apvtsRef, id, title, "^v DRAG", tooltip, fmt);
    cellMap[id] = c.get();
    addAndMakeVisible (*c);
    cells.push_back (std::move (c));
}

void AdvancedPageContent::addChoice (const char* id, const juce::String& title,
                                      const juce::String& tooltip, juce::StringArray choiceList)
{
    auto c = std::make_unique<EffectCellChoice> (apvtsRef, id, title, tooltip, std::move (choiceList));
    cellMap[id] = c.get();
    addAndMakeVisible (*c);
    choiceCells.push_back (std::move (c));
}

void AdvancedPageContent::setMacroLabels (const std::array<juce::String, 4>& labels)
{
    if (macroStrip != nullptr)
        macroStrip->setMacroLabels (labels);
}

void AdvancedPageContent::setSection (Section section)
{
    currentSection = section;
    styleSectionTab (tabSource, section == Section::sourceChop);
    styleSectionTab (tabTexture, section == Section::textureMotion);
    styleSectionTab (tabMacros, section == Section::macrosSpace);
    resized();
}

void AdvancedPageContent::styleSectionTab (juce::TextButton& btn, bool active) const
{
    btn.setToggleState (active, juce::dontSendNotification);
    btn.setColour (juce::TextButton::buttonColourId,
                   active ? AdvancedWidgets::kGold().withAlpha (0.25f)
                          : juce::Colours::transparentBlack);
    btn.setColour (juce::TextButton::textColourOffId, AdvancedWidgets::kGold());
}

void AdvancedPageContent::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff0a1424));
    g.setColour (AdvancedWidgets::kGold().withAlpha (0.35f));
    g.drawRect (getLocalBounds().reduced (2), 1);
    g.setColour (AdvancedWidgets::kGold());
    g.setFont (AviatorTokens::hudBold (14.f));
    g.drawText ("PERFORMANCE / TEXTURE ENGINE", getLocalBounds().removeFromTop (28),
                juce::Justification::centred);
}

void AdvancedPageContent::layoutSection (juce::Rectangle<int> area)
{
    const int tileW = 100;
    const int tileH = 72;
    const int gap = 8;
    int x = area.getX();
    int y = area.getY();
    int col = 0;

    auto place = [&] (juce::Component& c)
    {
        c.setVisible (true);
        c.setBounds (x + col * (tileW + gap), y, tileW, tileH);
        if (++col >= 8)
        {
            col = 0;
            y += tileH + gap;
        }
    };

    auto hideAll = [&]
    {
        for (auto& c : cells) c->setVisible (false);
        for (auto& c : choiceCells) c->setVisible (false);
        macroStrip->setVisible (false);
    };

    hideAll();

    auto findCell = [&] (const char* id) -> juce::Component*
    {
        const auto it = cellMap.find (id);
        return it != cellMap.end() ? it->second : nullptr;
    };

    auto show = [&] (const char* id)
    {
        if (auto* c = findCell (id))
            place (*c);
    };

    switch (currentSection)
    {
        case Section::sourceChop:
            for (const char* id : { P::SRC_START, P::SRC_END, P::SRC_TUNE, P::SRC_SPEED,
                                    P::SRC_REVERSE, P::SRC_LOOP_MODE, P::SRC_BPM_SYNC, P::SRC_KEYTRACK,
                                    P::SRC_ORIGINAL_BPM,
                                    P::CHOP_ON, P::CHOP_AMOUNT, P::CHOP_RATE, P::CHOP_GATE,
                                    P::CHOP_SWING, P::CHOP_RANDOM, P::CHOP_REVERSE_CHANCE, P::CHOP_SMOOTH })
                show (id);
            break;
        case Section::textureMotion:
            for (const char* id : { P::PTEX_ON, P::PTEX_FREEZE, P::PTEX_GRAIN_SIZE, P::PTEX_DENSITY,
                                    P::PTEX_POSITION, P::PTEX_PITCH_SPREAD, P::PTEX_SMEAR,
                                    P::PTEX_WIDTH, P::PTEX_MIX, P::PERF_MODE,
                                    P::PERF_FX_STUTTER, P::PERF_FX_REVERSE, P::PERF_FX_HALF_TIME, P::PERF_FX_FREEZE })
                show (id);
            break;
        case Section::macrosSpace:
        {
            macroStrip->setVisible (true);
            auto macroArea = area.removeFromTop (120);
            macroStrip->setBounds (macroArea);
            y = area.getY();
            x = area.getX();
            col = 0;
            for (const char* id : { P::FX_REVERB_ON, P::REVERB_AMOUNT, P::FX_DELAY_ON, P::FX_DELAY_MIX })
                show (id);
            break;
        }
    }
}

void AdvancedPageContent::resized()
{
    auto bounds = getLocalBounds().reduced (12);
    bounds.removeFromTop (28);
    auto tabs = bounds.removeFromBottom (32);
    const int tabW = tabs.getWidth() / 3;
    tabSource.setBounds (tabs.removeFromLeft (tabW).reduced (2));
    tabTexture.setBounds (tabs.removeFromLeft (tabW).reduced (2));
    tabMacros.setBounds (tabs.reduced (2));
    layoutSection (bounds);
}
