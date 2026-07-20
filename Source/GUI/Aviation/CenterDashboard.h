#pragma once

#include "MiniControls.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>
#include <memory>

// =============================================================================
//  CenterDashboard — the cockpit's central instrument console.
//    - mini display strip: RPM (host BPM) / KEY / TUNE (src_tune, draggable)
//    - centered gold preset title
//    - display row: GLOBALS (output_gain) | radar | aircraft blueprint |
//                   radar | LIMITER (output_limiter)
//    - lower strip: LOFI / STEREO / DYNAMICS / WIDTH / HUMANIZE cells
// =============================================================================

class CenterDashboard : public juce::Component,
                        private juce::Timer
{
public:
    explicit CenterDashboard (juce::AudioProcessorValueTreeState& apvts);
    ~CenterDashboard() override;

    /** Host tempo provider (message thread; returns BPM). */
    std::function<double()> bpmProvider;

    void setKeyText (const juce::String& text);
    void setTitleText (const juce::String& text);

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    class LimiterCell;

    void timerCallback() override;
    void paintRadar (juce::Graphics& g, juce::Rectangle<float> area, float sweepPhase, bool clockwise);
    void paintBlueprint (juce::Graphics& g, juce::Rectangle<float> area);
    void paintMiniDisplay (juce::Graphics& g);

    juce::Rectangle<int> miniDisplayArea() const;
    juce::Rectangle<int> radarLeftArea() const;
    juce::Rectangle<int> radarRightArea() const;
    juce::Rectangle<int> blueprintArea() const;

    juce::AudioProcessorValueTreeState& apvtsRef;
    juce::String keyText { "-" };
    juce::String titleText;

    std::unique_ptr<MiniRotary> globalsKnob;
    std::unique_ptr<LimiterCell> limiterCell;
    std::unique_ptr<MiniParam> lofiCell, stereoCell, dynamicsCell, widthCell, humanizeCell;

    AviationMini::FineDragSlider tuneSlider { juce::Slider::RotaryVerticalDrag, juce::Slider::NoTextBox };
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> tuneAttachment;

    float sweepPhase { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CenterDashboard)
};
