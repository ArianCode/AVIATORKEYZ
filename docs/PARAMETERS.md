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

---

## Flight Deck parameters (v5, 2026-09-05)

Appended after the performance-FX block. Total registered parameters: **262**.

| ID | Display Name | Type | Range / Choices | Default | Notes |
|---|---|---|---|---|---|
| `arp_on` | Arp On | Bool | off / on | off | ENGAGE pad. Host note on/offs feed the arp while on |
| `arp_mode` | Arp Mode | Choice | Up, Down, Up-Down, Random, As Played | Up | |
| `arp_rate` | Arp Rate | Choice | 1/4, 1/8, 1/16, 1/32 | 1/16 | Beat-synced to host PPQ when playing, free-run otherwise |
| `arp_feel` | Arp Feel | Choice | Straight, Triplet, Dotted | Straight | |
| `arp_octaves` | Arp Octaves | Int | 1–4 | 1 | |
| `arp_gate` | Arp Gate | Float | 0.05–1 | 0.7 | Note length as a fraction of the step |
| `arp_swing` | Arp Swing | Float | 0–1 | 0 | Odd steps delayed by up to half a step |
| `arp_humanize` | Arp Humanize | Float | 0–1 | 0 | ±12 ms timing, ±30 % velocity jitter |
| `arp_oct_spread` | Arp Octave Spread | Float | 0–1 | 0 | Chance per step of a ±1 octave jump |
| `arp_hold` | Arp Hold | Bool | off / on | off | Latch: chord keeps playing after release; next key starts a new chord |
| `arp_target` | Arp Target | Choice | Slices, Notes | Notes | SLICES: C1 = slice 0 of 16 across `src_start…src_end`, played at the sample root |
| `flip_window` | Flip Window | Choice | Phrase, Slice, Beat | Phrase | Window a live `reverse` flip is confined to (slice = 1/16 of source window, beat = one host beat) |
| `flip_snap` | Flip Snap | Choice | Off, 1/4, 1/8, 1/16 | 1/8 | A `reverse` edge is applied at the next grid boundary |

### Semantics changed in the same build

| ID | Change |
|---|---|
| `env_amp_decay` | Value is **seconds** (0–10). The processor converts to ms; previously it was fed as ms (max 10 ms). Sustain now applies when decay is 0 |
| `osc1_level`, `osc2_level` | Default 0.7 → **0**. The synth is an additive LAYER MIX layer; presets and pre-v5 projects load sampler-only |
| `filter_type` | Default LP → **HP**. `filter_enabled` still defaults off; the MAIN FILTER panel engages it on drag |
| `reverse` | Also flips sounding voices live (see `flip_window` / `flip_snap`) |

---

## MFX rack (v5, 2026-09-05)

Two effect slots in series (A → B) with a shared reverb send. Each slot owns **28 IDs**; adding an effect never adds IDs — only a descriptor in `Source/DSP/Mfx/MfxDescriptors.cpp` and a DSP class in `MfxEffects.cpp`. Total registered parameters: **318**.

| ID pattern | Type | Range | Default | Notes |
|---|---|---|---|---|
| `mfx{1,2}_on` | Bool | off / on | off | Slot power |
| `mfx{1,2}_effect` | Choice | Grain Cloud, Sweep Filter, Aviation Delay, Saturator, Stutter, Freeze, Bitcrusher, Compressor, Chorus, Space | A: Sweep Filter · B: Grain Cloud | Switching loads that effect's defaults into the bank. Index 2 was Tape Echo until 2026-09-10 |
| `mfx{1,2}_send` | Float | 0–1 | 0 | Send to the shared reverb return |
| `mfx{1,2}_level` | Float | −24…+6 dB | 0 | Slot output level |
| `mfx{1,2}_p01…p16` | Float | 0–1 (normalised) | per effect | Generic slots; the descriptor maps each to a musical range and label. Hosts see "MFX A Param 3" |
| `mfx{1,2}_asg{1..4}_src` | Choice | Off, Mod Wheel, Velocity, Aftertouch, LFO 1, Envelope, Macro 1, Note Pitch | Off | Modulation source for assign row n |
| `mfx{1,2}_asg{1..4}_amt` | Float | −1…+1 | 0 | Sens; the target slot is fixed per effect (see descriptor `assignTargets`) |

