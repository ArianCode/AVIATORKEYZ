#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class FooterBar : public juce::Component
{
public:
    FooterBar();

    void setSampleRate (double sampleRate);
    void setBlockSize (int blockSize);
    void setHudText (const juce::String& hud);

    void paint (juce::Graphics& g) override;
    void resized() override;

    std::function<void()> onSettingsClicked;
    std::function<void()> onAboutClicked;

private:
    void refreshMetaLabel();

    juce::Label statusLabel;
    juce::Label metaLabel;
    juce::Label hudLabel;
    juce::TextButton settingsButton { "Settings" };
    juce::TextButton aboutButton { "About" };
    juce::Label versionLabel;

    double lastSampleRate = 44100.0;
    int    blockSize      = 256;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FooterBar)
};
