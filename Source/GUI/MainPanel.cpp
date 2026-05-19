#include "MainPanel.h"
#include "../PluginProcessor.h"
#include <memory>
#include "../State/StateSchema.h"
#include "LuxuryLookAndFeel.h"

MainPanel::MainPanel (AviatorKeyzProcessor& p)
    : processor (p)
    , luxuryLookAndFeel (std::make_unique<LuxuryLookAndFeel>())
    , presetBrowser (p, [this] { syncHeaderFromPresetManager(); })
    , waveformDisplay (p)
    , knobInputGain (p.getAPVTS(), AviatorKeyz::ParamID::INPUT_GAIN, "Input")
    , knobOutputGain (p.getAPVTS(), AviatorKeyz::ParamID::OUTPUT_GAIN, "Output")
    , knobGlide (p.getAPVTS(), AviatorKeyz::ParamID::GLIDE_TIME, "Glide")
    , knobSmear (p.getAPVTS(), AviatorKeyz::ParamID::SMEAR, "Smear")
    , knobTone (p.getAPVTS(), AviatorKeyz::ParamID::TONE, "Tone")
    , knobReverbAmt (p.getAPVTS(), AviatorKeyz::ParamID::REVERB_AMOUNT, "Room")
    , knobReverbSize (p.getAPVTS(), AviatorKeyz::ParamID::REVERB_SIZE, "Decay")
    , knobWidth (p.getAPVTS(), AviatorKeyz::ParamID::STEREO_WIDTH, "Width")
    , knobAttack (p.getAPVTS(), AviatorKeyz::ParamID::ENV_ATTACK, "Attack")
    , knobRelease (p.getAPVTS(), AviatorKeyz::ParamID::ENV_RELEASE, "Release")
    , knobPan (p.getAPVTS(), AviatorKeyz::ParamID::PAN, "Pan")
{
    setLookAndFeel (luxuryLookAndFeel.get());

    addAndMakeVisible (headerBar);
    addAndMakeVisible (presetBrowser);
    addAndMakeVisible (waveformDisplay);

    for (auto* k : { &knobInputGain,
                     &knobOutputGain,
                     &knobGlide,
                     &knobSmear,
                     &knobTone,
                     &knobReverbAmt,
                     &knobReverbSize,
                     &knobWidth,
                     &knobAttack,
                     &knobRelease,
                     &knobPan })
        addAndMakeVisible (*k);

    reverseButton.setButtonText ("Reverse");
    reverseButton.setClickingTogglesState (true);
    addAndMakeVisible (reverseButton);
    reverseAttachment =
        std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
            p.getAPVTS(), AviatorKeyz::ParamID::REVERSE, reverseButton);

    addAndMakeVisible (saveUserPresetButton);
    saveUserPresetButton.onClick = [this] {
        const auto alert = std::make_shared<juce::AlertWindow> (
            "Save user preset",
            "Saved under Documents/AviatorKeyz/Presets/<category>/",
            juce::AlertWindow::NoIcon);

        alert->addTextEditor ("nm", "My preset", "Preset name");
        alert->addButton ("OK", 1, juce::KeyPress (juce::KeyPress::returnKey));
        alert->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));

        alert->enterModalState (
            true,
            juce::ModalCallbackFunction::create ([this, alert] (int result) {
                if (result != 1)
                    return;

                const auto name = alert->getTextEditorContents ("nm").trim();

                if (name.isEmpty())
                    return;

                if (processor.getPresetManager().saveUserPreset (
                        presetBrowser.getSelectedCategory(), name))
                    presetBrowser.refreshPresetList();
            }));
    };

    syncHeaderFromPresetManager();
}

MainPanel::~MainPanel()
{
    setLookAndFeel (nullptr);
}

void MainPanel::syncHeaderFromPresetManager()
{
    auto& pm = processor.getPresetManager();
    headerBar.setPresetInfo (pm.getCurrentCategory(), pm.getCurrentPresetName());
}

void MainPanel::paint (juce::Graphics& g)
{
    g.fillAll (LuxuryLookAndFeel::backgroundColour());

    const auto bounds = getLocalBounds().toFloat();
    juce::ColourGradient vignette (juce::Colours::transparentBlack,
                                   bounds.getCentreX(),
                                   bounds.getCentreY(),
                                   juce::Colour (0x66000000),
                                   0.f,
                                   0.f,
                                   true);
    g.setGradientFill (vignette);
    g.fillRect (bounds);
}

void MainPanel::resized()
{
    auto r = getLocalBounds().reduced (10);
    headerBar.setBounds (r.removeFromTop (76));

    r.removeFromTop (8);
    auto body = r;

    auto leftCol = body.removeFromLeft (230);
    presetBrowser.setBounds (leftCol.removeFromTop (150));

    leftCol.removeFromTop (10);
    reverseButton.setBounds (leftCol.removeFromTop (28));
    leftCol.removeFromTop (8);
    saveUserPresetButton.setBounds (leftCol.removeFromTop (30));

    waveformDisplay.setBounds (body.removeFromTop (128));
    body.removeFromTop (10);

    auto knobArea = body;
    const int cols = 4;
    const int cellW = juce::jmax (72, knobArea.getWidth() / cols);
    const int cellH = 104;

    juce::Component* knobs[] = { &knobInputGain,
                                   &knobOutputGain,
                                   &knobGlide,
                                   &knobSmear,
                                   &knobTone,
                                   &knobReverbAmt,
                                   &knobReverbSize,
                                   &knobWidth,
                                   &knobAttack,
                                   &knobRelease,
                                   &knobPan };

    for (int i = 0; i < 11; ++i)
    {
        const int row = i / cols;
        const int col = i % cols;
        knobs[i]->setBounds (knobArea.getX() + col * cellW,
                             knobArea.getY() + row * cellH,
                             cellW,
                             cellH);
    }
}
