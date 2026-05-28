# AviatorKeyz — Cockpit UI Implementation Plan

> **Photo-anchored crossworld UI (implemented).** The shipped editor uses the cockpit photograph as ground truth (`Resources/UI/Cockpit/cockpit_photo_1x.jpg`), glass MFD/radar zones, and physical knob anchors — not animated CSS/JUCE scenery or a reconstructed shell PNG. See [`docs/UI_COCKPIT_CONTROL_MAP.md`](UI_COCKPIT_CONTROL_MAP.md), [`Source/GUI/Cockpit/`](../../Source/GUI/Cockpit/), and [`aviatorkeyz_cockpit_prototype.html`](../aviatorkeyz_cockpit_prototype.html). Legacy M0 shell sources live in [`Source/GUI/_legacy/`](../Source/GUI/_legacy/).

Version: 1.0
Owner: AviatorKeyz UI architecture
Target: C++20 / JUCE 8 / CMake / VST3 / Windows 10–11 / FL Studio first
Reference image: `Copilot_20260415_130658.png` (arctic-night cockpit, aurora windshield, twin tablets, throttle quadrant)

---

## 1. Executive Summary

The plugin UI will be rebuilt as a high-fidelity **luxury cockpit instrument**. The reference image is treated as the structural blueprint, not a moodboard: every major region of the painting maps 1:1 to a JUCE component, and we hold a contract of **≥ 90% visual accuracy** to the reference.

This is a **re-skin and structural pivot** on top of the existing M0 scaffold. We keep the disciplined parameter/state/DSP layers already in `Source/DSP/`, `Source/State/`, and `PluginProcessor`, and we replace the current piano-black/champagne synth shell in `Source/GUI/` with a new cockpit shell. Parameter IDs in `APVTS` are frozen and re-bound to aviation-themed UI placeholders — the **labels change, the IDs do not**, so existing automation lanes and presets remain compatible.

The cockpit is rendered as a **layered scene**: a baked, image-backed cockpit shell (overhead panel, side tablets, dashboard frame, throttle frame, windshield mask) on the bottom, animated windshield scenery on a mid layer, and interactive JUCE custom controls painted on top. Static layers are cached into `juce::Image` buffers; only lightweight overlays animate at a capped 30 FPS. This is how we keep cockpit density without paying CPU for it every frame.

Default editor size is **1600 × 900**, with proportional scaling via a single `scaleFactor` function. The UI is resizable inside FL Studio between **1280 × 720** and **2048 × 1152**, locked to a **16:9 aspect ratio** to preserve perspective and avoid the cockpit geometry stretching.

The cockpit-concept → plugin-feature mapping (Throttle = glide, Engine = sampler, Wings = width, Landing Gear = drop, Radar = browser, Flight Mode = preset category, Emergency Switch = creative FX, Cabin Lights = tone, Altitude = reverb, Turbulence = lo-fi/glitch) is the canonical functional contract. Everything else in the cockpit (overhead switches, dashboard readouts) is decorative or secondary — interactive where it serves a real plugin behavior, painted-but-inert where it doesn't, never invented just to fill space.

---

## 2. Visual Region Breakdown

The reference image decomposes into seven primary regions. Pixel ranges are normalized to a 1600 × 900 design canvas; runtime layout is proportional, not pixel-fixed.

| # | Region                | Approx. Bounds (px, design)         | Purpose                                                                       | Interactivity |
|---|-----------------------|-------------------------------------|-------------------------------------------------------------------------------|---------------|
| 1 | HeaderBar             | x 0–1600, y 0–28                    | Brand strip, preset name, scale handle. Sits above the cockpit frame.         | Light         |
| 2 | OverheadPanel         | x 0–1600, y 28–215                  | Dense overhead switches, dials, illuminated tiles. Decorative + secondary.    | Light         |
| 3 | WindshieldScenePanel  | x 175–1425, y 215–520               | Arctic runway, aurora, mountains, glass reflection. Animated hero scene.      | None (visual) |
| 4 | FlightDashboardPanel  | x 175–1425, y 520–720               | 5 cockpit screens + numeric readouts + small gauges. Main instrumentation.    | High          |
| 5 | LeftPresetTablet      | x 0–175, y 320–840                  | "ARCTIC WAVE" tablet — preset browser / search / categories.                  | High          |
| 6 | RightControlTablet    | x 1425–1600, y 320–840              | "FROZEN STRINGS" tablet — sound-shape controls (cabin lights / altitude / etc.) | High        |
| 7 | CenterConsolePanel    | x 400–1200, y 720–880               | Throttle quadrant, twin yokes, button matrix, emergency switch.               | High (hero)   |
| — | FooterBar             | x 0–1600, y 880–900                 | Status (CPU, voices, sample rate, MIDI activity). Minimal chrome.             | Low           |

A region boundary is allowed to be "lossy" by up to ±2% of width/height, which is enough wiggle to round to whole pixels at any scale without breaking the painted cockpit alignment.

### 2.1 Region notes

- The **overhead panel** in the image is densely packed with hundreds of micro-elements. We do not paint every one in JUCE. We bake the overhead into a high-resolution PNG (one per supported scale tier) and overlay a small number of *real* interactive switches (~12) on top.
- The **windshield** is the emotional centerpiece. It must stay close to the image. We render the cockpit-pillar mask + dashboard top edge as part of the cockpit shell so the scene only fills the cutout shape — no fighting the cockpit silhouette with rectangular art.
- The **dashboard** has five visible screens in the reference (two outer flight displays, two radar/PFD screens, one center small screen). These become real plugin views: Engine display, two radar/browser-related screens, an FX/spatial visualizer, and a center status screen.
- The **center console** is the most tactile zone. It must look and feel like the most premium part of the UI — heavier glow, more detail, more shadow, more weight in the cursor under the throttle.

---

## 3. Component Hierarchy

