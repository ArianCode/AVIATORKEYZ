#include "PerformanceApvtsReader.h"
#include "../../State/StateSchema.h"
#include "../../State/CategorySoundPolicy.h"

namespace
{
float load (const std::atomic<float>* p) noexcept
{
    return p != nullptr ? p->load() : 0.f;
}

bool loadBool (const std::atomic<float>* p) noexcept
{
    return load (p) > 0.5f;
}
} // namespace

void PerformanceApvtsReader::ParamCache::init (const juce::AudioProcessorValueTreeState& apvts)
{
    namespace P = AviatorKeyz::ParamID;

    auto get = [&apvts] (const char* id) { return apvts.getRawParameterValue (id); };

    srcStart = get (P::SRC_START);
    srcEnd = get (P::SRC_END);
    srcTune = get (P::SRC_TUNE);
    srcSpeed = get (P::SRC_SPEED);
    srcReverse = get (P::SRC_REVERSE);
    srcBpmSync = get (P::SRC_BPM_SYNC);
    srcOriginalBpm = get (P::SRC_ORIGINAL_BPM);
    srcRootNote = get (P::SRC_ROOT_NOTE);
    srcPlaybackMode = get (P::SRC_PLAYBACK_MODE);
    srcKeytrack = get (P::SRC_KEYTRACK);
    srcLoopMode = get (P::SRC_LOOP_MODE);

    chopOn = get (P::CHOP_ON);
    chopAmount = get (P::CHOP_AMOUNT);
    chopRate = get (P::CHOP_RATE);
    chopGate = get (P::CHOP_GATE);
    chopSwing = get (P::CHOP_SWING);
    chopRandom = get (P::CHOP_RANDOM);
    chopReverseChance = get (P::CHOP_REVERSE_CHANCE);
    chopSmooth = get (P::CHOP_SMOOTH);

    static constexpr const char* kStepSuffixes[numStepFields] { "on", "vol", "offset", "rev", "pitch" };
    for (int i = 0; i < P::CHOP_STEP_COUNT; ++i)
        for (int f = 0; f < numStepFields; ++f)
            chopStep[i][f] = apvts.getRawParameterValue (P::chopStepParamId (i, kStepSuffixes[f]));

    ptexOn = get (P::PTEX_ON);
    ptexFreeze = get (P::PTEX_FREEZE);
    ptexGrainSize = get (P::PTEX_GRAIN_SIZE);
    ptexDensity = get (P::PTEX_DENSITY);
    ptexPosition = get (P::PTEX_POSITION);
    ptexPitchSpread = get (P::PTEX_PITCH_SPREAD);
    ptexSmear = get (P::PTEX_SMEAR);
    ptexWidth = get (P::PTEX_WIDTH);
    ptexMix = get (P::PTEX_MIX);

    perfMode = get (P::PERF_MODE);
    perfStutter = get (P::PERF_FX_STUTTER);
    perfReverse = get (P::PERF_FX_REVERSE);
    perfHalfTime = get (P::PERF_FX_HALF_TIME);
    perfFreeze = get (P::PERF_FX_FREEZE);
    perfTapeStop = get (P::PERF_FX_TAPE_STOP);
    perfScatter = get (P::PERF_FX_SCATTER);
    perfPitchDrop = get (P::PERF_FX_PITCH_DROP);
    perfFilterSweep = get (P::PERF_FX_FILTER_SWEEP);

    macros[0] = get (P::PERF_MACRO_1);
    macros[1] = get (P::PERF_MACRO_2);
    macros[2] = get (P::PERF_MACRO_3);
    macros[3] = get (P::PERF_MACRO_4);
}

