#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace AviatorKeyz
{

/** True when state uses `<PARAM id=… value=…/>` children (JUCE 8 APVTS + factory presets). */
bool stateTreeUsesParamChildren (const juce::ValueTree& state);

/** True for bundled factory presets that only store the core 12 performance params. */
bool isPartialFactoryPresetState (const juce::ValueTree& state);

/** Reset every registered APVTS parameter to its layout default (normalized). */
void resetApvtsToDefaults (juce::AudioProcessorValueTreeState& apvts);

/**
 * Apply a preset / host state tree to APVTS.
 *
 * - Factory preset format (PARAM children, denormalized values): reset defaults,
 *   apply each PARAM, then force sample-only source blend.
 * - Host / user preset format (APVTS copyState properties, normalized 0–1):
 *   delegate to apvts.replaceState().
 */
void applyStateTreeToApvts (juce::AudioProcessorValueTreeState& apvts,
                            const juce::ValueTree& state);

/** Copy legacy phrase/tex params into v4 performance params when missing. */
void migrateLegacyAdvancedParams (juce::ValueTree& state);

} // namespace AviatorKeyz