```
PluginEditor
└── MainPanel                               // root, owns layout + scale
    ├── CockpitBackground                   // static, image-backed
    ├── OverheadPanel                       // baked PNG + ~12 IlluminatedButtons
    ├── WindshieldScenePanel                // mid layer, animated scenery
    │   ├── ArcticBackgroundLayer           // cached
    │   ├── AuroraLayer                     // animated, low FPS
    │   ├── RunwayLightsLayer               // animated, cached strobe table
    │   └── GlassReflectionLayer            // cached + slow drift
    ├── FlightDashboardPanel                // baked frame + child screens
    │   ├── EngineDisplay                   // main sound source
    │   ├── RadarBrowserDisplay             // browser readout
    │   ├── FxSpatialVisualizer             // FX/spatial telemetry
    │   ├── CenterStatusDisplay             // voices/CPU/MIDI activity
    │   └── DashboardGaugeStrip             // width / tone / reverb / turbulence / output gauges
    ├── LeftPresetTablet                    // RadarBrowser host
    │   └── RadarBrowser                    // preset browser + search + categories
    ├── RightControlTablet
    │   ├── CabinLightsControl
    │   ├── AltitudeControl
    │   ├── WingWidthControl
    │   ├── TurbulenceControl
    │   └── LandingGearControl
    ├── CenterConsolePanel                  // hero interactive zone
    │   ├── ThrottleControl                 // primary lever
    │   ├── EmergencySwitch                 // guarded creative FX
    │   ├── YokeOrnamentLeft                // decorative
    │   ├── YokeOrnamentRight               // decorative
    │   ├── ButtonMatrix                    // FlightModeSelector source
    │   └── FaderStrip                      // sub-modulation faders
    ├── HeaderBar
    │   ├── BrandLogo
    │   ├── PresetNameDisplay
    │   └── EditorScaleHandle
    └── FooterBar
        ├── CpuMeterReadout
        ├── VoicesReadout
        ├── SampleRateReadout
        └── MidiActivityIndicator
```

### 3.1 Component responsibilities

For each component below: responsibility, visual style, controls, placeholder mapping, paint strategy, scaling behavior, performance risk, build priority.

#### MainPanel
- **Responsibility:** Owns layout, scale factor, scene compositing order, repaint coordination.
- **Visual style:** Invisible container.
- **Controls:** None.
- **Paint:** Does not paint; defers to children.
- **Scaling:** Computes `scale = width / 1600.f` once per resize, propagates to children via `setBoundsScaled` helpers.
- **Performance risk:** Low.
- **Priority:** P1 (build first).

#### CockpitBackground
- **Responsibility:** The dark cockpit-shell silhouette — pillars, dashboard frame edges, throttle housing, tablet bezels. Holds the cockpit identity even when overlays are dim.
- **Visual style:** Deep cockpit black + charcoal metal + subtle electric-blue rim lighting.
- **Controls:** None.
- **Paint:** Image-backed (cached `juce::Image`), regenerated only on resize.
- **Scaling:** Re-scales source image via `Graphics::drawImage` with `RectanglePlacement::fillDestination` (no aspect distortion because the editor itself is aspect-locked).
- **Performance risk:** Low (single image blit per frame).
- **Priority:** P1.

#### OverheadPanel
- **Responsibility:** Top decorative-but-rich switch panel; secondary plugin actions live here.
- **Visual style:** High-density baked PNG of the overhead from the reference, plus ~12 interactive `IlluminatedButton`s placed on the panel.
- **Controls:** Hydraulics on/off (acts as global FX bypass mirror), Lights (UI dim/brightness), Comm (panic / all-notes-off), and 9 reserved-future switches that paint as illuminated but only emit a click animation.
- **Placeholder mapping:** No primary mapping; supports the cockpit illusion.
- **Paint:** Baked image bottom layer + JUCE switch overlays.
- **Scaling:** Image scales; switch hit regions are stored as normalized rects.
- **Performance risk:** Low if image is cached; high if we tried to paint primitives.
- **Priority:** P2.

#### WindshieldScenePanel
- **Responsibility:** Hero scenery; emotional anchor of the plugin.
- **Visual style:** Arctic mountains, runway centerline, blue-violet aurora, soft glass reflections.
- **Controls:** None.
- **Paint:** Three layers — `ArcticBackgroundLayer` (cached), `AuroraLayer` (low-FPS animation), `RunwayLightsLayer` (cached strobe table sampled by timer), `GlassReflectionLayer` (drifts slowly).
- **Scaling:** Background image scales; aurora and runway lights are sampled from cached gradient tables, then composited.
- **Performance risk:** Medium — must not redraw the whole scene per frame. See §9.
- **Priority:** P2 (skeleton in P1, animation in P4).

#### FlightDashboardPanel
- **Responsibility:** The cockpit dashboard frame plus the five interactive screens and gauges.
- **Visual style:** Black instrument panel, electric-blue screen content, white/cyan numeric readouts.
- **Controls:** Hosts `EngineDisplay`, `RadarBrowserDisplay`, `FxSpatialVisualizer`, `CenterStatusDisplay`, `DashboardGaugeStrip`.
- **Placeholder mapping:** Engine = main sampler; gauges echo right-tablet controls so the user sees live values centrally.
- **Paint:** Image-backed frame + child screens that paint themselves.
- **Scaling:** Frame image scales; child screens use normalized child bounds.
- **Performance risk:** Medium — keep each screen at ≤ 30 FPS and only repaint dirty rects.
- **Priority:** P1 (frame) / P3 (screens).