EngineState PerformanceApvtsReader::readBaseState (const ParamCache& c) noexcept
{
    EngineState s;

    s.source.start = load (c.srcStart);
    s.source.end = juce::jmax (s.source.start + 0.01f, load (c.srcEnd));
    s.source.tune = load (c.srcTune);
    s.source.speed = load (c.srcSpeed);
    s.source.reverse = loadBool (c.srcReverse);
    s.source.bpmSync = loadBool (c.srcBpmSync);
    s.source.originalBpm = load (c.srcOriginalBpm);
    s.source.rootNote = static_cast<int> (load (c.srcRootNote));
    s.source.playbackMode = static_cast<SamplePlaybackMode> (
        juce::jlimit (0, 4, static_cast<int> (load (c.srcPlaybackMode))));
    s.source.keytrack = loadBool (c.srcKeytrack);
    s.source.loopMode = static_cast<LoopMode> (juce::jlimit (0, 2, static_cast<int> (load (c.srcLoopMode))));

    s.chop.enabled = loadBool (c.chopOn);
    s.chop.amount = load (c.chopAmount);
    s.chop.rateIndex = juce::jlimit (0, 3, static_cast<int> (load (c.chopRate)));
    s.chop.gate = load (c.chopGate);
    s.chop.swing = load (c.chopSwing);
    s.chop.random = load (c.chopRandom);
    s.chop.reverseChance = load (c.chopReverseChance);
    s.chop.smooth = load (c.chopSmooth);

    for (int i = 0; i < AviatorKeyz::ParamID::CHOP_STEP_COUNT; ++i)
    {
        auto& step = s.chop.steps[static_cast<size_t> (i)];
        step.enabled = loadBool (c.chopStep[i][ParamCache::stepOn]);
        step.volume = load (c.chopStep[i][ParamCache::stepVol]);
        step.sliceOffset = load (c.chopStep[i][ParamCache::stepOffset]);
        step.reverse = loadBool (c.chopStep[i][ParamCache::stepRev]);
        step.pitchOffset = static_cast<int> (load (c.chopStep[i][ParamCache::stepPitch]));
    }

    s.texture.enabled = loadBool (c.ptexOn);
    s.texture.freeze = loadBool (c.ptexFreeze);
    s.texture.grainSize = load (c.ptexGrainSize);
    s.texture.density = load (c.ptexDensity);
    s.texture.position = load (c.ptexPosition);
    s.texture.pitchSpread = load (c.ptexPitchSpread);
    s.texture.smear = load (c.ptexSmear);
    s.texture.width = load (c.ptexWidth);
    s.texture.mix = load (c.ptexMix);

    s.performance.mode = static_cast<PerformanceMode> (juce::jlimit (0, 7, static_cast<int> (load (c.perfMode))));
    s.performance.stutter = loadBool (c.perfStutter);
    s.performance.reverse = loadBool (c.perfReverse);
    s.performance.halfTime = loadBool (c.perfHalfTime);
    s.performance.freeze = loadBool (c.perfFreeze);
    s.performance.tapeStop = loadBool (c.perfTapeStop);
    s.performance.scatter = loadBool (c.perfScatter);
    s.performance.pitchDrop = loadBool (c.perfPitchDrop);
    s.performance.filterSweep = loadBool (c.perfFilterSweep);

    return s;
}

float PerformanceApvtsReader::readMacroValue (const ParamCache& cache, int macroIndex) noexcept
{
    if (macroIndex < 0 || macroIndex > 3)
        return 0.5f;
    return load (cache.macros[macroIndex]);
}

void PerformanceApvtsReader::applyCategoryPlaybackDefaults (
    juce::AudioProcessorValueTreeState& apvts,
    const juce::ValueTree& loadedState,
    const juce::String& category,
    const juce::String& presetName,
    const juce::String& soundTypeAttr,
    const bool isFactoryPreset) noexcept
{
    juce::ignoreUnused (loadedState, isFactoryPreset);

    const auto soundType = soundTypeAttr.isNotEmpty()
                               ? AviatorKeyz::soundTypeFromString (soundTypeAttr)
                               : AviatorKeyz::inferSoundTypeFromStem (category, presetName);

    // Preset metadata owns playback behavior — always apply, never leave to a UI algorithm picker.
    AviatorKeyz::applyPlaybackPolicyToApvts (apvts, category, soundType);
}

