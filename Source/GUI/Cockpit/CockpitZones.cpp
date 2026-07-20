#include "CockpitZones.h"

namespace AviatorCockpit
{

static const KnobAnchor kAnchors[] =
{
    { "oh_hyd_main",       "reverb_amount", "reverbBypass", 0.14f, 0.10f, 0.014f, AnchorKind::toggle,        "HYD" },
    { "oh_lights_cabin",   "tone",          nullptr,        0.72f, 0.11f, 0.012f, AnchorKind::toggle,        "CAB" },
    { "oh_comm_vhf1",      nullptr,         "panic",        0.82f, 0.10f, 0.012f, AnchorKind::toggle,        "V1"  },
    { "oh_reverse",        "reverse",       nullptr,        0.20f, 0.18f, 0.012f, AnchorKind::toggle,        "REV" },
    { "shelf_cabin_lights","tone",          nullptr,        0.19f, 0.62f, 0.018f, AnchorKind::rotary,        "TONE"},
    { "shelf_altitude",    "reverb_amount", nullptr,        0.22f, 0.68f, 0.018f, AnchorKind::rotary,        "REV" },
    { "shelf_wings",       "stereo_width",  nullptr,        0.78f, 0.62f, 0.018f, AnchorKind::rotary,        "BRT" },
    { "shelf_turbulence",  "smear",         nullptr,        0.81f, 0.68f, 0.018f, AnchorKind::rotary,        "FILT"},
    { "shelf_input",       "input_gain",    nullptr,        0.75f, 0.74f, 0.016f, AnchorKind::rotary,        "IN"  },
    { "shelf_reverb_size", "reverb_size",   nullptr,        0.84f, 0.74f, 0.016f, AnchorKind::rotary,        "SIZE"},
    { "dash_atk",          "env_attack",    nullptr,        0.32f, 0.72f, 0.014f, AnchorKind::rotary,        "ATK" },
    { "dash_rel",          "env_release",   nullptr,        0.36f, 0.72f, 0.014f, AnchorKind::rotary,        "REL" },
    { "dash_glide",        "glide_time",    nullptr,        0.60f, 0.72f, 0.014f, AnchorKind::rotary,        "GLD" },
    { "dash_pan",          "pan",           nullptr,        0.64f, 0.72f, 0.014f, AnchorKind::rotary,        "PAN" },
    { "throttle_main",     "output_gain",   nullptr,        0.48f, 0.78f, 0.022f, AnchorKind::verticalLever, "VOL" },
    { "throttle_glide",    "glide_time",    nullptr,        0.52f, 0.78f, 0.022f, AnchorKind::verticalLever, "PITCH"},
    { "throttle_tone",     "tone",          nullptr,        0.44f, 0.78f, 0.020f, AnchorKind::verticalLever, "FLT" },
    { "emergency",         "smear",         "emergencyBurst",0.56f,0.82f, 0.016f, AnchorKind::guardedDome,   "EMER"},
    { "mode_taxi",         nullptr,         "flightMode",   0.46f, 0.86f, 0.010f, AnchorKind::pushButton,    "TAXI"},
    { "mode_cruise",       nullptr,         "flightMode",   0.49f, 0.86f, 0.010f, AnchorKind::pushButton,    "CRUISE"},
    { "mode_climb",        nullptr,         "flightMode",   0.52f, 0.86f, 0.010f, AnchorKind::pushButton,    "CLIMB"},
    { "mode_descent",      nullptr,         "flightMode",   0.55f, 0.86f, 0.010f, AnchorKind::pushButton,    "DESC"},
    { "fader_env",         "env_attack",    nullptr,        0.43f, 0.88f, 0.008f, AnchorKind::verticalLever, "ENV" },
    { "fader_lfo",         "glide_time",    nullptr,        0.46f, 0.88f, 0.008f, AnchorKind::verticalLever, "LFO" },
    { "fader_vel",         "stereo_width",  nullptr,        0.49f, 0.88f, 0.008f, AnchorKind::verticalLever, "VEL" },
    { "fader_exp",         "pan",           nullptr,        0.52f, 0.88f, 0.008f, AnchorKind::verticalLever, "EXP" },
    { "preset_prev",       nullptr,         "presetPrev",   0.12f, 0.44f, 0.012f, AnchorKind::pushButton,    "PREV"},
    { "preset_next",       nullptr,         "presetNext",   0.88f, 0.44f, 0.012f, AnchorKind::pushButton,    "NEXT"},
    { "library",           nullptr,         "library",      0.05f, 0.50f, 0.014f, AnchorKind::pushButton,    "LIB" },
};

const KnobAnchor* getKnobAnchors() noexcept { return kAnchors; }
int getKnobAnchorCount() noexcept { return (int) (sizeof (kAnchors) / sizeof (kAnchors[0])); }

juce::Rectangle<int> anchorBounds (juce::Rectangle<int> photoArea, const KnobAnchor& a) noexcept
{
    const int cx = photoArea.getX() + juce::roundToInt (a.x * (float) photoArea.getWidth());
    const int cy = photoArea.getY() + juce::roundToInt (a.y * (float) photoArea.getHeight());
    const int d  = juce::jmax (16, juce::roundToInt (a.r * 2.f * (float) photoArea.getWidth()));
    return { cx - d / 2, cy - d / 2, d, d };
}

} // namespace AviatorCockpit