#### LeftPresetTablet → RadarBrowser
- **Responsibility:** Preset browsing, search, categories, favorites, save/load.
- **Visual style:** Cockpit-mounted touchscreen, dark glass, electric-blue cursor, radar sweep behind the list.
- **Controls:** Search field, scrolling list, category chips, prev/next, favorite, save, load, mode toggles.
- **Placeholder mapping:** **Radar**.
- **Paint:** Tablet bezel image + custom-painted radar sweep + JUCE `ListBox`/`TextEditor` styled via `LookAndFeel_AviatorBlue`.
- **Scaling:** Item height and font size scale; list count remains the same.
- **Performance risk:** Low — list repaints only on scroll/selection; sweep is cached.
- **Priority:** P2.

#### RightControlTablet
- **Responsibility:** Sound shaping controls grouped by aviation metaphor.
- **Visual style:** Mirror of left tablet visually, but full of knobs/dials/multi-mode controls.
- **Controls:** `CabinLightsControl`, `AltitudeControl`, `WingWidthControl`, `TurbulenceControl`, `LandingGearControl`. Each is a labeled group on the tablet "screen."
- **Placeholder mapping:** Cabin Lights, Altitude, Wings, Turbulence, Landing Gear (see §5).
- **Paint:** Tablet bezel + per-control custom paint.
- **Scaling:** Knob diameter ∝ scale; layout uses a 5-row vertical grid.
- **Performance risk:** Low if knobs cache their dial face.
- **Priority:** P2.

#### CenterConsolePanel
- **Responsibility:** The most tactile and premium zone. Throttle, emergency switch, button matrix, faders, decorative yokes.
- **Visual style:** Heaviest metal feel, brightest electric-blue accents, deepest shadows, optional champagne accent only on the throttle handle (brand mark).
- **Controls:** `ThrottleControl`, `EmergencySwitch`, `ButtonMatrix` (mode buttons / quick actions), `FaderStrip` (small modulation faders), `YokeOrnamentLeft/Right` (decorative).
- **Placeholder mapping:** Throttle = glide, Emergency = creative FX, Button Matrix = FlightMode/preset categories, Faders = related modulation.
- **Paint:** Image-backed housing + heavy custom paint on the throttle. Yokes are baked-in to the housing image.
- **Scaling:** Throttle physical length ∝ scale; button hit regions normalized.
- **Performance risk:** Medium — throttle handle drag must repaint just the throttle bounding rect.
- **Priority:** P1 (housing + throttle) / P3 (matrix + faders) / P5 (emergency-switch polish).

#### HeaderBar / FooterBar
- **Responsibility:** Plugin identity and runtime status.
- **Visual style:** Minimal cockpit chrome, ice-blue text, monospace numerics.
- **Controls:** Brand + preset name in the header; CPU, voices, sample rate, MIDI activity in the footer.
- **Paint:** Lightweight.
- **Scaling:** Font size ∝ scale.
- **Performance risk:** None.
- **Priority:** P1.

---

## 4. Control Mapping (Cockpit Concept → UI Region → Control)

| Cockpit Concept  | Plugin Role                          | UI Region                              | Suggested Control Type                |
|------------------|--------------------------------------|----------------------------------------|----------------------------------------|
| Throttle         | Glide / pitch movement (macro)       | Center Console                         | Vertical lever (`ThrottleControl`)     |
| Engine           | Main sound source / sampler          | Flight Dashboard (left/center screen)  | `EngineDisplay` module                 |
| Wings            | Stereo width / spread                | Right Tablet + Dashboard gauge mirror  | `WingWidthControl` (knob with spread arc) |
| Landing Gear     | Drop / tape-stop / slowdown          | Right Tablet                           | `LandingGearControl` (guarded staged lever) |
| Radar            | Preset browser / sound search        | Left Tablet                            | `RadarBrowser` (list + sweep)          |
| Flight Mode      | Preset category / performance mode   | Center Console button matrix           | `FlightModeSelector` (latching tabs)   |
| Emergency Switch | Big creative FX trigger              | Center Console (right of throttle)     | `EmergencySwitch` (guarded illuminated) |
| Cabin Lights     | Tone / brightness                    | Right Tablet                           | `CabinLightsControl` (knob)            |
| Altitude         | Reverb / space                       | Right Tablet                           | `AltitudeControl` (vertical depth dial) |
| Turbulence       | Lo-fi / wobble / stutter / glitch    | Right Tablet                           | `TurbulenceControl` (multi-mode macro) |

### 4.1 Secondary controls (decorative-functional)

| Cockpit element        | Plugin role                            | Where                  | Notes                                |
|------------------------|----------------------------------------|------------------------|--------------------------------------|
| Overhead "HYDRAULICS"  | Mirror of plugin bypass                | OverheadPanel          | Read-only; reflects host bypass state |
| Overhead "LIGHTS"      | UI dim / brightness                    | OverheadPanel          | Local UI setting; not host-automated  |
| Overhead "COMM"        | Panic / all-notes-off                  | OverheadPanel          | Sends MIDI panic                     |
| Dashboard numeric strip | Live readouts (BPM, voices, SR, etc.) | Flight Dashboard top   | Read-only animated                   |

---

## 5. Parameter / Placeholder Mapping

This is the **stable contract** between the UI placeholders and `APVTS` parameter IDs. Parameter IDs are **frozen** at first ship (project rule 7) and must not change even if the visual label evolves.

