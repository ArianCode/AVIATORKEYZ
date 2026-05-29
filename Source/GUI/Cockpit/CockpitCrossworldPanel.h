#pragma once

#include "CockpitPhotoBackground.h"
#include "CockpitZones.h"
#include "../FooterBar.h"
#include "../PrecisionKnob.h"
#include "../ReverseToggle.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>
#include <memory>
#include <vector>

class AviatorKeyzProcessor;

/** Cockpit UI: untouched photo background + organized glass overlay panels. */
class CockpitCrossworldPanel : public juce::Component,
                               private juce::Timer
{
public:
    explicit CockpitCrossworldPanel (AviatorKeyzProcessor& processor);
    ~CockpitCrossworldPanel() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

    std::function<void()> onLibraryRequested;
    std::function<void()> onAboutRequested;

    void refreshPresetUI();

private:
    class GlassZone : public juce::Component
    {
    public:
        explicit GlassZone (AviatorCockpit::ZoneId id);
        void paint (juce::Graphics& g) override;
        AviatorCockpit::ZoneId zoneId;
        juce::String title;
    };

    class StatusStrip : public juce::Component
    {
    public:
        void paint (juce::Graphics& g) override;
        juce::Label readout { "status", "" };
    };

    std::unique_ptr<PrecisionKnob> makeKnob (const char* paramId,
                                             const juce::String& name,
                                             const juce::String& sublabel,
                                             PrecisionKnob::ValueFormat format);

    void buildPanelControls();
    void buildCategoryFilters();
    void layoutZones();
    void layoutLeftBrowser();
    void layoutCenterMacros();
    void layoutEnvelopePanel();
    void layoutModulationPanel();
    void layoutEffectsPanel();
    void layoutStatusStrip();
    void buildPresetList();
    void applyCategoryFilter();
    void updateCategoryButtonStates();
    PrecisionKnob::ValueFormat formatForParam (const char* paramId) const;
    bool categoryMatchesFilter (const juce::String& category) const;
    void timerCallback() override;

    AviatorKeyzProcessor& processor;
    CockpitPhotoBackground photoBackground;

    GlassZone leftBrowserPanel   { AviatorCockpit::ZoneId::leftMfd };
    GlassZone effectsPanel       { AviatorCockpit::ZoneId::rightMfd };
    GlassZone envelopePanel      { AviatorCockpit::ZoneId::radarAdsr };
    GlassZone modulationPanel    { AviatorCockpit::ZoneId::radarLfo };
    GlassZone centerMacroZone    { AviatorCockpit::ZoneId::throttleQuadrant };
    StatusStrip headerStatusStrip;

    FooterBar footer;

    juce::ListBox presetList;
    juce::TextEditor searchBox;
    juce::StringArray presetDisplayNames;
    juce::StringArray presetCategories;
    juce::Array<int> presetFlatIndices;
    juce::String categoryFilter;
    juce::Label selectedPresetLabel;

    std::vector<std::unique_ptr<juce::TextButton>> categoryButtons;

    std::vector<std::unique_ptr<PrecisionKnob>> macroKnobs;
    std::vector<std::unique_ptr<PrecisionKnob>> fxKnobs;
    std::vector<std::unique_ptr<PrecisionKnob>> envKnobs;
    std::vector<std::unique_ptr<PrecisionKnob>> modKnobs;
    std::unique_ptr<ReverseToggle> motionToggle;

    juce::Label syncReadout;
    juce::Label decayReadout;
    juce::Label sustainReadout;

    juce::Rectangle<int> photoArea;
    float lfoSweep = 0.f;

    struct PresetListModel;
    std::unique_ptr<PresetListModel> presetModel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CockpitCrossworldPanel)
};
