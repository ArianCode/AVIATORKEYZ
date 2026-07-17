#include "PerformanceApvtsReader.h"
#include "../../State/StateSchema.h"
#include "../../State/CategorySoundPolicy.h"

namespace
{
float load (const juce::AudioProcessorValueTreeState& apvts, const char* id) noexcept
{
    if (auto* p = apvts.getRawParameterValue (id))
        return p->load();
    return 0.f;
}

bool loadBool (const juce::AudioProcessorValueTreeState& apvts, const char* id) noexcept
{
    return load (apvts, id) > 0.5f;
}
} // namespace

EngineState PerformanceApvtsReader::readBaseState (const juce::AudioProcessorValueTreeState& apvts) noexcept
{
    namespace P = AviatorKeyz::ParamID;
    EngineState s;

    s.source.start = load (apvts, P::SRC_START);
    s.source.end = juce::jmax (s.source.start + 0.01f, load (apvts, P::SRC_END));
    s.source.tune = load (apvts, P::SRC_TUNE);
    s.source.speed = load (apvts, P::SRC_SPEED);
    s.source.reverse = loadBool (apvts, P::SRC_REVERSE);
    s.source.bpmSync = loadBool (apvts, P::SRC_BPM_SYNC);
    s.source.originalBpm = load (apvts, P::SRC_ORIGINAL_BPM);
    s.source.rootNote = static_cast<int> (load (apvts, P::SRC_ROOT_NOTE));
    s.source.playbackMode = static_cast<SamplePlaybackMode> (
        juce::jlimit (0, 4, static_cast<int> (load (apvts, P::SRC_PLAYBACK_MODE))));
    s.source.keytrack = loadBool (apvts, P::SRC_KEYTRACK);
    s.source.loopMode = static_cast<LoopMode> (juce::jlimit (0, 2, static_cast<int> (load (apvts, P::SRC_LOOP_MODE))));

    s.chop.enabled = loadBool (apvts, P::CHOP_ON);
    s.chop.amount = load (apvts, P::CHOP_AMOUNT);
    s.chop.rateIndex = juce::jlimit (0, 3, static_cast<int> (load (apvts, P::CHOP_RATE)));
    s.chop.gate = load (apvts, P::CHOP_GATE);
    s.chop.swing = load (apvts, P::CHOP_SWING);
    s.chop.random = load (apvts, P::CHOP_RANDOM);
    s.chop.reverseChance = load (apvts, P::CHOP_REVERSE_CHANCE);
    s.chop.smooth = load (apvts, P::CHOP_SMOOTH);

    for (int i = 0; i < P::CHOP_STEP_COUNT; ++i)
    {
        auto& step = s.chop.steps[static_cast<size_t> (i)];
        step.enabled = loadBool (apvts, P::chopStepParamId (i, "on").toRawUTF8());
        step.volume = load (apvts, P::chopStepParamId (i, "vol").toRawUTF8());
        step.sliceOffset = load (apvts, P::chopStepParamId (i, "offset").toRawUTF8());
        step.reverse = loadBool (apvts, P::chopStepParamId (i, "rev").toRawUTF8());
        step.pitchOffset = static_cast<int> (load (apvts, P::chopStepParamId (i, "pitch").toRawUTF8()));
    }

    s.texture.enabled = loadBool (apvts, P::PTEX_ON);
    s.texture.freeze = loadBool (apvts, P::PTEX_FREEZE);
    s.texture.grainSize = load (apvts, P::PTEX_GRAIN_SIZE);
    s.texture.density = load (apvts, P::PTEX_DENSITY);
    s.texture.position = load (apvts, P::PTEX_POSITION);
    s.texture.pitchSpread = load (apvts, P::PTEX_PITCH_SPREAD);
    s.texture.smear = load (apvts, P::PTEX_SMEAR);
    s.texture.width = load (apvts, P::PTEX_WIDTH);
    s.texture.mix = load (apvts, P::PTEX_MIX);

    s.performance.mode = static_cast<PerformanceMode> (juce::jlimit (0, 7, static_cast<int> (load (apvts, P::PERF_MODE))));
    s.performance.stutter = loadBool (apvts, P::PERF_FX_STUTTER);
    s.performance.reverse = loadBool (apvts, P::PERF_FX_REVERSE);
    s.performance.halfTime = loadBool (apvts, P::PERF_FX_HALF_TIME);
    s.performance.freeze = loadBool (apvts, P::PERF_FX_FREEZE);
    s.performance.tapeStop = loadBool (apvts, P::PERF_FX_TAPE_STOP);
    s.performance.scatter = loadBool (apvts, P::PERF_FX_SCATTER);
    s.performance.pitchDrop = loadBool (apvts, P::PERF_FX_PITCH_DROP);
    s.performance.filterSweep = loadBool (apvts, P::PERF_FX_FILTER_SWEEP);

    return s;
}

void PerformanceApvtsReader::applyCategoryPlaybackDefaults (
    juce::AudioProcessorValueTreeState& apvts,
    const juce::ValueTree& loadedState,
    const juce::String& category,
    const juce::String& presetName,
    const juce::String& soundTypeAttr,
    const bool isFactoryPreset) noexcept
{
    juce::ignoreUnused (loadedState);

    const auto soundType = soundTypeAttr.isNotEmpty()
                               ? AviatorKeyz::soundTypeFromString (soundTypeAttr)
                               : AviatorKeyz::inferSoundTypeFromStem (category, presetName);

    if (isFactoryPreset)
    {
        AviatorKeyz::applyPlaybackPolicyToApvts (apvts, category, soundType);
        return;
    }

    auto stateHasParam = [&] (const char* id) -> bool
    {
        for (int i = 0; i < loadedState.getNumChildren(); ++i)
        {
            const auto child = loadedState.getChild (i);
            if (child.hasType ("PARAM") && child.getProperty ("id").toString() == id)
                return true;
        }
        return false;
    };

    if (stateHasParam (AviatorKeyz::ParamID::SRC_PLAYBACK_MODE))
        return;

    AviatorKeyz::applyPlaybackPolicyToApvts (apvts, category, soundType);
}

float PerformanceApvtsReader::readMacroValue (const juce::AudioProcessorValueTreeState& apvts, int macroIndex) noexcept
{
    namespace P = AviatorKeyz::ParamID;
    switch (macroIndex)
    {
        case 0: return load (apvts, P::PERF_MACRO_1);
        case 1: return load (apvts, P::PERF_MACRO_2);
        case 2: return load (apvts, P::PERF_MACRO_3);
        case 3: return load (apvts, P::PERF_MACRO_4);
        default: return 0.5f;
    }
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