| UI Placeholder       | Parameter ID (frozen)   | Type    | Range          | Default | Smoothing | Notes |
|----------------------|-------------------------|---------|----------------|---------|-----------|-------|
| Throttle             | `glide`                 | Float   | 0.0 – 1.0      | 0.0     | 30 ms     | Maps to existing `GlideEngine`. |
| Engine: Source       | `engineSource`          | Choice  | 0..N-1         | 0       | n/a       | Switches active `SamplerEngine` source bank. |
| Engine: Voice Macro  | `engineVoiceMacro`      | Float   | 0.0 – 1.0      | 0.5     | 20 ms     | Macro over voice/round-robin/layer blend. |
| Wings                | `width`                 | Float   | 0.0 – 1.0      | 0.5     | 15 ms     | Stereo width. |
| Landing Gear         | `landingGear`           | Float   | 0.0 – 1.0      | 0.0     | 50 ms     | Replaces previous "reverse/drop" placeholder; staged. |
| Landing Gear Mode    | `landingGearMode`       | Choice  | Drop / Tape / Slow | Tape | n/a       | Discrete mode. |
| Radar: Search        | (UI-only)               | —       | —              | —       | —         | Not host-automated. |
| Flight Mode          | `flightMode`            | Choice  | Categories enum | 0      | n/a       | Doubles as preset filter + performance mode. |
| Emergency Switch     | `emergencyFx`           | Float   | 0.0 – 1.0      | 0.0     | 5 ms      | One-shot when crossing threshold; momentary illumination. |
| Cabin Lights         | `tone`                  | Float   | 0.0 – 1.0      | 0.5     | 15 ms     | Tone/brightness. |
| Altitude             | `reverbAmount`          | Float   | 0.0 – 1.0      | 0.2     | 25 ms     | Reverb mix. |
| Altitude Depth       | `reverbDepth`           | Float   | 0.0 – 1.0      | 0.5     | 25 ms     | Reverb tail/size. |
| Turbulence           | `turbulence`            | Float   | 0.0 – 1.0      | 0.0     | 20 ms     | Macro for current turbulence mode. |
| Turbulence Mode      | `turbulenceMode`        | Choice  | Lofi/Wobble/Stutter/Glitch | Lofi | n/a    | Discrete mode. |
| Master Output        | `output`                | Float   | -inf..+6 dB    | 0 dB    | 20 ms     | Footer + dashboard gauge. |

**Compatibility note.** If any of these IDs already exist in `PluginProcessor.cpp` with different semantics (e.g., the current scaffold may have `reverse`, `smear`, `reverbAmount`, etc.), we **retain the existing IDs** and only update the visible label. If we need a *new* semantic that does not yet exist, we add a new ID — we never recycle an old ID with a new meaning. A migration table will live in `docs/PARAMETERS.md` updates with each change.

---

## 6. Design Token Plan

Tokens live in `Source/GUI/AviatorTokens.h` (new file, sibling to existing `DesignTokens.h`). The existing piano/champagne tokens stay in place until the cockpit migration is complete, then `DesignTokens.h` becomes a thin alias for `AviatorTokens` and old call sites compile unchanged.

```cpp
// Source/GUI/AviatorTokens.h
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

namespace AviatorTokens
{
    // --- Cockpit surfaces ---
    inline juce::Colour cockpitBlack()   { return juce::Colour (0xff05070b); }
    inline juce::Colour panelMetal()     { return juce::Colour (0xff0d1118); }
    inline juce::Colour panelMetalHi()   { return juce::Colour (0xff141a25); }
    inline juce::Colour midnightNavy()   { return juce::Colour (0xff0a1424); }
    inline juce::Colour glassDark()      { return juce::Colour (0xff0b1018); }

    // --- Illumination ---
    inline juce::Colour electricBlue()   { return juce::Colour (0xff3ea6ff); }
    inline juce::Colour iceBlue()        { return juce::Colour (0xff8fd6ff); }
    inline juce::Colour iceGlow()        { return juce::Colour (0xffbfe9ff); }
    inline juce::Colour auroraGreen()    { return juce::Colour (0xff6ee7c8); }
    inline juce::Colour warningAmber()   { return juce::Colour (0xffffb14a); }
    inline juce::Colour emergencyRed()   { return juce::Colour (0xffff5a4d); }

    // --- Text ---
    inline juce::Colour textPrimary()    { return juce::Colour (0xffe6f3ff); }
    inline juce::Colour textSecondary()  { return juce::Colour (0xff7d97b3); }
    inline juce::Colour textMuted()      { return juce::Colour (0xff3d4a5e); }

    // --- Brand accent (used sparingly) ---
    inline juce::Colour champagneGold()  { return juce::Colour (0xffd4bc86); }

    // --- Layout (design canvas: 1600 x 900) ---
    static constexpr int kDesignWidth   = 1600;
    static constexpr int kDesignHeight  = 900;
    static constexpr float kAspect      = (float) kDesignWidth / (float) kDesignHeight;

    static constexpr int kHeaderH         = 28;
    static constexpr int kOverheadH       = 187;
    static constexpr int kWindshieldH     = 305;
    static constexpr int kDashboardH      = 200;
    static constexpr int kConsoleH        = 160;
    static constexpr int kFooterH         = 20;

    static constexpr int kSideTabletW     = 175;

    // --- Geometry tokens ---
    static constexpr float kCornerRadius   = 10.f;
    static constexpr float kBezelThickness = 2.f;
    static constexpr float kGlowSpread     = 12.f;
    static constexpr float kPanelInset     = 6.f;

    // --- Animation ---
    static constexpr int kSceneryFps     = 30;
    static constexpr int kRunwayBlinkMs  = 1100;
    static constexpr int kAuroraDriftMs  = 12000;
    static constexpr int kEmergencyFlashMs = 220;

    // --- Scale helpers ---
    inline float scaleFactor (int currentWidth) noexcept
    {
        return juce::jlimit (0.8f, 1.28f, (float) currentWidth / (float) kDesignWidth);
    }

    inline int scaled (int designPx, float s) noexcept
    {
        return juce::roundToInt ((float) designPx * s);
    }

    inline juce::Rectangle<int> scaledRect (juce::Rectangle<int> designRect, float s)
    {
        return { scaled (designRect.getX(), s),
                 scaled (designRect.getY(), s),
                 scaled (designRect.getWidth(), s),
                 scaled (designRect.getHeight(), s) };
    }

    // --- Fonts ---
    inline juce::Font hud (float size)     { return juce::Font (juce::FontOptions ("DM Mono", size, juce::Font::plain)); }
    inline juce::Font label (float size)   { return juce::Font (juce::FontOptions ("Inter", size, juce::Font::plain)); }
    inline juce::Font brand (float size)   { return juce::Font (juce::FontOptions ("Inter", size, juce::Font::bold)); }
}
```

