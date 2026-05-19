# JUCE — distribution and license path (AviatorKeyz)

**Purpose:** Document how this project **uses** JUCE and what a **proprietary, closed-source** VST3 build implies, so a lawyer and your accountant can align **CMake settings** with **contractual** obligations.

**This is not legal advice.**

---

## How this project uses JUCE

- **Integration:** JUCE 8.0.9 is obtained via **CMake `FetchContent`** and linked as static/shared libraries for the VST3 and Standalone targets. See [../CMakeLists.txt](../CMakeLists.txt).  
- **Modules used:** e.g. `juce_audio_utils`, `juce_audio_processors`, `juce_audio_formats`, `juce_dsp`, `juce_gui_extra`, `juce_gui_basics`.  
- **Splash / reporting:** The project sets:

  `JUCE_DISPLAY_SPLASH_SCREEN=0` and `JUCE_REPORT_APP_USAGE=0`

  Per JUCE’s historical documentation, the splash is tied to **GPL/AGPL** use in many setups; **disabling the splash** is consistent with a **non-GPL** path but **does not** by itself grant a commercial license. **You must still** have a valid **JUCE commercial / Indie** subscription (or other approved commercial license) for **proprietary, closed-source** distribution, **or** you must **comply with AGPL** (including source distribution rules).

---

## Two real paths (summary)

| Path | What it means (high level) | Typical for AviatorKeyz if you ship closed binaries? |
|------|----------------------------|------------------------------------------------------|
| **JUCE commercial (e.g. Personal / Indie / Pro / Enterprise)** | You follow the [JUCE 8 EULA](https://juce.com/legal/juce-8-licence/) and **pay/qualify** per JUCE’s pricing. | **Yes** — this is the usual path for a **closed-source** commercial plugin. |
| **AGPLv3** | You license JUCE under AGPL; you must **comply with AGPL** for your **combined work** (often including **offering** corresponding source to users, depending on how you distribute). | Only if you **intend** an open-source / AGPL product; **not** a typical proprietary storefront plugin without legal review. |

**Action item:** Keep **proof of subscription** or **license agreement** with JUCE/ROLI as applicable, and have counsel confirm the EULA matches your **revenue** and **team size** tier.

---

## VST3 and Steinberg

VST3 builds use the **VST3 SDK** included in JUCE’s tree under **proprietary** Steinberg terms. That is **separate** from JUCE’s AGPL/commercial choice. You must be **registered** with Steinberg and comply with the **VST3 SDK** license for distributing VST3 plug-ins. Your lawyer should treat this as a **separate** checkbox from JUCE’s license.

---

## Suggested one-line for your IP packet

> “AviatorKeyz is a proprietary audio plug-in that links JUCE 8.0.9. We intend to ship under the **JUCE commercial license** (or alternative path **TBD and confirmed with counsel**), not AGPL, and to comply with the **Steinberg VST3** license for VST3 builds.”

Replace “intend” with “actively maintain [tier] with Juce Ltd.” once true.

---

## Reference files in this folder

- [vendors/JUCE-8.0.9-LICENSE.md](vendors/JUCE-8.0.9-LICENSE.md)  
- [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)  
- [SOURCES.md](SOURCES.md)  
