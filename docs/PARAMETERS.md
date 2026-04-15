# AviatorKeyz — Parameter Reference

**CRITICAL:** Parameter IDs in this document are FIXED after 1.0 release.
They live in `Source/State/StateSchema.h`. Never rename, remove, or reorder them.
Hosts embed these IDs in saved projects and automation clips.

---

## Schema Version

`STATE_SCHEMA_VERSION = 1`

Bump this integer when the binary state format changes incompatibly. Add migration logic in `PluginProcessor::setStateInformation()`.

---

## Parameter Table

| # | ID | Display Name | Type | Range | Default | Smoothed | Unit | Notes |
|---|---|---|---|---|---|---|---|---|
| 1 | `input_gain` | Input Gain | Float | -24 to +12 | 0.0 | Yes (20ms) | dB | Pre-DSP input level |
| 2 | `output_gain` | Output Gain | Float | -24 to +12 | 0.0 | Yes (20ms) | dB | Post-DSP output level |
| 3 | `reverse` | Reverse | Bool | false / true | false | No | — | Reverses buffer read direction per voice |
| 4 | `glide_time` | Glide | Float | 0 to 500 | 0.0 | No* | ms | Portamento time; 0 = snap (off); skewed curve (0.35) |
| 5 | `smear` | Smear | Float | 0 to 1 | 0.0 | Yes (20ms) | — | Transient blur amount |
| 6 | `tone` | Tone | Float | -1 to +1 | 0.0 | Yes (20ms) | — | Tilt EQ; negative = dark/warm, positive = bright/clean |
| 7 | `reverb_amount` | Reverb | Float | 0 to 1 | 0.0 | Yes (20ms) | — | Reverb wet level |
| 8 | `reverb_size` | Reverb Size | Float | 0 to 1 | 0.5 | No | — | Reverb decay / room size |
| 9 | `stereo_width` | Width | Float | 0 to 2 | 1.0 | Yes (20ms) | — | M-S matrix; 0=mono, 1=stereo, 2=hyper-wide |

*`glide_time` is not smoothed via SmoothedValue — it is applied as a ramp duration in GlideEngine on each note-on event.

---

## Text Formatters

These are the human-readable display strings shown in host automation lanes and UI tooltips:

| ID | Example values |
|---|---|
| `input_gain` | `-6.0 dB`, `0.0 dB`, `+3.0 dB` |
| `output_gain` | `-6.0 dB`, `0.0 dB`, `+3.0 dB` |
| `reverse` | `Off`, `On` |
| `glide_time` | `Off` (at 0), `50 ms`, `200 ms`, `500 ms` |
| `smear` | `0%`, `50%`, `100%` |
| `tone` | `Neutral` (at 0), `25% Dark`, `80% Bright` |
| `reverb_amount` | `0%`, `50%`, `100%` |
| `reverb_size` | `0%`, `50%`, `100%` |
| `stereo_width` | `Mono` (at 0), `Stereo` (at 1.0), `150%` |

---

## NormalisableRange Notes

- `input_gain` / `output_gain`: linear, 0.01 dB step
- `glide_time`: skewed range, skew factor `0.35` — puts ~80% of the knob throw over 0–50 ms where musical glide lives
- All others: linear

---

## Adding Future Parameters

1. Add the ID constant to `Source/State/StateSchema.h` below the existing list
2. Add the parameter to `createParameterLayout()` in `PluginProcessor.cpp`
3. Add a row to this table
4. Add smoothing in `prepareToPlay` if applicable
5. Add text formatter
6. Do NOT bump `STATE_SCHEMA_VERSION` for additions with sane defaults — hosts will simply receive the default value
7. DO bump `STATE_SCHEMA_VERSION` if you remove or rescale an existing parameter

---

## Compatibility Notes

- Parameter IDs are embedded in FL Studio automation clips and project files
- Changing any ID in column 2 above will silently break all saved projects
- The only safe operations after 1.0 are: add new params at the end, or bump schema version with migration