The above gives every component one source of truth for color, geometry, layout, and animation timing.

---

## 7. Custom Control Plan

All controls inherit from a thin `AviatorControl` base that:
- Holds a `juce::Image` face cache regenerated on `resized()` only.
- Repaints only when value/state changes (no per-frame repaint).
- Pulls colors and geometry from `AviatorTokens`.
- Routes value changes through `juce::AudioProcessorValueTreeState::SliderAttachment` / `ButtonAttachment` / `ComboBoxAttachment`.

For each control, the following spec applies.

### ThrottleControl
- **Inspired by:** Center throttle levers.
- **Maps to:** `glide` (0.0 – 1.0).
- **Form:** Vertical lever with metal handle; champagne-gold band on the grip (brand accent).
- **Interaction:** Vertical drag, fine modifier with Shift, right-click → reset to default.
- **Paint:** Cached lever shaft and slot; handle is painted dynamically; glow around handle = current value.
- **Hit area:** Slightly larger than visual handle for easier grabbing inside FL Studio.
- **Risk:** Mouse drag latency in FL Studio mixer view — must throttle to ≤ 60 Hz repaint.

### EngineModule (`EngineDisplay`)
- **Inspired by:** Left dashboard flight screen.
- **Maps to:** `engineSource` + `engineVoiceMacro`.
- **Form:** Black instrument screen, electric-blue readout: current source name, source category, voice macro arc, mini waveform.
- **Interaction:** Click source name to cycle, click waveform to scrub macro.
- **Paint:** Background cached; only readout text + macro arc repaint on change.

### WingWidthControl
- **Inspired by:** Wing balance / spread.
- **Maps to:** `width` (0.0 – 1.0).
- **Form:** Circular knob with two wing-tip indicators that spread further apart as value increases.
- **Interaction:** Rotary drag; double-click resets.
- **Paint:** Knob face cached; only wing-tip overlays redraw.

### LandingGearControl
- **Inspired by:** Landing gear lever.
- **Maps to:** `landingGear` + `landingGearMode`.
- **Form:** Staged vertical lever with 3 detents (off / mid / full) + small mode selector below.
- **Interaction:** Drag through detents; mode selector is a tiny segmented control.
- **Paint:** Cached lever housing; stage indicators light when crossed.

### RadarBrowser
- **Inspired by:** Radar screen + cockpit tablet.
- **Maps to:** Preset browser, search, category filter, favorite/save/load.
- **Form:** Radar sweep behind a scrollable preset list; category chips at the top; search at the bottom.
- **Interaction:** Standard list controls; sweep animates at a fixed slow rate independent of selection.

### FlightModeSelector
- **Inspired by:** Center console mode buttons.
- **Maps to:** `flightMode`.
- **Form:** A 4–6 cell illuminated button row in the button matrix. Latching, mutually exclusive.
- **Interaction:** Click to select; current selection glows iceBlue.

### EmergencySwitch
- **Inspired by:** Guarded emergency switch on the console.
- **Maps to:** `emergencyFx`.
- **Form:** Red-amber illuminated dome under a hinged guard. Guard must be flipped open before press; press triggers a flash.
- **Interaction:** Click guard to open, click dome to trigger. Re-locks after 2 s of inactivity.
- **Paint:** Guard rotation animated over ~220 ms; dome flash uses precomputed gradient stops.

### CabinLightsControl
- **Inspired by:** Cabin lighting knob.
- **Maps to:** `tone` (brightness/tilt EQ).
- **Form:** Soft round knob with a halo whose intensity follows the value.
- **Paint:** Halo gradient cached at 8 brightness steps; value selects step + interpolates.

### AltitudeControl
- **Inspired by:** Altimeter / depth.
- **Maps to:** `reverbAmount` + `reverbDepth`.
- **Form:** Vertical "altitude bar" with a depth dial behind it. Bar rises as reverb mix increases; dial sets tail size.
- **Paint:** Bar + dial cached separately; only the bar's filled portion repaints on change.

### TurbulenceControl
- **Inspired by:** Disturbance/wobble.
- **Maps to:** `turbulence` + `turbulenceMode`.
- **Form:** Knob with an unstable, jittering ring that animates only when value > 0. Mode selector picks Lo-fi/Wobble/Stutter/Glitch.
- **Paint:** Jitter ring is procedural; off when value == 0 (zero CPU cost at rest).

### Reusable supporting controls
- `PrecisionKnob` (already exists; restyle to AviatorTokens).
- `HorizontalFader` (already exists; restyle).
- `IlluminatedButton` (new; replaces `ReverseToggle`).
- `AircraftToggle` (new; switch with click animation).
- `ScreenTab` (new; tab styled like a cockpit screen tab).
- `MeterDisplay` (new; for gauges).

---

## 8. Scaling Strategy

- **Design canvas:** 1600 × 900.
- **Default editor size:** 1600 × 900.
- **Min size:** 1280 × 720.
- **Max size:** 2048 × 1152.
- **Aspect ratio:** Locked to 16:9 (`AudioProcessorEditor::setResizable (true, true)` + `setFixedAspectRatio (16.f / 9.f)` via a `juce::ComponentBoundsConstrainer`). Locking the aspect ratio is **non-negotiable**; without it, the cockpit perspective collapses.
- **Scale factor:** `s = clamp (width / 1600.f, 0.8f, 1.28f)`. Below 0.8 the cockpit labels become illegible; above 1.28 the image-baked overhead panel starts to look soft. If we ever support 4K-tier scaling we add a `@2x` PNG set, not more scale headroom.
- **Layout:** All bounds are computed in design pixels, then translated through `AviatorTokens::scaled(...)`. Hardcoded pixel values are forbidden outside of `AviatorTokens.h`.
- **Persistence:** Editor width is stored in the plugin state so the host restores it on session reload.
- **FL Studio quirk:** FL allows the user to resize wrapper windows independent of the editor's preferred size. We handle this by *always* honoring our aspect ratio internally and letting FL paint a background bar if its wrapper is wrong-shaped. Document this in `known-host-issues.md`.

