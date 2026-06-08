#include "MainPanel.h"
#include "../PluginProcessor.h"
#include "LuxuryLookAndFeel.h"

MainPanel::MainPanel (AviatorKeyzProcessor& p)
    : processor (p)
    , lookAndFeel (std::make_unique<LuxuryLookAndFeel>())
    , cockpit (p)
{
    setOpaque (true);
    setLookAndFeel (lookAndFeel.get());
    addAndMakeVisible (cockpit);

    cockpit.onLibraryRequested = [this] { showLibraryPopup(); };
    cockpit.onAboutRequested = [] {
        juce::AlertWindow::showMessageBoxAsync (
            juce::AlertWindow::InfoIcon,
            "AviatorKeyz",
            "Photo-anchored cockpit UI.\nAviatorKeyz v1.0.0");
    };

    auto& pm = p.getPresetManager();
    auto loadSampleHook = pm.onPresetLoaded;
    pm.onPresetLoaded = [this, loadSampleHook] (const juce::String& category,
                                                const juce::String& name,
                                                const juce::String& sampleId,
                                                int rootNote) {
        if (loadSampleHook)
            loadSampleHook (category, name, sampleId, rootNote);
        refreshPresetUI();
    };

    refreshPresetUI();
}

MainPanel::~MainPanel()
{
    setLookAndFeel (nullptr);
}

void MainPanel::refreshPresetUI()
{
    cockpit.refreshPresetUI();
}

void MainPanel::showLibraryPopup()
{
    if (libraryCallout != nullptr)
    {
        libraryCallout->dismiss();
        return;
    }

    auto browser = std::make_unique<PresetBrowser> (processor, [this] {
        refreshPresetUI();
        if (libraryCallout != nullptr)
            libraryCallout->dismiss();
    });
    browser->setSize (320, 200);

    auto popupArea = juce::Rectangle<int> (320, 200).withCentre (getScreenBounds().getCentre());
    libraryCallout = &juce::CallOutBox::launchAsynchronously (
        std::move (browser), popupArea, nullptr);
}

void MainPanel::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff030309));
}

void MainPanel::resized()
{
    cockpit.setBounds (getLocalBounds());
}
