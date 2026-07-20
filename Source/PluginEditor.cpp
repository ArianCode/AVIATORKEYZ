#include "PluginEditor.h"
#include "GUI/Jetsonic/JetsonicTheme.h"
#include "DSP/Performance/PerformanceTypes.h"

namespace
{
constexpr float kAspect = (float) Jetsonic::kDesignW / (float) Jetsonic::kDesignH;
} // namespace

AviatorKeyzEditor::AviatorKeyzEditor (AviatorKeyzProcessor& p)
    : AudioProcessorEditor (&p)
    , processorRef (p)
{
    setOpaque (true);

    // Proportional scaling only: the whole interface keeps the reference
    // composition at every size.
    setResizable (true, true);
    if (auto* constrainer = getConstrainer())
    {
        constrainer->setFixedAspectRatio (kAspect);
        constrainer->setSizeLimits (kMinWidth, juce::roundToInt ((float) kMinWidth / kAspect),
                                    kMaxWidth, juce::roundToInt ((float) kMaxWidth / kAspect));
    }
    setSize (Jetsonic::kDesignW, Jetsonic::kDesignH);

    mainView = std::make_unique<JetsonicMainView> (p);
    mainView->onModeChanged = [this] (bool performance) { setPerformanceView (performance); };
    addAndMakeVisible (*mainView);

    advancedPanel = std::make_unique<AdvancedPanel> (p);
    advancedPanel->setVisible (false);
    addAndMakeVisible (*advancedPanel);

    auto& presetManager = p.getPresetManager();
    previousPresetLoadedHandler = presetManager.onPresetLoaded;
    presetManager.onPresetLoaded = [this, previousPresetLoaded = previousPresetLoadedHandler] (const juce::String& category,
                                                                                              const juce::String& name,
                                                                                              const juce::String& sampleId,
                                                                                              int rootNote) {
        if (previousPresetLoaded)
            previousPresetLoaded (category, name, sampleId, rootNote);

        if (advancedPanel != nullptr)
            advancedPanel->refreshPresetUI();
    };

    previousMacroMapsLoadedHandler = presetManager.onMacroMapsLoaded;
    presetManager.onMacroMapsLoaded = [this, previousMacroMapsLoaded = previousMacroMapsLoadedHandler] (const std::array<MacroControl, 4>& macros) {
        if (previousMacroMapsLoaded)
            previousMacroMapsLoaded (macros);

        if (advancedPanel != nullptr)
            advancedPanel->refreshPresetUI();
    };

    advancedPanel->refreshPresetUI();
    setPerformanceView (false);

    layoutContent();
    repaint();

#if JUCE_DEBUG
    // UI validation harness: AVIATORKEYZ_SNAPSHOT_PATH=<file.png> renders the
    // main view at the canonical 1647x955 design size and quits (debug only).
    const auto snapshotPath = juce::SystemStats::getEnvironmentVariable ("AVIATORKEYZ_SNAPSHOT_PATH", {});
    if (snapshotPath.isNotEmpty())
    {
        juce::Timer::callAfterDelay (1000,
            [safe = juce::Component::SafePointer<AviatorKeyzEditor> (this), snapshotPath]
            {
                if (safe == nullptr || safe->mainView == nullptr)
                    return;

                auto image = safe->mainView->createComponentSnapshot (
                    safe->mainView->getLocalBounds(), false, 1.0f);

                juce::File file (snapshotPath);
                file.deleteFile();
                juce::FileOutputStream stream (file);
                if (stream.openedOk())
                {
                    juce::PNGImageFormat png;
                    png.writeImageToStream (image, stream);
                }

                if (auto* app = juce::JUCEApplication::getInstance())
                    app->systemRequestedQuit();
            });
    }
#endif
}

AviatorKeyzEditor::~AviatorKeyzEditor()
{
    auto& presetManager = processorRef.getPresetManager();
    presetManager.onPresetLoaded = previousPresetLoadedHandler;
    presetManager.onMacroMapsLoaded = previousMacroMapsLoadedHandler;
}

void AviatorKeyzEditor::setPerformanceView (bool performance)
{
    performanceView = performance;
    mainView->getTopHeader().setPerformanceSelected (performance);
    if (advancedPanel != nullptr)
        advancedPanel->setVisible (performance);
    layoutContent();
}

void AviatorKeyzEditor::layoutContent()
{
    if (mainView == nullptr || getWidth() <= 0 || getHeight() <= 0)
        return;

    // The main view lives in the fixed design space; scale it as one unit.
    const float scale = (float) getWidth() / (float) Jetsonic::kDesignW;
    mainView->setTransform (juce::AffineTransform::scale (scale));
    mainView->setBounds (0, 0, Jetsonic::kDesignW, Jetsonic::kDesignH);

    // PERFORMANCE panel replaces everything below the top header.
    if (advancedPanel != nullptr && advancedPanel->isVisible())
    {
        const int headerBottom = juce::roundToInt ((float) (Jetsonic::topHeaderBounds().getBottom() + 2) * scale);
        advancedPanel->setBounds (0, headerBottom, getWidth(), getHeight() - headerBottom);
        advancedPanel->toFront (false);
    }
}

void AviatorKeyzEditor::paint (juce::Graphics& g)
{
    g.fillAll (Jetsonic::bgBlack());
}

void AviatorKeyzEditor::resized()
{
    layoutContent();
}

void AviatorKeyzEditor::visibilityChanged()
{
    if (isShowing())
        layoutContent();
}