---

## 9. Performance Strategy

Cockpit density is the main risk. The plan: **bake the static, animate only what must move, repaint only what changed.**

### 9.1 Layering and caching

1. **Cockpit shell cache (`juce::Image`)**: rebuilt only on `resized()`. Contains overhead panel art, dashboard frame, tablet bezels, console housing, yokes. Single blit per frame.
2. **Windshield static cache**: arctic mountains + runway base + sky gradient. Rebuilt only on resize.
3. **Windshield animated overlay**: aurora + runway-light blink. Repainted at 30 FPS max via a single `juce::Timer` driven from `MainPanel`, not per-component timers.
4. **Custom controls**: each control caches its face into a small `juce::Image`. Repaints only on value change or hover/focus state change.
5. **Numeric readouts**: repaint only when the underlying value changes by ≥ display threshold (e.g., voices change, CPU readout updates 4 Hz).

### 9.2 Forbidden patterns

- Painting full-window gradients inside a 60 FPS loop.
- Allocating in `paint()` (no `new`, no big STL containers).
- One timer per control. A single scenery timer fans out to layers via dirty flags.
- Calling `repaint()` with no rect argument when a sub-rect suffices.

### 9.3 Budgets

- Idle CPU (no MIDI, no UI motion): < 0.5% on a modern desktop.
- Scenery animation active: < 2.0% total UI cost.
- Knob drag at 60 Hz: scenery still maintains 30 FPS without dropping below 28 FPS measured.

### 9.4 Audio-thread isolation

