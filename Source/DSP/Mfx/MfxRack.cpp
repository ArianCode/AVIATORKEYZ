#include "MfxRack.h"

MfxRack::MfxRack()
{
    for (auto& slot : slots)
        for (int e = 0; e < (int) Mfx::Effect::count; ++e)
            slot.effects[(size_t) e].reset (makeEffect (static_cast<Mfx::Effect> (e)));
}

Mfx::EffectProcessor* MfxRack::makeEffect (Mfx::Effect e)
{
    switch (e)
    {
        case Mfx::Effect::grainCloud:  return new Mfx::GrainCloud();
        case Mfx::Effect::sweepFilter: return new Mfx::SweepFilter();
        case Mfx::Effect::tapeEcho:    return new Mfx::TapeEcho();
        case Mfx::Effect::saturator:   return new Mfx::Saturator();
        case Mfx::Effect::stutter:     return new Mfx::Stutter();
        case Mfx::Effect::freeze:      return new Mfx::Freeze();
        case Mfx::Effect::bitcrusher:  return new Mfx::Bitcrusher();
        case Mfx::Effect::compressor:  return new Mfx::Compressor();
        case Mfx::Effect::chorus:      return new Mfx::Chorus();
        case Mfx::Effect::space:       return new Mfx::Space();
        case Mfx::Effect::count:       break;
    }
    return new Mfx::Saturator();
}

void MfxRack::attachParameters (juce::AudioProcessorValueTreeState& apvts)
{
    for (int s = 0; s < Mfx::kNumSlots; ++s)
    {
        auto& slot = slots[(size_t) s];
        slot.on = apvts.getRawParameterValue (Mfx::onId (s));
        slot.effect = apvts.getRawParameterValue (Mfx::effectId (s));
        slot.send = apvts.getRawParameterValue (Mfx::sendId (s));
        slot.level = apvts.getRawParameterValue (Mfx::levelId (s));
        for (int p = 0; p < Mfx::kParamsPerSlot; ++p)
            slot.params[(size_t) p] = apvts.getRawParameterValue (Mfx::paramId (s, p));
        for (int a = 0; a < Mfx::kNumAssigns; ++a)
        {
            slot.assignSrc[(size_t) a] = apvts.getRawParameterValue (Mfx::assignSourceId (s, a));
            slot.assignAmt[(size_t) a] = apvts.getRawParameterValue (Mfx::assignAmountId (s, a));
        }
    }
}

void MfxRack::prepare (const juce::dsp::ProcessSpec& spec)
{
    for (auto& slot : slots)
    {
        for (auto& fx : slot.effects)
            fx->prepare (spec);
        slot.activeEffect = -1;
        slot.levelSmoothed.reset (spec.sampleRate, 0.02);
        slot.levelSmoothed.setCurrentAndTargetValue (1.f);
    }
    sendReverb.setSampleRate (spec.sampleRate);
    juce::Reverb::Parameters p;
    p.roomSize = 0.82f;
    p.damping = 0.45f;
    p.width = 1.f;
    p.wetLevel = 1.f;
    p.dryLevel = 0.f;
    sendReverb.setParameters (p);
    sendReverb.reset();
    sendBus.setSize (2, (int) spec.maximumBlockSize, false, true, true);
    prepared = true;
}

void MfxRack::reset()
{
    for (auto& slot : slots)
        for (auto& fx : slot.effects)
            fx->reset();
    sendReverb.reset();
    sendBus.clear();
}