namespace AviatorKeyz
{

void migrateLegacyAdvancedParams (juce::ValueTree& state)
{
    if (! state.isValid())
        return;

    auto hasParam = [&] (const char* id) -> bool
    {
        for (int i = 0; i < state.getNumChildren(); ++i)
        {
            const auto child = state.getChild (i);
            if (child.hasType ("PARAM") && child.getProperty ("id").toString() == id)
                return true;
        }
        return false;
    };

    auto getParamValue = [&] (const char* id) -> float
    {
        for (int i = 0; i < state.getNumChildren(); ++i)
        {
            const auto child = state.getChild (i);
            if (child.hasType ("PARAM") && child.getProperty ("id").toString() == id)
                return static_cast<float> (child.getProperty ("value"));
        }
        return 0.f;
    };

    auto setParamIfMissing = [&] (const char* id, float value)
    {
        if (! hasParam (id))
        {
            juce::ValueTree p ("PARAM");
            p.setProperty ("id", id, nullptr);
            p.setProperty ("value", value, nullptr);
            state.appendChild (p, nullptr);
        }
    };

    namespace P = AviatorKeyz::ParamID;

    if (! hasParam (P::SRC_START) && hasParam (P::PHRASE_START))
    {
        const float start = getParamValue (P::PHRASE_START);
        const float len = hasParam (P::PHRASE_LENGTH) ? getParamValue (P::PHRASE_LENGTH) : 1.f;
        setParamIfMissing (P::SRC_START, start);
        setParamIfMissing (P::SRC_END, juce::jmin (1.f, start + len));
    }

    if (! hasParam (P::SRC_TUNE) && hasParam (P::PHRASE_PITCH))
        setParamIfMissing (P::SRC_TUNE, getParamValue (P::PHRASE_PITCH));

    if (! hasParam (P::SRC_BPM_SYNC) && hasParam (P::PHRASE_TEMPO_SYNC))
        setParamIfMissing (P::SRC_BPM_SYNC, getParamValue (P::PHRASE_TEMPO_SYNC));

    if (! hasParam (P::SRC_LOOP_MODE) && hasParam (P::PHRASE_LOOP))
        setParamIfMissing (P::SRC_LOOP_MODE, getParamValue (P::PHRASE_LOOP) > 0.5f ? 1.f : 0.f);

    if (! hasParam (P::PTEX_MIX) && hasParam (P::TEX_AMOUNT))
        setParamIfMissing (P::PTEX_MIX, getParamValue (P::TEX_AMOUNT));

    if (! hasParam (P::PTEX_ON) && hasParam (P::TEX_ENABLED))
        setParamIfMissing (P::PTEX_ON, getParamValue (P::TEX_ENABLED));

    if (! hasParam (P::PTEX_GRAIN_SIZE) && hasParam (P::TEX_GRAIN_SIZE))
        setParamIfMissing (P::PTEX_GRAIN_SIZE, getParamValue (P::TEX_GRAIN_SIZE));

    if (! hasParam (P::PTEX_DENSITY) && hasParam (P::TEX_GRAIN_DENSITY))
        setParamIfMissing (P::PTEX_DENSITY, getParamValue (P::TEX_GRAIN_DENSITY));

    if (! hasParam (P::PTEX_WIDTH) && hasParam (P::TEX_WIDTH))
        setParamIfMissing (P::PTEX_WIDTH, getParamValue (P::TEX_WIDTH));

    setParamIfMissing (P::SOURCE_BLEND, 0.f);

    if (! hasParam (P::SRC_PLAYBACK_MODE))
        setParamIfMissing (P::SRC_PLAYBACK_MODE, static_cast<float> (SamplePlaybackMode::PhraseOriginal));

    if (! hasParam (P::SRC_KEYTRACK))
        setParamIfMissing (P::SRC_KEYTRACK, 0.f);
}

} // namespace AviatorKeyz