- The UI thread never touches DSP state directly. Parameter changes flow through `APVTS` → `ParameterAttachment` → atomic params consumed in `processBlock`. (This matches project rule 6 and the existing scaffold's structure.)
- UI animation timers run on the message thread only; no thread hops besides `JUCE`'s built-in `MessageManager::callAsync` for cross-thread notifications (e.g., voice count updates).

---

## 10. 90% Cockpit Accuracy Strategy

Goal: viewers familiar with the reference image must immediately recognize the plugin as "the same cockpit." We hold this with five disciplines.

### 10.1 Trace, don't reinvent

Before any custom paint code is written, we trace the cockpit image into normalized polygon bounds: overhead silhouette, windshield cutout, dashboard frame, tablet outlines, console housing. These polygons live in `Source/GUI/CockpitGeometry.h` and become the authoritative layout source.

### 10.2 Use the image as the cockpit shell

For the densest areas (overhead, side tablets bezel, console housing, dashboard frame), we ship pre-rendered art assets:

```
Resources/UI/Cockpit/
├── cockpit_shell_1x.png        // 1600x900 baseline cockpit shell
├── cockpit_shell_125x.png      // 2000x1125 hi-res tier
├── overhead_panel_1x.png
├── overhead_panel_125x.png
├── dashboard_frame_1x.png
├── dashboard_frame_125x.png
├── left_tablet_bezel_1x.png
├── right_tablet_bezel_1x.png
├── console_housing_1x.png
├── console_housing_125x.png
└── windshield_static_1x.png    // mountains + sky base
```

Custom paint sits **on top** of these images. We don't try to redraw the cockpit in primitives — we'd lose accuracy and CPU.

### 10.3 Preserve symmetry and perspective

- Side tablets are mirror images of each other in layout and size.
- Yokes are bilaterally symmetric.
- Windshield vanishing point is centered horizontally; runway centerline aligns with it.
- Throttle is centered on the console; emergency switch sits to its right at a fixed offset.

### 10.4 Lighting consistency

- Every illuminated UI element pulls from a small color set in `AviatorTokens` (electricBlue / iceBlue / iceGlow). No ad-hoc colors.
- Glow intensity scales with state, not with scale factor.
- Champagne gold is used **only** on the throttle grip band as a brand identifier — nowhere else.

### 10.5 Acceptance check

For each milestone, we side-by-side the running plugin against the reference image. Where divergence exceeds visual estimate of 10%, we file a fix ticket. The check is documented in `TESTPLAN.md` updates per milestone.

---

## 11. Implementation Phases

### Phase 1 — Layout Skeleton (target: 2 days)
- Create `AviatorTokens.h`.
- Resize default editor to 1600 × 900, lock aspect.
- Replace `MainPanel` layout with cockpit regions (headers, overhead, windshield, dashboard, tablets, console, footer) using **placeholder colored rects** so each region's bounds are visible.
- Confirm proportional scaling between 1280×720 and 2048×1152.
- Verify FL Studio scan still finds the plugin and the new editor opens without resize crashes.

### Phase 2 — Static Cockpit Shell (target: 4 days)
- Drop in `cockpit_shell_1x.png` and overlay components.
- Implement `CockpitBackground`, `OverheadPanel` (with 12 stub `IlluminatedButton`s), tablet bezels, dashboard frame image, console housing image, footer.
- No interactivity yet beyond stub buttons that print to debug log.
- Visual acceptance: side-by-side cockpit silhouette ≥ 95%.

### Phase 3 — Placeholder Feature Integration (target: 6 days)
- Implement `ThrottleControl`, `EngineDisplay`, `WingWidthControl`, `LandingGearControl`, `RadarBrowser`, `FlightModeSelector`, `EmergencySwitch`, `CabinLightsControl`, `AltitudeControl`, `TurbulenceControl`.
- Wire each to its `APVTS` parameter via attachments.
- Smoke-test parameter automation in FL Studio: write a single-line automation lane per control and confirm the UI follows playback.
- Smoke-test preset recall: change all controls, save preset, reload, verify state matches.

### Phase 4 — Animated Scenery (target: 3 days)
- Implement aurora drift layer, runway light blink, glass reflection drift.
- Single scenery timer at 30 FPS; per-layer dirty flags.
- Profile CPU on Win 10/11 in FL Studio. Hold idle UI cost < 2.0%.

### Phase 5 — Luxury Polish (target: 4 days)
- Per-control glow, shadow, bevel passes.
- LED states for all illuminated elements (off / armed / active / clipped).
- Fine cockpit labels (HYDRAULICS, LIGHTS, COMM, etc.) baked into overhead PNG; ensure legibility at 0.8 scale.
- Brand logo and preset name in `HeaderBar`; reserve right-edge zone for plugin resize handle.
- Micro animations: throttle "snap" on release, emergency-switch guard close, gear detent click.

### Phase 6 — QA / FL Studio Testing (target: 3 days)
- Full FL Studio matrix: open, save, reopen, multiple instances, automation, transport sync, sample-rate change, buffer-size change, bypass/reload, project reopen.
- Windows display-scaling pass (100%, 125%, 150%, 175%, 200%).
- CPU profile: idle, single-note, polyphony bursts, fast automation.
- Update `TESTPLAN.md`, `known-host-issues.md`, `CHANGELOG.md`.

Total target: ~22 working days for the full UI rebuild.

---

## 12. Risks and Fixes

| Risk                                                                | Likelihood | Impact   | Mitigation |
|---------------------------------------------------------------------|------------|----------|------------|
| Aspect-ratio enforcement fights FL Studio wrapper                  | Medium     | High     | Lock aspect inside `ComponentBoundsConstrainer`; accept FL letterboxing. Document in `known-host-issues.md`. |
| Baked cockpit PNGs look soft at 1.28× scale                         | Medium     | Medium   | Ship a 1.25× PNG tier; clamp scale at 1.28 so we never upscale 1× past 1.28×. |
| Param ID drift breaks user automation                               | Medium     | Critical | Keep `PluginProcessor` parameter IDs frozen even when relabeled; document every rename in `docs/PARAMETERS.md`. |
| Aurora + runway-lights cost too much CPU at 30 FPS                  | Medium     | Medium   | Pre-bake aurora frames into a 60-frame loop; runway blink uses a precomputed brightness LUT. |
| Emergency switch fires accidentally                                 | Medium     | Medium   | Two-step interaction: open guard, then press. Guard auto-recloses on 2 s idle. |
| Custom font (DM Mono / Cormorant) missing on host system            | Low        | Medium   | Embed via `juce_add_binary_data`; fall back to JUCE typeface if loading fails. |
| Repaint storms on parameter automation                              | Low        | Medium   | Throttle attachment listeners via `juce::AsyncUpdater` per control. |
| Visual divergence creeps past 10% over milestones                   | Medium     | High     | Cockpit-accuracy acceptance check after every milestone (see §10.5). |
| Existing M0 GUI code is partially obsolete                          | High       | Low      | Old GUI files move to `Source/GUI/_legacy/` until Phase 3 completes, then deleted. Build target excludes `_legacy/`. |
| FL Studio plugin category misclassification (effect vs instrument)  | Low        | High     | Verify `juce_add_plugin` flags `IS_SYNTH TRUE NEEDS_MIDI_INPUT TRUE` and check the FL scan log per build. |
| Editor crashes on resize at small sizes                             | Medium     | High     | Test 1280×720 first in every phase; cache images regenerate only on actual size change. |

---

## 13. Exact First Coding Steps

Each step is a small reviewable commit. Do them in order; do not skip.

1. **Add `Source/GUI/AviatorTokens.h`** with the contents from §6. Build, confirm no warnings.
2. **Add `Resources/UI/Cockpit/.gitkeep`** plus a placeholder gray-rectangle `cockpit_shell_1x.png` so the binary-data target compiles even before final art arrives. Update `juce_add_binary_data` in `CMakeLists.txt` to include `Resources/UI/Cockpit/*.png`.
3. **In `PluginEditor.cpp` constructor**, change default size to `setSize (1600, 900)` and call `setResizable (true, true)` + assign a `ComponentBoundsConstrainer` with `setFixedAspectRatio (16.f / 9.f)`, min 1280×720, max 2048×1152.
4. **Refactor `MainPanel::resized()`** to compute scale via `AviatorTokens::scaleFactor (getWidth())` and lay out seven region rects (`headerR`, `overheadR`, `windshieldR`, `dashboardR`, `leftTabletR`, `rightTabletR`, `consoleR`, `footerR`). Paint each region with a debug color and a label. **Commit and screenshot.**
5. **Add `Source/GUI/Cockpit/CockpitGeometry.h`** with `NormalizedRect` polygons for each region traced from the reference image. Replace the rects from step 4 with calls into this file.
6. **Create `Source/GUI/Cockpit/CockpitBackground.{h,cpp}`** and add it as the first child of `MainPanel`. It draws the placeholder `cockpit_shell_1x.png` scaled to bounds.
7. **Create stub component files** `OverheadPanel`, `WindshieldScenePanel`, `FlightDashboardPanel`, `LeftPresetTablet`, `RightControlTablet`, `CenterConsolePanel` under `Source/GUI/Cockpit/`. Each starts as an empty `juce::Component` with a debug `paint()` that draws its name in `AviatorTokens::iceBlue()`.
8. **Wire region bounds** to those stubs. Build and verify each region is visible at its design proportions across the full resize range. **Commit. End of Phase 1.**
9. **Move existing M0 GUI files** under `Source/GUI/_legacy/` and exclude from CMake (or move to a separate static lib excluded from the plugin target). Keep `PluginProcessor.cpp` and `Source/State/*` and `Source/DSP/*` untouched.
10. **Decide on cockpit shell PNG provenance.** Either we paint it from the reference manually, or we generate cockpit-tier art assets through whatever workflow you prefer. The plan does not commit to a specific generator — only to the file paths in §10.2.

After step 10, Phase 2 begins.

---

## 14. Suggested File Structure

```
Source/
├── PluginProcessor.{h,cpp}                    // unchanged in scope (rebind labels only)
├── PluginEditor.{h,cpp}                       // owns MainPanel
├── DSP/                                       // unchanged
│   ├── SamplerEngine.{h,cpp}
│   ├── GlideEngine.{h,cpp}
│   ├── ToneShaper.{h,cpp}
│   ├── ReverbTail.{h,cpp}
│   ├── ReversePlayer.{h,cpp}
│   └── SmearProcessor.{h,cpp}
├── State/                                     // unchanged
│   ├── StateSchema.h
│   ├── PresetManager.{h,cpp}
│   ├── SampleLibrary.{h,cpp}
│   └── FactoryResources.{h,cpp}
├── MIDI/                                      // unchanged
│   └── MidiHandler.{h,cpp}
└── GUI/
    ├── AviatorTokens.h                        // NEW — replaces DesignTokens.h once migration done
    ├── DesignTokens.h                         // KEEP temporarily as alias shim
    ├── MainPanel.{h,cpp}                      // REWRITE — cockpit layout
    ├── HeaderBar.{h,cpp}                      // RESTYLE
    ├── FooterBar.{h,cpp}                      // RESTYLE
    ├── LuxuryLookAndFeel.{h,cpp}              // REPLACE with AviatorLookAndFeel
    ├── Cockpit/
    │   ├── CockpitGeometry.h                  // NEW — normalized region polygons
    │   ├── CockpitBackground.{h,cpp}          // NEW
    │   ├── OverheadPanel.{h,cpp}              // NEW
    │   ├── WindshieldScenePanel.{h,cpp}       // NEW
    │   ├── FlightDashboardPanel.{h,cpp}       // NEW
    │   ├── LeftPresetTablet.{h,cpp}           // NEW (hosts RadarBrowser)
    │   ├── RightControlTablet.{h,cpp}         // NEW
    │   └── CenterConsolePanel.{h,cpp}         // NEW
    ├── Controls/
    │   ├── ThrottleControl.{h,cpp}            // NEW
    │   ├── EmergencySwitch.{h,cpp}            // NEW
    │   ├── WingWidthControl.{h,cpp}           // NEW
    │   ├── LandingGearControl.{h,cpp}         // NEW
    │   ├── CabinLightsControl.{h,cpp}         // NEW
    │   ├── AltitudeControl.{h,cpp}            // NEW
    │   ├── TurbulenceControl.{h,cpp}          // NEW
    │   ├── FlightModeSelector.{h,cpp}         // NEW
    │   ├── RadarBrowser.{h,cpp}               // NEW
    │   ├── EngineDisplay.{h,cpp}              // NEW
    │   ├── IlluminatedButton.{h,cpp}          // NEW
    │   ├── AircraftToggle.{h,cpp}             // NEW
    │   ├── ScreenTab.{h,cpp}                  // NEW
    │   ├── MeterDisplay.{h,cpp}               // NEW
    │   ├── PrecisionKnob.{h,cpp}              // RESTYLE existing
    │   └── HorizontalFader.{h,cpp}            // RESTYLE existing
    └── _legacy/                               // existing M0 components staged for deletion
        ├── ArtworkPanel.{h,cpp}
        ├── MacroSection.{h,cpp}
        ├── SecondaryPanel.{h,cpp}
        ├── CategoryBar.{h,cpp}
        ├── PresetBrowser.{h,cpp}              // superseded by RadarBrowser
        ├── PluginShell.{h,cpp}
        ├── ReverseToggle.{h,cpp}
        ├── WaveformDisplay.{h,cpp}
        └── KnobComponent.{h,cpp}

Resources/
├── Factory/                                   // existing audio samples
└── UI/
    └── Cockpit/                               // NEW
        ├── cockpit_shell_1x.png
        ├── cockpit_shell_125x.png
        ├── overhead_panel_1x.png
        ├── overhead_panel_125x.png
        ├── dashboard_frame_1x.png
        ├── dashboard_frame_125x.png
        ├── left_tablet_bezel_1x.png
        ├── right_tablet_bezel_1x.png
        ├── console_housing_1x.png
        ├── console_housing_125x.png
        └── windshield_static_1x.png

docs/
├── UI_COCKPIT_IMPLEMENTATION_PLAN.md          // this document
├── ARCHITECTURE.md                            // update with cockpit hierarchy
├── PARAMETERS.md                              // update with frozen IDs + labels
└── ...                                        // unchanged
```

---

## Appendix A — Notes for the Implementing Agent

- **Do not delete `Source/GUI/_legacy/` until Phase 3 completes and parameter binding is verified in FL Studio.** It exists as a safety net for parameter ID continuity.
- **Every new file gets a one-paragraph header comment** explaining its responsibility, what it does *not* do, and which `AviatorTokens` it depends on. (Project rule: comments only where they add real value — this is one of those places.)
- **`MainPanel` is the only component that owns the scenery timer.** No child component starts its own timer. If a child needs to animate, it exposes a `setAnimationDirty()` method that `MainPanel`'s timer calls.
- **Acceptance per commit:** the plugin must still scan in FL Studio after every commit. If a commit breaks the FL scan, revert and split the change.
- **Compatibility note tag:** every PR that touches `Source/State/StateSchema.h` or `PluginProcessor` parameter layout must include the literal string `COMPAT-NOTE:` followed by an explanation, per project rule 9.

---

End of plan.
