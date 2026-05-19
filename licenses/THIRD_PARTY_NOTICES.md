# Third-party notices — AviatorKeyz

This file lists **third-party components** used to build and ship the product, for license compatibility review and **attribution** where required.  
**It is a technical inventory, not legal advice.** Counsel should confirm completeness before commercial release.

---

## Build-time and linked dependencies

### JUCE 8.0.9

| Field | Value |
|-------|--------|
| **Name** | JUCE |
| **Version** | 8.0.9 (pinned in CMake `FetchContent`) |
| **Source** | https://github.com/juce-framework/JUCE |
| **How used** | Fetched at build time; compiled into the plugin/standalone. Modules linked include `juce_audio_utils`, `juce_audio_processors`, `juce_audio_formats`, `juce_dsp`, `juce_gui_extra`, `juce_gui_basics` (see [../CMakeLists.txt](../CMakeLists.txt)). |
| **License** | **Dual:** GNU **AGPLv3** (if you elect open-source compliance) **or** [JUCE 8 commercial / End User Licence Agreement](https://juce.com/legal/juce-8-licence/) (typical for closed-source, proprietary distribution). |
| **Attribution** | See JUCE’s [LICENSE.md](vendors/JUCE-8.0.9-LICENSE.md) and the official EULA. Additional notices may be required on your website or in-product per your JUCE agreement. |
| **Your obligation** | For a **closed-source, shipped binary**, you must either comply with **AGPL** (including source offer) or hold a valid **JUCE commercial / Indie** license. **See [JUCE_DISTRIBUTION.md](JUCE_DISTRIBUTION.md).** |

### JUCE’s transitive dependencies (examples relevant to a typical VST3 build)

JUCE’s own `LICENSE.md` lists embedded subcomponents (FLAC, Ogg, zlib, VST3 SDK, AAX SDK, HarfBuzz, etc.). The **VST3 SDK** in particular is **proprietary** to Steinberg; your use is subject to the Steinberg VST3 license in addition to JUCE. Do not copyleft-assume: read the VST3 SDK terms when distributing VST3 builds.

*Full list and paths:* refer to the JUCE 8.0.9 tree under your build’s `_deps` or to [vendors/JUCE-8.0.9-LICENSE.md](vendors/JUCE-8.0.9-LICENSE.md) (summary + links).

---

## Shipped with the product (your content)

| Item | License / clearance |
|------|----------------------|
| `Resources/Factory/factory_default.wav` | **TBD** — document author and license; add to this file when known. |
| `Resources/Presets/Factory/**/*.xml` | **TBD** — preset authorship; if based on unlicensed third-party material, same restriction applies. |

When you add **fonts**, **sample libraries**, or **new audio**, add a row here and place full license/EULA text under `licenses/vendors/`.

---

## Reproducibility

- Version pins: [SOURCES.md](SOURCES.md)
