#include "MainPanel.h"
#include "../PluginProcessor.h"
#include "LuxuryLookAndFeel.h"

namespace
{
    juce::String makeHudAlt (const juce::String& seed)
    {
        const int fl = 80 + (int) std::abs (seed.hashCode()) % 340;
        return "FL " + juce::String (fl);
    }

    juce::String makeHudHdg (const juce::String& seed)
    {
        const int hdg = (int) std::abs (seed.hashCode() >> 4) % 360;
        return "HDG " + juce::String (hdg) + juce::String::fromUTF8 ("\xc2\xb0");
    }

    juce::String makeHudRoute (const juce::String& seed)
    {
        static const char* airports[] = { "KLAS", "KJFK", "KSFO", "KEWR", "KMIA", "KLAX", "KORD", "KSEA" };
        const int a = std::abs (seed.hashCode()) % 8;
        const int b = std::abs (seed.hashCode() >> 3) % 8;
        return juce::String (airports[a]) + " → " + airports[b];
    }
}

MainPanel::MainPanel (AviatorKeyzProcessor& p)
    : processor (p)
    , luxuryLookAndFeel (std::make_unique<LuxuryLookAndFeel>())
    , pluginShell (p)
{
    setOpaque (true);
    setLookAndFeel (luxuryLookAndFeel.get());

    addAndMakeVisible (pluginShell);

    pluginShell.getCategories().setCategories (p.getPresetManager().getAllCategories());
    pluginShell.getCategories().onCategorySelected =
        [this] (const juce::String& cat) { selectCategory (cat); };

    auto& header = pluginShell.getHeader();
    header.onPreviousPreset = [this] { navigatePreset (-1); };
    header.onNextPreset     = [this] { navigatePreset (+1); };
    header.onLibraryClicked = [this] { showLibraryPopup(); };

    auto& artwork = pluginShell.getArtwork();
    artwork.onPreviousPreset = [this] { navigatePreset (-1); };
    artwork.onNextPreset     = [this] { navigatePreset (+1); };

    auto& footer = pluginShell.getFooter();
    footer.onAboutClicked = [] {
        juce::AlertWindow::showMessageBoxAsync (
            juce::AlertWindow::InfoIcon,
            "AviatorKeyz",
            "Premium sample instrument.\nAviatorKeyz v1.0.0");
    };
    footer.onSettingsClicked = [this] { showLibraryPopup(); };

    p.getPresetManager().onPresetLoaded = [this] (const juce::String&, const juce::String&, const juce::String&) {
        refreshPresetUI();
    };

    refreshPresetUI();
    resized();
    repaint();
}

MainPanel::~MainPanel()
{
    setLookAndFeel (nullptr);
}

PresetDisplayInfo MainPanel::makeDisplayInfo() const
{
    auto& pm = processor.getPresetManager();
    const auto category = pm.getCurrentCategory();
    const auto name     = pm.getCurrentPresetName();

    PresetDisplayInfo info;
    info.categoryTag = category + " · Factory";
    info.heroName    = name;
    info.subtitle    = "AviatorKeyz · " + category;
    info.hudAlt      = makeHudAlt (name);
    info.hudHdg      = makeHudHdg (name);
    info.hudRoute    = makeHudRoute (name);
    return info;
}

void MainPanel::refreshPresetUI()
{
    auto& pm = processor.getPresetManager();
    const auto info = makeDisplayInfo();

    pluginShell.getHeader().setPresetDisplayName (pm.getCurrentPresetName());
    pluginShell.getHeader().setPresetCount (pm.getTotalPresetCount());
    pluginShell.getCategories().setActiveCategory (pm.getCurrentCategory());
    pluginShell.getArtwork().setPresetInfo (info);
    pluginShell.getFooter().setHudText (info.hudAlt + " · " + info.hudHdg);
    pluginShell.getFooter().setSampleRate (processor.getSampleRate() > 0.0 ? processor.getSampleRate() : 44100.0);
    pluginShell.getFooter().setBlockSize (processor.getBlockSize() > 0 ? processor.getBlockSize() : 256);
}

void MainPanel::navigatePreset (int delta)
{
    if (processor.getPresetManager().loadAdjacentPreset (delta))
    {
        pluginShell.getArtwork().triggerPresetFlash();
        refreshPresetUI();
    }
}

void MainPanel::selectCategory (const juce::String& category)
{
    auto& pm = processor.getPresetManager();

    if (category.equalsIgnoreCase ("All"))
        return;

    const auto presets = pm.getPresetsForCategory (category);
    if (presets.isEmpty())
        return;

    if (pm.loadPreset (category, presets[0]))
    {
        pluginShell.getArtwork().triggerPresetFlash();
        refreshPresetUI();
    }
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
    browser->setSize (280, 150);

    libraryCallout = &juce::CallOutBox::launchAsynchronously (
        std::move (browser),
        pluginShell.getHeader().getScreenBounds().removeFromRight (300).withHeight (160),
        nullptr);
}

void MainPanel::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff030309));
}

void MainPanel::resized()
{
    auto bounds = getLocalBounds();

    if (bounds.getWidth() < 4 || bounds.getHeight() < 4)
        return;

    const float scale = juce::jmin (bounds.getWidth()  / (float) DesignTokens::kDesignWidth,
                                    bounds.getHeight() / (float) DesignTokens::kDesignHeight);

    const int scaledW = juce::roundToInt ((float) DesignTokens::kDesignWidth  * scale);
    const int scaledH = juce::roundToInt ((float) DesignTokens::kDesignHeight * scale);
    const int offsetX = (bounds.getWidth()  - scaledW) / 2;
    const int offsetY = (bounds.getHeight() - scaledH) / 2;

    pluginShell.setBounds (offsetX, offsetY, scaledW, scaledH);
    pluginShell.setTransform (juce::AffineTransform());
    pluginShell.resized();
}
