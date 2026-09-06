#pragma once

#include "DeckWidgets.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>

class AviatorKeyzProcessor;

// =============================================================================
//  CargoHoldZone — user sample import.
//  Drop a WAV/AIFF (<= 60 s) on the zone or click DROP CARGO to browse. The
//  waveform strip shows the loaded sound (factory or cargo) with draggable
//  TRIM handles (src_start / src_end) and the live playhead; the control row
//  drives playback mode, loop, host sync and root note.
// =============================================================================

class CargoHoldZone : public juce::Component,
                      public juce::FileDragAndDropTarget,
                      private juce::Timer
{
public:
    explicit CargoHoldZone (AviatorKeyzProcessor& processor);
    ~CargoHoldZone() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

    bool isInterestedInFileDrag (const juce::StringArray& files) override;
    void fileDragEnter (const juce::StringArray&, int, int) override;
    void fileDragExit (const juce::StringArray&) override;
    void filesDropped (const juce::StringArray& files, int x, int y) override;

    /** Message thread: refresh readouts after a preset / sample change. */
    void refreshFromProcessor();

private:
    class DropZone;
    class WaveBox;

    void timerCallback() override;
    void loadFile (const juce::File& file);
    void browse();
    void showError (const juce::String& message);

    AviatorKeyzProcessor& processorRef;
    juce::AudioProcessorValueTreeState& apvts;

    std::unique_ptr<DropZone> dropZone;
    std::unique_ptr<WaveBox> waveBox;
    std::unique_ptr<DeckSegment> modeSeg;
    std::unique_ptr<DeckChip> loopChip, syncChip, rootChip, trimChip, feedChip;
    std::unique_ptr<juce::FileChooser> chooser;

    juce::String errorText;
    juce::uint32 errorUntilMs { 0 };
    bool dragOver { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CargoHoldZone)
};
