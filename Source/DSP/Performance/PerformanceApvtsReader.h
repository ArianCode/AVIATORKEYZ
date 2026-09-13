#pragma once

#include "PerformanceTypes.h"
#include <juce_audio_processors/juce_audio_processors.h>

class PerformanceApvtsReader
{
public:
    /** Cached raw-atomic parameter pointers for every value read per audio block.

        Built once on the message thread (processor constructor). Audio-thread
        reads through the cache perform no string formatting, no map lookups,
        and no allocation — chopStepParamId() in particular builds a formatted
        juce::String and must never be called from the audio thread.
    */
    struct ParamCache
    {
        void init (const juce::AudioProcessorValueTreeState& apvts);

        enum StepField { stepOn = 0, stepVol, stepOffset, stepRev, stepPitch, numStepFields };

        // Source
        std::atomic<float>* srcStart = nullptr;
        std::atomic<float>* srcEnd = nullptr;
        std::atomic<float>* srcTune = nullptr;
        std::atomic<float>* srcSpeed = nullptr;
        std::atomic<float>* srcSpeedSnap = nullptr;
        std::atomic<float>* srcReverse = nullptr;
        std::atomic<float>* srcBpmSync = nullptr;
        std::atomic<float>* srcOriginalBpm = nullptr;
        std::atomic<float>* srcRootNote = nullptr;
        std::atomic<float>* srcPlaybackMode = nullptr;
        std::atomic<float>* srcKeytrack = nullptr;
        std::atomic<float>* srcLoopMode = nullptr;
        std::atomic<float>* srcLoopStart = nullptr;
        std::atomic<float>* srcLoopEnd = nullptr;

        // Slice pads
        std::atomic<float>* sliceDiv = nullptr;
        std::atomic<float>* sliceRandom = nullptr;
        std::atomic<float>* sliceCut[15] = {};

        // Chop
        std::atomic<float>* chopOn = nullptr;
        std::atomic<float>* chopAmount = nullptr;
        std::atomic<float>* chopRate = nullptr;
        std::atomic<float>* chopGate = nullptr;
        std::atomic<float>* chopSwing = nullptr;
        std::atomic<float>* chopRandom = nullptr;
        std::atomic<float>* chopReverseChance = nullptr;
        std::atomic<float>* chopSmooth = nullptr;
        std::atomic<float>* chopStep[16][numStepFields] = {};

        // Texture
        std::atomic<float>* ptexOn = nullptr;
        std::atomic<float>* ptexFreeze = nullptr;
        std::atomic<float>* ptexGrainSize = nullptr;
        std::atomic<float>* ptexDensity = nullptr;
        std::atomic<float>* ptexPosition = nullptr;
        std::atomic<float>* ptexPitchSpread = nullptr;
        std::atomic<float>* ptexSmear = nullptr;
        std::atomic<float>* ptexWidth = nullptr;
        std::atomic<float>* ptexMix = nullptr;

        // Performance
        std::atomic<float>* perfMode = nullptr;
        std::atomic<float>* perfStutter = nullptr;
        std::atomic<float>* perfReverse = nullptr;
        std::atomic<float>* perfHalfTime = nullptr;
        std::atomic<float>* perfFreeze = nullptr;
        std::atomic<float>* perfTapeStop = nullptr;
        std::atomic<float>* perfScatter = nullptr;
        std::atomic<float>* perfPitchDrop = nullptr;
        std::atomic<float>* perfFilterSweep = nullptr;

        std::atomic<float>* macros[4] = {};
    };

    static EngineState readBaseState (const ParamCache& cache) noexcept;

    static float readMacroValue (const ParamCache& cache, int macroIndex) noexcept;

    /** Apply category playback locks. Factory presets always force policy. */
    static void applyCategoryPlaybackDefaults (juce::AudioProcessorValueTreeState& apvts,
                                               const juce::ValueTree& loadedState,
                                               const juce::String& category,
                                               const juce::String& presetName,
                                               const juce::String& soundTypeAttr,
                                               bool isFactoryPreset) noexcept;
};
