#pragma once

#include "DeckWidgets.h"
#include "../../DSP/RollingSampler.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>
#include <vector>

class AviatorKeyzProcessor;

// =============================================================================
//  RollingSamplerStrip — the live 30-second sampler, inline in the Flight Deck's
//  title strip. It occupies that row and nothing else: no zone below it is
//  covered, moved or resized.
//
//    ○ LIVE   ▁▂▃▅▂▁▃▅▂▁ 30 s rolling ▂▃▅▁▂   1.84s   TO CARGO  DRAG   SRC · KEY
//
//  Audio rolls in at the right edge of the inline wave and is deleted off the
//  left. Drag the wave to mark a region, then send it to CARGO HOLD for further
//  flipping or drag it straight into the DAW as a WAV. The region is held as
//  absolute frame positions, so it keeps pointing at the same audio while the
//  window scrolls under it, and turns red once it starts rolling out.
//
//  The strip also carries the SRC / KEY / BPM / MODE readout that has always
//  lived at the right of this row.
// =============================================================================

class RollingSamplerStrip : public juce::Component,
                            private juce::Timer
{
public:
    explicit RollingSamplerStrip (AviatorKeyzProcessor& processor);
    ~RollingSamplerStrip() override;

    /** Fired after a region is loaded into cargo as the active sound. */
    std::function<void()> onRegionLoaded;

    void paint (juce::Graphics& g) override;
    void resized() override;

    void mouseMove (const juce::MouseEvent& e) override;
    void mouseExit (const juce::MouseEvent&) override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;
    void mouseUp (const juce::MouseEvent& e) override;
    void mouseDoubleClick (const juce::MouseEvent& e) override;

private:
    enum class Grab { none, newRegion, moveStart, moveEnd, dragOut };

    void timerCallback() override;
    void showStatus (const juce::String& text, bool isError);
    void toggleLive();
    void selectAll();
    void sendToCargo();
    void beginExternalDrag();

    juce::Rectangle<int> liveBounds() const;
    juce::Rectangle<int> waveBounds() const;
    juce::Rectangle<int> cargoBounds() const;
    juce::Rectangle<int> dragBounds() const;
    juce::Rectangle<int> readoutBounds() const;

    float   xForFrame (int64_t frame) const;
    int64_t frameForX (int x) const;

    bool hasRegion() const noexcept { return regionEnd > regionStart; }
    bool regionIsAging() const noexcept;
    double regionSeconds() const;

    AviatorKeyzProcessor& processorRef;

    // The window as of the last repaint, so hit-testing and painting agree
    // even though "now" advances continuously.
    int64_t viewNow { 0 };
    int64_t viewOldest { 0 };
    int64_t windowFrames { 1 };

    int64_t regionStart { 0 };
    int64_t regionEnd { 0 };

    Grab grab { Grab::none };
    int  hoverItem { -1 };      // 0 live, 1 cargo, 2 drag
    bool dragStarted { false };

    float  meterLevel { 0.f };
    double heldSeconds { 0.0 };

    std::vector<RollingSampler::Bucket> columns;

    juce::String statusText;
    bool         statusIsError { false };
    juce::uint32 statusUntilMs { 0 };

    juce::String readoutLine1, readoutLine2;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RollingSamplerStrip)
};
