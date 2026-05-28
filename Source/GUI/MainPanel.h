#pragma once

#include "AviatorTokens.h"
#include "Cockpit/CockpitCrossworldPanel.h"
#include "PresetBrowser.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>

class AviatorKeyzProcessor;

class MainPanel : public juce::Component
{
public:
    explicit MainPanel (AviatorKeyzProcessor& processor);
    ~MainPanel() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

    static int getDesignWidth()  { return AviatorTokens::kDesignWidth; }
    static int getDesignHeight() { return AviatorTokens::kDesignHeight + AviatorTokens::kFooterH; }

private:
    void refreshPresetUI();
    void showLibraryPopup();

    AviatorKeyzProcessor& processor;
    std::unique_ptr<juce::LookAndFeel> lookAndFeel;
    CockpitCrossworldPanel cockpit;
    juce::Component::SafePointer<juce::CallOutBox> libraryCallout;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainPanel)
};
