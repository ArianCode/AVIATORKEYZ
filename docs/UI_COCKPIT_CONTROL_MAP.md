# Cockpit Control Map

Photo-anchored crossworld UI: every control from the M0 shell and cockpit prototype has a home on a glass screen or physical anchor in [`Resources/UI/Cockpit/cockpit_photo_1x.jpg`](../Resources/UI/Cockpit/cockpit_photo_1x.jpg).

| Control | Param / action | Home |
|---------|----------------|------|
| Reverse | `reverse` | Overhead anchor `oh_reverse` |
| Glide | `glide_time` | Shelf `dash_glide`, lever `throttle_glide`, fader `fader_lfo` |
| Smear | `smear` | Shelf `shelf_turbulence`, emergency `emergency` |
| Tone | `tone` | Shelf `shelf_cabin_lights`, lever `throttle_tone` |
| Env attack | `env_attack` | Shelf `dash_atk`, fader `fader_env` |
| Env release | `env_release` | Shelf `dash_rel` |
| Reverb amount | `reverb_amount` | Shelf `shelf_altitude`, overhead `oh_hyd_main` |
| Reverb size | `reverb_size` | Shelf `shelf_reverb_size` |
| Stereo width | `stereo_width` | Shelf `shelf_wings`, fader `fader_vel` |
| Output gain | `output_gain` | Lever `throttle_main` |
| Pan | `pan` | Shelf `dash_pan`, fader `fader_exp` |
| Input gain | `input_gain` | Shelf `shelf_input` |
| Preset list | PresetManager | Left MFD glass |
| Preset search | UI-only | Left MFD glass |
| Preset prev/next | `presetPrev` / `presetNext` | Autopilot strip buttons |
| Library | `library` | Anchor `library` |
| ADSR curve | readout | `radarAdsr` glass |
| LFO sweep | `glide_time` + `smear` placeholder | `radarLfo` glass |
| FX meters | readout | Right MFD glass |
| Engine name | PresetManager | Left MFD `engineReadout` |
| Patch index | PresetManager | Autopilot `stripReadout` |
| Flight mode | `flightMode` (visual) | Mode buttons |
| CPU / SR / HUD | readout | Footer |

Anchors: [`Resources/UI/Cockpit/knobAnchors.json`](../Resources/UI/Cockpit/knobAnchors.json) and [`Source/GUI/Cockpit/CockpitZones.cpp`](../Source/GUI/Cockpit/CockpitZones.cpp).