Randomiser locks and the reroll undo stack are plugin state (`mfxLocks1/2` properties), not parameters.

**Aviation Delay slot map** (effect 2; indices match `Mfx::AviationDelay::Param`):

| Slot | Label | Range | Default | Notes |
|---|---|---|---|---|
| p01 | Mode | Clean, Tape, Analog, BBD, Lo-Fi, Pitch, Reverse, Cloud | Clean | Changes the delay-line model; switching fades the wet out and back in (~24 ms) |
| p02 | Style | Single, Stereo, Ping-Pong, Dual, Ratio, Quad | Stereo | Routing of line A (L/mono) and line B (R) |
| p03 | Time | 10–2000 ms | 375 ms | Ignored when Sync ≠ Free |
| p04 | Sync | Free, 1/32, 1/16T, 1/16, 1/16D, 1/8T, 1/8, 1/8D, 1/4T, 1/4, 1/4D, 1/2, 1/1 | Free | Host tempo; clamped to 2 s |
| p05 | Feedback | 0–100 % | 35 % | Loop stays bounded at 100 % (saturating / soft-limited) |
| p06 | Mix | 0–100 % | 30 % | Dry stays at unity up to 50 %; wet reaches unity at 50 % |
| p07 | Diffusion | 0–100 % | 0 % | All-pass smear inside the loop; Cloud mode has a 35 % floor |
| p08 | Mod Depth | 0–100 % | 12 % | Wow/flutter (Tape), chorus (Clean/BBD), drift (Analog), detune (Pitch) |
| p09 | Mod Rate | 0.05–10 Hz | 0.6 Hz | |
| p10 | Lo Cut | 20–2000 Hz | 60 Hz | In the feedback loop |
| p11 | Hi Cut | 400–20000 Hz | 12 kHz | In the feedback loop; BBD/Lo-Fi filters also track their clock |
| p12 | Age | 0–100 % | 25 % | Saturation, noise and wear for the active mode |
| p13 | Duck | 0–100 % | 0 % | One-knob ducking of the wet by the dry signal |
| p14 | Pitch | −12…+12 st | +12 st | Pitch mode only |
| p15 | Ratio | 25–100 % | 75 % | Dual: R = T·ratio · Ratio: snapped musical ratio · Quad: tap spacing |
| p16 | Width | 0–100 % | 100 % | Mid/side width of the wet |

**Migration:** projects/presets with the old ATMOSPHERE layer (`ptex_on` = 1) that predate the rack load with slot B = Grain Cloud carrying the old values. The `ptex_*` IDs stay registered but are no longer processed; the Texture macro destinations now offset whichever slot hosts Grain Cloud. On the MAIN page the LAYER MIX "GRN" fader/toggle became "FX B" (slot B level/power) and the centre-strip WIDTH cell became REV SEND (slot A).


---

## Flip controls, envelope switch (v5, 2026-09-05 evening)

| ID | Type | Range / Choices | Default | Notes |
|---|---|---|---|---|
| `flip_mode` | Choice | Latch, Momentary | Latch | Applies to the lever click and the MIDI trigger |
| `flip_trigger_on` | Bool | off / on | off | When on, `flip_trigger_note` is consumed and throws the lever |
| `flip_trigger_note` | Int | 0–127 | 24 (C0) | |
| `env_enabled` | Bool | off / on | on | Off = flat envelope (instant attack, full sustain, 10 ms release) |
| `src_speed_snap` | Bool | off / on | on | SPEED lock: `src_speed` plays on 0.25 steps (×0.25 … ×4), macros included |

`src_root_note` is the key that plays the sample at its recorded pitch. Loads set it to the sample's own root (smpl chunk / key detection / preset `rootNote`) plus the user's re-root; that offset is stored as `rootShift` in preset XML and host state.

`src_playback_mode` = **Time Stretch** (index 3) now routes the sampler part of each note to the Signalsmith stretch voice (`src_speed` = time ratio 0.25–4, pitch preserved; `src_keytrack` = transpose). Index 4 (**Slice**) turns every key into one of 16 slice pads across `src_start…src_end`. Total registered parameters: **344**.