void MfxRack::processSlot (int index, juce::AudioBuffer<float>& buffer, const Mfx::Clock& clock,
                           const ModSources& mods, const TextureMacroOffsets& tex) noexcept
{
    auto& slot = slots[(size_t) index];
    const int n = buffer.getNumSamples();
    if (slot.on == nullptr || slot.effect == nullptr)
        return;

    const int effectIndex = juce::jlimit (0, (int) Mfx::Effect::count - 1, (int) slot.effect->load());
    if (effectIndex != slot.activeEffect)
    {
        if (slot.activeEffect >= 0)
            slot.effects[(size_t) slot.activeEffect]->reset();
        slot.effects[(size_t) effectIndex]->reset();
        slot.activeEffect = effectIndex;
    }

    const bool on = slot.on->load() > 0.5f;
    if (! on)
    {
        slotLevel[(size_t) index].store (0.f, std::memory_order_relaxed);
        return;
    }

    const auto effect = static_cast<Mfx::Effect> (effectIndex);
    const auto& desc = Mfx::descriptor (effect);

    // normalised -> +assign modulation -> real units (+ texture macro offsets)
    Mfx::Values values {};
    std::array<float, Mfx::kParamsPerSlot> norm {};
    for (int p = 0; p < Mfx::kParamsPerSlot; ++p)
        norm[(size_t) p] = slot.params[(size_t) p] != nullptr ? slot.params[(size_t) p]->load() : 0.f;

    for (int a = 0; a < Mfx::kNumAssigns; ++a)
    {
        const int target = desc.assignTargets[(size_t) a];
        if (target < 0 || target >= Mfx::kParamsPerSlot || slot.assignSrc[(size_t) a] == nullptr)
            continue;
        const auto src = static_cast<Mfx::ModSource> (juce::jlimit (0, (int) Mfx::ModSource::count - 1,
                                                                    (int) slot.assignSrc[(size_t) a]->load()));
        if (src == Mfx::ModSource::off)
            continue;
        const float amt = slot.assignAmt[(size_t) a] != nullptr ? slot.assignAmt[(size_t) a]->load() : 0.f; // -1..1
        norm[(size_t) target] = juce::jlimit (0.f, 1.f, norm[(size_t) target] + amt * mods.get (src));
    }

    if (effect == Mfx::Effect::grainCloud)
    {
        // Atmosphere macro destinations keep working on the slot that hosts the grain engine.
        norm[10] = juce::jlimit (0.f, 1.f, norm[10] + tex.mix);
        norm[0]  = juce::jlimit (0.f, 1.f, norm[0] + tex.grainSize);
        norm[1]  = juce::jlimit (0.f, 1.f, norm[1] + tex.density);
        norm[2]  = juce::jlimit (0.f, 1.f, norm[2] + tex.position);
        norm[3]  = juce::jlimit (0.f, 1.f, norm[3] + tex.pitchSpread);
        norm[8]  = juce::jlimit (0.f, 1.f, norm[8] + tex.smear);
        norm[7]  = juce::jlimit (0.f, 1.f, norm[7] + tex.width);
    }

    for (int p = 0; p < Mfx::kParamsPerSlot; ++p)
    {
        const auto& spec = desc.params[(size_t) p];
        values[(size_t) p] = spec.used() ? spec.denormalise (norm[(size_t) p]) : 0.f;
        liveValues[(size_t) index][(size_t) p].store (values[(size_t) p], std::memory_order_relaxed);
    }

    if (effect == Mfx::Effect::sweepFilter)
        static_cast<Mfx::SweepFilter*> (slot.effects[(size_t) effectIndex].get())->setEnvelopeLevel (mods.envelope);

    slot.effects[(size_t) effectIndex]->process (buffer, values, clock);

    // level + reverb send
    const float levelDb = slot.level != nullptr ? slot.level->load() : 0.f;
    slot.levelSmoothed.setTargetValue (juce::Decibels::decibelsToGain (levelDb));
    const float send = slot.send != nullptr ? juce::jlimit (0.f, 1.f, slot.send->load()) : 0.f;
    float* L = buffer.getWritePointer (0);
    float* R = buffer.getWritePointer (1);
    float* sL = sendBus.getWritePointer (0);
    float* sR = sendBus.getWritePointer (1);
    float peak = 0.f;
    for (int i = 0; i < n; ++i)
    {
        const float g = slot.levelSmoothed.getNextValue();
        L[i] *= g;
        R[i] *= g;
        sL[i] += L[i] * send;
        sR[i] += R[i] * send;
        peak = juce::jmax (peak, std::abs (L[i]), std::abs (R[i]));
    }
    slotLevel[(size_t) index].store (peak, std::memory_order_relaxed);
}

void MfxRack::process (juce::AudioBuffer<float>& buffer,
                       const Mfx::Clock& clock,
                       const ModSources& mods,
                       const TextureMacroOffsets& textureOffsets) noexcept
{
    if (! prepared || buffer.getNumChannels() < 2)
        return;
    const int n = buffer.getNumSamples();
    if (n <= 0 || n > sendBus.getNumSamples())
        return;

    sendBus.clear (0, n);
    bool anySend = false;
    for (int s = 0; s < Mfx::kNumSlots; ++s)
    {
        processSlot (s, buffer, clock, mods, textureOffsets);
        if (slots[(size_t) s].on != nullptr && slots[(size_t) s].on->load() > 0.5f
            && slots[(size_t) s].send != nullptr && slots[(size_t) s].send->load() > 0.001f)
            anySend = true;
    }

    if (anySend)
    {
        sendReverb.processStereo (sendBus.getWritePointer (0), sendBus.getWritePointer (1), n);
        buffer.addFrom (0, 0, sendBus, 0, 0, n, 0.6f);
        buffer.addFrom (1, 0, sendBus, 1, 0, n, 0.6f);
    }
}
