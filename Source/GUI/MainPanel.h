#pragma once

#include "DesignTokens.h"
#include "PluginShell.h"
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

    static int getDesignWidth()  { return DesignTokens::kDesignWidth; }
    static int getDesignHeight() { return DesignTokens::kDesignHeight; }

private:
    void refreshPresetUI();
    void navigatePreset (int delta);
    void selectCategory (const juce::String& category);
    void showLibraryPopup();
    PresetDisplayInfo makeDisplayInfo() const;

    AviatorKeyzProcessor& processor;

    std::unique_ptr<juce::LookAndFeel> luxuryLookAndFeel;
    PluginShell                          pluginShell;

    juce::Component::SafePointer<juce::CallOutBox> libraryCallout;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainPanel)
};
