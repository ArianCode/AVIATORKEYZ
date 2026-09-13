#include "MfxDescriptors.h"

namespace Mfx
{
namespace
{
using P = ParamSpec;
constexpr P none {};

// helpers to keep the table readable
constexpr P lin (const char* label, float mn, float mx, float rmn, float rmx, float def, const char* unit = "", bool cautious = false)
{
    return P { label, mn, mx, rmn, rmx, def, unit, false, 0, cautious };
}
constexpr P lg (const char* label, float mn, float mx, float rmn, float rmx, float def, const char* unit = "", bool cautious = false)
{
    return P { label, mn, mx, rmn, rmx, def, unit, true, 0, cautious };
}
constexpr P chc (const char* label, int count, int rmn, int rmx, int def)
{
    return P { label, 0.f, (float) (count - 1), (float) rmn, (float) rmx, (float) def, "", false, count, false };
}

Preset preset (const char* name, std::initializer_list<float> values)
{
    Preset p;
    p.name = name;
    int i = 0;
    for (float v : values)
        if (i < kParamsPerSlot)
            p.values[(size_t) i++] = v;
    return p;
}

// -----------------------------------------------------------------------------
//  The registry. Slot indices are documented next to each effect so the
//  DSP classes and the assign targets can refer to them by number.
// -----------------------------------------------------------------------------
const std::array<Descriptor, (size_t) Effect::count>& table()
{
    static const std::array<Descriptor, (size_t) Effect::count> t = { {
        // 0 GRAIN CLOUD  (the former ATMOSPHERE engine)
        { Effect::grainCloud, "grain", "Grain Cloud", Category::texture,
          { lg  ("Size",     5.f, 500.f, 20.f, 220.f, 80.f, "ms"),           // 0
            lin ("Density",  0.f, 100.f, 20.f, 85.f, 50.f, "%"),             // 1
            lin ("Position", 0.f, 100.f, 0.f, 100.f, 35.f, "%"),             // 2
            lin ("Spread",   0.f, 100.f, 10.f, 80.f, 45.f, "%"),             // 3
            lin ("Pitch",  -24.f, 24.f, -12.f, 12.f, 0.f, "st"),             // 4
            lin ("Scan",  -100.f, 100.f, -60.f, 60.f, 20.f, "%"),            // 5
            lin ("Drift",    0.f, 100.f, 0.f, 55.f, 22.f, "%"),              // 6
            lin ("Width",    0.f, 100.f, 30.f, 100.f, 75.f, "%"),            // 7
            lin ("Smear",    0.f, 100.f, 0.f, 60.f, 15.f, "%"),              // 8
            chc ("Freeze",   2, 0, 0, 0),                                    // 9
            lin ("Mix",      0.f, 100.f, 20.f, 80.f, 45.f, "%"),             // 10
            none, none, none, none, none },
          { 10, 1, 2, 0 },
          { preset ("Soft Halo",    { 120, 45, 30, 35, 0, 15, 20, 80, 10, 0, 35 }),
            preset ("Shimmer Lift", { 60, 70, 40, 60, 12, 30, 25, 90, 20, 0, 50 }),
            preset ("Deep Drift",   { 260, 35, 60, 25, -12, -40, 50, 70, 30, 0, 55 }),
            preset ("Frozen Bed",   { 180, 80, 50, 40, 0, 0, 10, 100, 40, 1, 70 }) } },

        // 1 SWEEP FILTER
        { Effect::sweepFilter, "sweep", "Sweep Filter", Category::filter,
          { lg  ("Cutoff",   20.f, 18000.f, 180.f, 7000.f, 1200.f, "Hz"),    // 0
            lin ("Reso",     0.f, 100.f, 10.f, 62.f, 30.f, "%", true),       // 1
            chc ("Type",     4, 0, 1, 0),                                    // 2 LP HP BP Notch
            lin ("Drive",    0.f, 100.f, 0.f, 45.f, 18.f, "%", true),        // 3
            lg  ("LFO Rate", 0.05f, 10.f, 0.1f, 3.2f, 0.5f, "Hz"),           // 4
            lin ("LFO Depth",0.f, 100.f, 0.f, 70.f, 0.f, "%"),               // 5
            lin ("Env Amt", -100.f, 100.f, -40.f, 70.f, 0.f, "%"),           // 6
            lin ("Mix",      0.f, 100.f, 40.f, 100.f, 100.f, "%"),           // 7
            none, none, none, none, none, none, none, none },
          { 0, 1, 5, 7 },
          { preset ("Warm LP",     { 900, 25, 0, 10, 0.3f, 0, 0, 100 }),
            preset ("Auto Wah",    { 600, 55, 2, 20, 1.5f, 60, 30, 100 }),
            preset ("HP Riser",    { 3000, 35, 1, 5, 0.1f, 40, 0, 100 }),
            preset ("Notch Swirl", { 1400, 40, 3, 0, 0.25f, 70, 0, 100 }) } },

        // 2 AVIATION DELAY  (multi-model engine, see AviationDelay.h; slot order = AviationDelay::Param)
        { Effect::aviationDelay, "delay", "Aviation Delay", Category::delay,
          { chc ("Mode",      8, 0, 7, 0),                                     // 0 Clean Tape Analog BBD Lo-Fi Pitch Reverse Cloud
            chc ("Style",     6, 0, 5, 1),                                     // 1 Single Stereo Ping-Pong Dual Ratio Quad
            lg  ("Time",      10.f, 2000.f, 90.f, 900.f, 375.f, "ms"),         // 2
            chc ("Sync",      13, 0, 11, 0),                                   // 3 FREE 1/32 .. 1/1
            lin ("Feedback",  0.f, 100.f, 10.f, 70.f, 35.f, "%", true),        // 4
            lin ("Mix",       0.f, 100.f, 15.f, 55.f, 30.f, "%"),              // 5
            lin ("Diffusion", 0.f, 100.f, 0.f, 70.f, 0.f, "%"),                // 6
            lin ("Mod Depth", 0.f, 100.f, 0.f, 55.f, 12.f, "%"),               // 7 wow/flutter, chorus, pitch detune
            lg  ("Mod Rate",  0.05f, 10.f, 0.1f, 3.f, 0.6f, "Hz"),             // 8
            lg  ("Lo Cut",    20.f, 2000.f, 20.f, 400.f, 60.f, "Hz"),          // 9  in the feedback loop
            lg  ("Hi Cut",    400.f, 20000.f, 2500.f, 16000.f, 12000.f, "Hz"), // 10 in the feedback loop
            lin ("Age",       0.f, 100.f, 0.f, 70.f, 25.f, "%"),               // 11 saturation / noise / wear per mode
            lin ("Duck",      0.f, 100.f, 0.f, 60.f, 0.f, "%"),                // 12 one-knob ducking
            lin ("Pitch",    -12.f, 12.f, -12.f, 12.f, 12.f, "st"),            // 13 PITCH mode
            lin ("Ratio",     25.f, 100.f, 40.f, 100.f, 75.f, "%"),            // 14 DUAL / RATIO / QUAD
            lin ("Width",     0.f, 100.f, 50.f, 100.f, 100.f, "%") },          // 15
          { 5, 4, 6, 2 },
          //                         mode sty time sync  fb mix dif dep  rate  lo    hi   age duck pit rat wid
          { preset ("Clean Dotted",   { 0, 2, 375,  7,  38, 28,  0,  8, 0.4f,  80, 11000,  0, 20, 12, 75, 100 }),
            preset ("Dub Reel",       { 1, 1, 420,  9,  66, 34,  8, 28, 0.7f, 110,  7500, 55, 15, 12, 75,  90 }),
            preset ("Warm Analog",    { 2, 3, 330,  0,  45, 30,  0, 18, 0.35f, 90,  6500, 35, 25, 12, 75, 100 }),
            preset ("Bucket Brigade", { 3, 1, 300,  6,  55, 32,  0, 30, 0.9f, 100,  9000, 40, 10, 12, 75, 100 }),
            preset ("Sampler Grit",   { 4, 2, 250,  6,  45, 30,  0, 10, 2.5f, 150,  9000, 60, 20, 12, 75, 100 }),
            preset ("Octave Shimmer", { 5, 1, 480, 10,  60, 32, 45, 20, 0.4f, 150,  9000, 10, 35, 12, 75, 100 }),
            preset ("Reverse Swell",  { 6, 1, 600, 11,  40, 38, 25, 10, 0.3f, 120, 10000, 10,  0, 12, 75, 100 }),
            preset ("Cloud Bloom",    { 7, 1, 700,  0,  72, 40, 80, 35, 0.35f, 180, 8000, 20, 60, 12, 75, 100 }) } },

        // 3 SATURATOR
        { Effect::saturator, "sat", "Saturator", Category::drive,
          { lin ("Drive",  0.f, 100.f, 10.f, 58.f, 30.f, "%", true),         // 0
            lin ("Bias",  -50.f, 50.f, -20.f, 20.f, 0.f, "%"),               // 1
            lin ("Tone", -100.f, 100.f, -45.f, 45.f, 10.f, "%"),             // 2
            lin ("Out",   -24.f, 6.f, -8.f, 0.f, -3.f, "dB", true),          // 3
            lin ("Mix",     0.f, 100.f, 40.f, 100.f, 100.f, "%"),            // 4
            none, none, none, none, none, none, none, none, none, none, none },
          { 0, 2, 4, 3 },
          { preset ("Console Glue", { 18, 0, 5, -2, 100 }),
            preset ("Tube Push",    { 45, 10, 20, -5, 100 }),
            preset ("Fuzz Edge",    { 85, 25, -20, -9, 80 }),
            preset ("Parallel Heat",{ 70, 0, 30, -6, 45 }) } },

        // 4 STUTTER
        { Effect::stutter, "stutter", "Stutter", Category::performance,
          { chc ("Division", 4, 1, 3, 2),                                    // 0 1/4 1/8 1/16 1/32
            lin ("Gate",   0.f, 100.f, 20.f, 90.f, 55.f, "%"),               // 1
            lin ("Jitter", 0.f, 100.f, 0.f, 50.f, 15.f, "%"),                // 2
            lin ("Pitch", -12.f, 12.f, -5.f, 5.f, 0.f, "st"),                // 3
            lin ("Decay",  0.f, 100.f, 0.f, 60.f, 20.f, "%"),                // 4
            lin ("Mix",    0.f, 100.f, 50.f, 100.f, 100.f, "%"),             // 5
            none, none, none, none, none, none, none, none, none, none },
          { 5, 0, 1, 3 },
          { preset ("Eighth Chop", { 1, 60, 0, 0, 10, 100 }),
            preset ("Buzz Roll",   { 3, 80, 10, 0, 30, 100 }),
            preset ("Glitch Dice", { 2, 45, 60, 3, 25, 80 }),
            preset ("Tape Chatter",{ 2, 70, 20, -12, 50, 60 }) } },

        // 5 FREEZE
        { Effect::freeze, "freeze", "Freeze", Category::performance,
          { lin ("Hold",   0.f, 100.f, 30.f, 100.f, 100.f, "%"),             // 0 wet
            lin ("Smear",  0.f, 100.f, 10.f, 70.f, 40.f, "%"),               // 1
            lin ("Decay",  0.f, 100.f, 40.f, 100.f, 80.f, "%"),              // 2
            lin ("Pitch", -24.f, 24.f, -12.f, 12.f, 0.f, "st"),              // 3
            lin ("Width",  0.f, 100.f, 30.f, 100.f, 80.f, "%"),              // 4
            chc ("Engage", 2, 1, 1, 1),                                      // 5
            none, none, none, none, none, none, none, none, none, none },
          { 0, 1, 3, 2 },
          { preset ("Ice Pad",     { 100, 30, 90, 0, 90, 1 }),
            preset ("Octave Ghost",{ 80, 50, 70, 12, 100, 1 }),
            preset ("Sub Bloom",   { 70, 60, 85, -12, 60, 1 }),
            preset ("Brake",       { 100, 20, 40, 0, 50, 1 }) } },

        // 6 BITCRUSHER
        { Effect::bitcrusher, "crush", "Bitcrusher", Category::lofi,
          { lin ("Bits",   1.f, 16.f, 4.f, 14.f, 10.f, "bit"),               // 0
            lg  ("Rate",   500.f, 44100.f, 1500.f, 22000.f, 8000.f, "Hz"),   // 1
            lin ("Jitter", 0.f, 100.f, 0.f, 45.f, 10.f, "%"),                // 2
            lg  ("Filter", 200.f, 12000.f, 600.f, 8000.f, 4000.f, "Hz"),     // 3
            lin ("Mix",    0.f, 100.f, 25.f, 100.f, 70.f, "%"),              // 4
            none, none, none, none, none, none, none, none, none, none, none },
          { 1, 0, 3, 4 },
          { preset ("12-bit Warm",   { 12, 22050, 0, 6000, 100 }),
            preset ("SP Grit",       { 10, 11025, 5, 4500, 100 }),
            preset ("Game Cart",     { 6, 8000, 0, 3000, 100 }),
            preset ("Broken Radio",  { 5, 3000, 40, 2000, 65 }) } },

        // 7 COMPRESSOR
        { Effect::compressor, "comp", "Compressor", Category::dynamics,
          { lin ("Thresh", -40.f, 0.f, -24.f, -4.f, -14.f, "dB"),            // 0
            lin ("Ratio",   1.f, 20.f, 2.f, 8.f, 4.f, ":1"),                 // 1
            lg  ("Attack",  0.1f, 100.f, 1.f, 40.f, 12.f, "ms"),             // 2
            lg  ("Release", 5.f, 1000.f, 40.f, 400.f, 140.f, "ms"),          // 3
            lin ("Makeup",  0.f, 24.f, 0.f, 9.f, 4.f, "dB", true),           // 4
            lin ("Mix",     0.f, 100.f, 40.f, 100.f, 100.f, "%"),            // 5
            none, none, none, none, none, none, none, none, none, none },
          { 0, 1, 4, 5 },
          { preset ("Gentle Lift", { -12, 2.5f, 20, 200, 2, 100 }),
            preset ("Pump",        { -24, 6, 2, 80, 6, 100 }),
            preset ("Brick",       { -18, 12, 0.5f, 60, 5, 100 }),
            preset ("Parallel NY", { -28, 8, 1, 120, 9, 50 }) } },

        // 8 CHORUS
        { Effect::chorus, "chorus", "Chorus", Category::modulation,
          { lg  ("Rate",   0.05f, 8.f, 0.15f, 2.5f, 0.6f, "Hz"),             // 0
            lin ("Depth",  0.f, 100.f, 15.f, 70.f, 35.f, "%"),               // 1
            lin ("Centre", 1.f, 30.f, 5.f, 20.f, 8.f, "ms"),                 // 2
            lin ("Feedback",-95.f, 95.f, -40.f, 40.f, 0.f, "%", true),       // 3
            lin ("Mix",    0.f, 100.f, 25.f, 70.f, 45.f, "%"),               // 4
            none, none, none, none, none, none, none, none, none, none, none },
          { 1, 0, 4, 3 },
          { preset ("Silk",      { 0.4f, 25, 7, 0, 35 }),
            preset ("80s Wide",  { 0.9f, 55, 12, 10, 55 }),
            preset ("Flange Lite",{ 0.2f, 40, 2, 45, 45 }),
            preset ("Seasick",   { 3.5f, 80, 15, -20, 60 }) } },

        // 9 SPACE (send-style reverb inside the slot)
        { Effect::space, "space", "Space", Category::space,
          { lin ("Size",    0.f, 100.f, 30.f, 90.f, 60.f, "%"),              // 0
            lin ("Damp",    0.f, 100.f, 20.f, 70.f, 40.f, "%"),              // 1
            lin ("Width",   0.f, 100.f, 60.f, 100.f, 100.f, "%"),            // 2
            lg  ("Pre",     1.f, 200.f, 5.f, 60.f, 15.f, "ms"),              // 3
            lg  ("Lo Cut",  20.f, 1000.f, 60.f, 400.f, 150.f, "Hz"),         // 4
            lin ("Mix",     0.f, 100.f, 15.f, 60.f, 30.f, "%"),              // 5
            none, none, none, none, none, none, none, none, none, none },
          { 5, 0, 1, 3 },
          { preset ("Cabin",     { 35, 50, 80, 8, 200, 20 }),
            preset ("Hangar",    { 85, 30, 100, 30, 120, 40 }),
            preset ("Cloud Bed", { 95, 60, 100, 60, 250, 55 }),
            preset ("Tight Room",{ 20, 70, 60, 3, 100, 18 }) } },
    } };
    return t;
}
} // namespace

const Descriptor& descriptor (Effect e) noexcept
{
    const int i = juce::jlimit (0, (int) Effect::count - 1, (int) e);
    return table()[(size_t) i];
}

const char* categoryName (Category c) noexcept
{
    switch (c)
    {
        case Category::texture:     return "TEXTURE";
        case Category::filter:      return "FILTER";
        case Category::delay:       return "DELAY";
        case Category::drive:       return "DRIVE";
        case Category::performance: return "PERFORMANCE";
        case Category::lofi:        return "LO-FI";
        case Category::dynamics:    return "DYNAMICS";
        case Category::modulation:  return "MODULATION";
        case Category::space:       return "SPACE";
    }
    return "";
}

const char* modSourceName (ModSource s) noexcept
{
    switch (s)
    {
        case ModSource::off:        return "Off";
        case ModSource::modWheel:   return "Mod Wheel";
        case ModSource::velocity:   return "Velocity";
        case ModSource::aftertouch: return "Aftertouch";
        case ModSource::lfo1:       return "LFO 1";
        case ModSource::envelope:   return "Envelope";
        case ModSource::macro1:     return "Macro 1";
        case ModSource::notePitch:  return "Note Pitch";
        case ModSource::count:      break;
    }
    return "";
}

juce::StringArray effectNames()
{
    juce::StringArray names;
    for (const auto& d : table())
        names.add (d.name);
    return names;
}

juce::StringArray modSourceNames()
{
    juce::StringArray names;
    for (int i = 0; i < (int) ModSource::count; ++i)
        names.add (modSourceName (static_cast<ModSource> (i)));
    return names;
}

std::array<float, kParamsPerSlot> defaultsNormalised (Effect e) noexcept
{
    std::array<float, kParamsPerSlot> out {};
    const auto& d = descriptor (e);
    for (int i = 0; i < kParamsPerSlot; ++i)
    {
        const auto& p = d.params[(size_t) i];
        out[(size_t) i] = p.used() ? p.normalise (p.def) : 0.f;
    }
    return out;
}

std::array<float, kParamsPerSlot> presetNormalised (Effect e, int presetIndex) noexcept
{
    auto out = defaultsNormalised (e);
    const auto& d = descriptor (e);
    if (presetIndex < 0 || presetIndex >= kMaxPresets || d.presets[(size_t) presetIndex].name == nullptr)
        return out;
    const auto& pr = d.presets[(size_t) presetIndex];
    for (int i = 0; i < kParamsPerSlot; ++i)
    {
        const auto& p = d.params[(size_t) i];
        if (p.used())
            out[(size_t) i] = p.normalise (pr.values[(size_t) i]);
    }
    return out;
}

std::array<float, kParamsPerSlot> reroll (Effect e,
                                          const std::array<float, kParamsPerSlot>& current,
                                          uint32_t lockMask,
                                          float amount,
                                          juce::Random& rng) noexcept
{
    auto out = current;
    const auto& d = descriptor (e);
    amount = juce::jlimit (0.f, 1.f, amount);

    for (int i = 0; i < kParamsPerSlot; ++i)
    {
        const auto& p = d.params[(size_t) i];
        if (! p.used() || (lockMask & (1u << i)) != 0)
            continue;

        float lo = p.rollMin, hi = p.rollMax;
        if (p.cautious)
            hi = lo + (hi - lo) * 0.7f;
        float target = lo + rng.nextFloat() * (hi - lo);
        if (p.steps > 0)
            target = std::round (target);

        const float targetN = p.normalise (target);
        out[(size_t) i] = juce::jlimit (0.f, 1.f, current[(size_t) i] + (targetN - current[(size_t) i]) * amount);
    }
    return out;
}

juce::String valueText (const ParamSpec& spec, float v)
{
    if (! spec.used())
        return {};

    const juce::String unit (spec.unit);
    if (spec.steps > 0)
    {
        const int idx = (int) std::round (v);
        const juce::String label (spec.label);
        if (label == "Type")     { static const char* n[] = { "LP", "HP", "BP", "NOTCH" }; return n[juce::jlimit (0, 3, idx)]; }
        if (label == "Sync")     { static const char* n[] = { "FREE", "1/32", "1/16T", "1/16", "1/16D", "1/8T", "1/8", "1/8D", "1/4T", "1/4", "1/4D", "1/2", "1/1" }; return n[juce::jlimit (0, 12, idx)]; }
        if (label == "Mode")     { static const char* n[] = { "CLEAN", "TAPE", "ANALOG", "BBD", "LO-FI", "PITCH", "REVERSE", "CLOUD" }; return n[juce::jlimit (0, 7, idx)]; }
        if (label == "Style")    { static const char* n[] = { "SINGLE", "STEREO", "PING-PONG", "DUAL", "RATIO", "QUAD" }; return n[juce::jlimit (0, 5, idx)]; }
        if (label == "Division") { static const char* n[] = { "1/4", "1/8", "1/16", "1/32" }; return n[juce::jlimit (0, 3, idx)]; }
        if (label == "Freeze" || label == "Engage") return idx ? "ON" : "OFF";
        return juce::String (idx);
    }

    if (unit == "Hz")
    {
        if (v >= 1000.f) return juce::String (v / 1000.f, 1) + "k";
        return (v < 10.f ? juce::String (v, 2) : juce::String (juce::roundToInt (v))) + "Hz";
    }
    if (unit == "ms")
        return v >= 1000.f ? juce::String (v / 1000.f, 2) + "s"
                           : (v < 10.f ? juce::String (v, 1) : juce::String (juce::roundToInt (v))) + "ms";
    if (unit == "dB")  return juce::String (v, 1) + "dB";
    if (unit == "st")  return (v >= 0.f ? "+" : "") + juce::String (juce::roundToInt (v)) + "st";
    if (unit == ":1")  return juce::String (v, 1) + ":1";
    if (unit == "%")   return juce::String (juce::roundToInt (v)) + "%";
    if (unit == "bit") return juce::String (juce::roundToInt (v)) + "b";
    return juce::String (v, 2);
}

juce::String slotPrefix (int slot)
{
    return "mfx" + juce::String (juce::jlimit (0, kNumSlots - 1, slot) + 1) + "_";
}

juce::String paramId (int slot, int paramIndex)
{
    return slotPrefix (slot) + "p" + juce::String::formatted ("%02d", juce::jlimit (0, kParamsPerSlot - 1, paramIndex) + 1);
}

juce::String assignSourceId (int slot, int assignIndex)
{
    return slotPrefix (slot) + "asg" + juce::String (juce::jlimit (0, kNumAssigns - 1, assignIndex) + 1) + "_src";
}

juce::String assignAmountId (int slot, int assignIndex)
{
    return slotPrefix (slot) + "asg" + juce::String (juce::jlimit (0, kNumAssigns - 1, assignIndex) + 1) + "_amt";
}
} // namespace Mfx
