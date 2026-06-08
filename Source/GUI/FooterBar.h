#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class FooterBar : public juce::Component
{
public:
    FooterBar();

    void setSampleRate (double sampleRate);
    void setBlockSize (int blockSize);
    void setActiveStatus (const juce::String& status);
    void setHostDescription (const juce::String& host);

    void paint (juce::Graphics& g) override;
    void resized() override;

    std::function<void()> onSettingsClicked;
    std::function<void()> onAboutClicked;

private:
    void refreshMetaLabel();

    juce::Label statusLabel;
    juce::Label metaLabel;
    juce::Label hostLabel;
    juce::TextButton settingsButton { "Presets" };
    juce::TextButton aboutButton { "About" };
    juce::Label versionLabel;

    double lastSampleRate = 44100.0;
    int    blockSize      = 256;
    juce::String hostDescription;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FooterBar)
};
