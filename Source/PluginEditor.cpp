#include "PluginEditor.h"
#include "GUI/Aviation/AviationTheme.h"
#include "GUI/FlightDeck/DeckWidgets.h"
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

    mainView = std::make_unique<AviationMainView> (p);
    mainView->onModeChanged = [this] (bool performance) { setPerformanceView (performance); };
    addAndMakeVisible (*mainView);

    flightDeck = std::make_unique<FlightDeckView> (p);
    flightDeck->setVisible (false);
    addAndMakeVisible (*flightDeck);

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
    // main view at the canonical 1647x955 design size and quits (debug only).
    // AVIATORKEYZ_SNAPSHOT_VIEW=performance renders the Flight Deck (1366x860).
    const auto snapshotPath = juce::SystemStats::getEnvironmentVariable ("AVIATORKEYZ_SNAPSHOT_PATH", {});
    if (snapshotPath.isNotEmpty())
    {
        const bool performance = juce::SystemStats::getEnvironmentVariable ("AVIATORKEYZ_SNAPSHOT_VIEW", {})
                                     .equalsIgnoreCase ("performance");
        if (performance)
            setPerformanceView (true);

        juce::Timer::callAfterDelay (1000,
            [safe = juce::Component::SafePointer<AviatorKeyzEditor> (this), snapshotPath, performance]
            {
                if (safe == nullptr || safe->mainView == nullptr)
                    return;

                juce::Component* target = performance ? (juce::Component*) safe->flightDeck.get()
                                                      : (juce::Component*) safe->mainView.get();
                auto image = target->createComponentSnapshot (target->getLocalBounds(), false, 1.0f);

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
    mainView->getTopHeader().setPerformanceSelected (performance);
    if (flightDeck != nullptr)
        flightDeck->setVisible (performance);
    layoutContent();
}

void AviatorKeyzEditor::layoutContent()
{
    if (mainView == nullptr || getWidth() <= 0 || getHeight() <= 0)
        return;

    // The main view lives in the fixed design space; scale it as one unit.
    const float scale = (float) getWidth() / (float) Aviation::kDesignW;
    mainView->setTransform (juce::AffineTransform::scale (scale));
    mainView->setBounds (0, 0, Aviation::kDesignW, Aviation::kDesignH);

    // The Flight Deck replaces everything below the top header, scaled to fit
    // that area while keeping its own 1366:860 composition, centred.
    if (flightDeck != nullptr && flightDeck->isVisible())
    {
        const int headerBottom = juce::roundToInt ((float) (Aviation::topHeaderBounds().getBottom() + 2) * scale);
        const float availW = (float) getWidth();
        const float availH = (float) juce::jmax (1, getHeight() - headerBottom);
        const float deckScale = juce::jmin (availW / (float) Deck::kDesignW, availH / (float) Deck::kDesignH);
        const float x = (availW - Deck::kDesignW * deckScale) * 0.5f;
        const float y = (float) headerBottom + (availH - Deck::kDesignH * deckScale) * 0.5f;
        flightDeck->setTransform (juce::AffineTransform::scale (deckScale).translated (x, y));
        flightDeck->setBounds (0, 0, Deck::kDesignW, Deck::kDesignH);
        flightDeck->toFront (false);
    }
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
