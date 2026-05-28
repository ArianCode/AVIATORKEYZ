#pragma once

#include "CockpitPhotoBackground.h"
#include "CockpitZones.h"
#include "../FooterBar.h"
#include "../PrecisionKnob.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>
#include <memory>
#include <vector>

class AviatorKeyzProcessor;
class PhotoAnchoredKnob;
class ReverseToggle;

/** Photo-anchored crossworld UI: photo + glass zones + physical anchor controls. */
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

    void layoutZones();
    void layoutPhysicalControls();
    void buildPresetList();
    void handleAnchorAction (const char* action);
    PrecisionKnob::ValueFormat formatForParam (const char* paramId) const;
    void paintAdsrCurve (juce::Graphics& g, juce::Rectangle<int> bounds);
    void paintLfoRadar (juce::Graphics& g, juce::Rectangle<int> bounds);
    void paintFxMeters (juce::Graphics& g, juce::Rectangle<int> bounds);
    void timerCallback() override;

    AviatorKeyzProcessor& processor;
    CockpitPhotoBackground photoBackground;

    GlassZone leftMfd { AviatorCockpit::ZoneId::leftMfd };
    GlassZone rightMfd { AviatorCockpit::ZoneId::rightMfd };
    GlassZone radarAdsr { AviatorCockpit::ZoneId::radarAdsr };
    GlassZone radarLfo { AviatorCockpit::ZoneId::radarLfo };
    GlassZone autopilotStrip { AviatorCockpit::ZoneId::autopilotStrip };
    GlassZone overheadZone { AviatorCockpit::ZoneId::overhead };
    GlassZone throttleZone { AviatorCockpit::ZoneId::throttleQuadrant };

    FooterBar footer;

    juce::ListBox presetList;
    juce::TextEditor searchBox;
    juce::StringArray presetDisplayNames;
    juce::Label engineReadout;
    juce::Label stripReadout;

    std::vector<std::unique_ptr<PhotoAnchoredKnob>> photoKnobs;
    std::vector<std::unique_ptr<ReverseToggle>> photoToggles;
    std::vector<std::unique_ptr<juce::TextButton>> photoButtons;
    std::vector<std::unique_ptr<juce::Slider>> photoLevers;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> leverAttachments;
    std::vector<juce::Component*> anchoredWidgets;
    std::vector<int> anchoredIndices;

    juce::Rectangle<int> photoArea;
    float lfoSweep = 0.f;

    struct PresetListModel;
    std::unique_ptr<PresetListModel> presetModel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CockpitCrossworldPanel)
};
