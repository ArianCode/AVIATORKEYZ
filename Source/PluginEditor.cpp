#include "PluginEditor.h"
#include "GUI/Aviation/AviationTheme.h"
#include "DSP/Performance/PerformanceTypes.h"

namespace
{
constexpr float kAspect = (float) Aviation::kDesignW / (float) Aviation::kDesignH;
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
    setSize (kDefaultWidth, juce::roundToInt ((float) kDefaultWidth / kAspect));

    // Design canvas: pages first (bottom), then the shared header on top. The
    // pages are opaque and cover the canvas, so it needs no paint of its own.
    canvas.setBounds (0, 0, Aviation::kDesignW, Aviation::kDesignH);
    addAndMakeVisible (canvas);

    mainView = std::make_unique<AviationMainView> (p);
    mainView->setBounds (canvas.getLocalBounds());
    canvas.addAndMakeVisible (*mainView);

    flightDeck = std::make_unique<FlightDeckView> (p);
    flightDeck->setBounds (canvas.getLocalBounds());
    canvas.addChildComponent (*flightDeck);

    header.setBounds (Aviation::topHeaderBounds());
    header.onModeChanged = [this] (bool performance) { setPerformanceView (performance); };
    header.onSaveClicked = [this]
    {
        auto& pm = processorRef.getPresetManager();
        pm.saveUserPreset (pm.getCurrentCategory(), pm.getCurrentPresetName());
    };
    header.onSettingsClicked = [this] { openLibraryOverlay(); };
    header.onUtilityClicked = [this] { openAboutOverlay(); };
    canvas.addAndMakeVisible (header);

    auto& presetManager = p.getPresetManager();
    previousPresetLoadedHandler = presetManager.onPresetLoaded;
    presetManager.onPresetLoaded = [this, previousPresetLoaded = previousPresetLoadedHandler] (const juce::String& category,
                                                                                              const juce::String& name,
                                                                                              const juce::String& sampleId,
                                                                                              int rootNote) {
        if (previousPresetLoaded)
            previousPresetLoaded (category, name, sampleId, rootNote);

        if (flightDeck != nullptr)
            flightDeck->refreshPresetUI();
    };

    previousUserSampleHandler = p.onUserSampleChanged;
    p.onUserSampleChanged = [this, previous = previousUserSampleHandler]
    {
        if (previous)
            previous();
        if (flightDeck != nullptr)
            flightDeck->refreshPresetUI();
        if (mainView != nullptr)
            mainView->requestPresetUiRefresh();
    };

    setPerformanceView (false);

    layoutContent();
    repaint();

#if JUCE_DEBUG
    // UI validation harness: AVIATORKEYZ_SNAPSHOT_PATH=<file.png> renders the
    // full design canvas (header + current page) at 1647x955 and quits (debug
    // only). AVIATORKEYZ_SNAPSHOT_VIEW=performance renders the Flight Deck page.
    const auto snapshotPath = juce::SystemStats::getEnvironmentVariable ("AVIATORKEYZ_SNAPSHOT_PATH", {});
    if (snapshotPath.isNotEmpty())
    {
        const bool performance = juce::SystemStats::getEnvironmentVariable ("AVIATORKEYZ_SNAPSHOT_VIEW", {})
                                     .equalsIgnoreCase ("performance");
        if (performance)
            setPerformanceView (true);

        juce::Timer::callAfterDelay (1000,
            [safe = juce::Component::SafePointer<AviatorKeyzEditor> (this), snapshotPath]
            {
                if (safe == nullptr)
                    return;

                auto& target = safe->canvas;
                auto image = target.createComponentSnapshot (target.getLocalBounds(), false, 1.0f);

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
    processorRef.onUserSampleChanged = previousUserSampleHandler;
}

void AviatorKeyzEditor::setPerformanceView (bool performance)
{
    performanceView = performance;
    header.setPerformanceSelected (performance);

    // Pages swap; the header stays. Nothing from the hidden page shows through.
    if (mainView != nullptr)
        mainView->setVisible (! performance);
    if (flightDeck != nullptr)
        flightDeck->setVisible (performance);
}

void AviatorKeyzEditor::openLibraryOverlay()
{
    if (libraryOverlay == nullptr)
    {
        libraryOverlay = std::make_unique<PresetLibraryOverlay> (processorRef);
        libraryOverlay->setBounds (canvas.getLocalBounds());
        canvas.addChildComponent (*libraryOverlay);
    }
    libraryOverlay->showOverlay();
    libraryOverlay->toFront (true);
}

void AviatorKeyzEditor::openAboutOverlay()
{
    if (aboutOverlay == nullptr)
    {
        aboutOverlay = std::make_unique<AboutOverlay>();
        aboutOverlay->setBounds (canvas.getLocalBounds());
        canvas.addChildComponent (*aboutOverlay);
    }
    aboutOverlay->showOverlay();
    aboutOverlay->toFront (true);
}

void AviatorKeyzEditor::layoutContent()
{
    if (getWidth() <= 0 || getHeight() <= 0)
        return;

    // The whole canvas lives in the fixed design space; scale it as one unit.
    const float scale = (float) getWidth() / (float) Aviation::kDesignW;
    canvas.setTransform (juce::AffineTransform::scale (scale));
    canvas.setBounds (0, 0, Aviation::kDesignW, Aviation::kDesignH);
}

void AviatorKeyzEditor::paint (juce::Graphics& g)
{
    g.fillAll (Aviation::bgBlack());
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
