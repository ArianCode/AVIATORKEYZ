#include "MainPanel.h"
#include "../PluginProcessor.h"
#include "LuxuryLookAndFeel.h"

MainPanel::MainPanel (AviatorKeyzProcessor& p)
    : processor (p),
      cockpit (p)
{
    lookAndFeel = std::make_unique<LuxuryLookAndFeel>();
    setLookAndFeel (lookAndFeel.get());

    addAndMakeVisible (cockpit);

    auto& pm = processor.getPresetManager();
    previousPresetLoadedHandler = pm.onPresetLoaded;
    pm.onPresetLoaded = [this, loadSampleHook = previousPresetLoadedHandler] (const juce::String& category,
                                                                              const juce::String& name,
                                                                              const juce::String& sampleId,
                                                                              int rootNote)
    {
        // Break out of ListBox/mouse stack before suspendProcessing and UI refresh.
        juce::Timer::callAfterDelay (0, [safe = juce::Component::SafePointer<MainPanel> (this),
                                         loadSampleHook,
                                         category,
                                         name,
                                         sampleId,
                                         rootNote]
        {
            if (safe == nullptr)
                return;

            if (loadSampleHook)
                loadSampleHook (category, name, sampleId, rootNote);

            safe->cockpit.requestPresetUiRefresh();
        });
    };
}

MainPanel::~MainPanel()
{
    auto& pm = processor.getPresetManager();
    if (pm.onPresetLoaded)
        pm.onPresetLoaded = previousPresetLoadedHandler;

    setLookAndFeel (nullptr);
}

void MainPanel::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff030309));
}

void MainPanel::resized()
{
    cockpit.setBounds (getLocalBounds());
}
