# AviatorKeyz — IP summary (for counsel review)

**Product:** AviatorKeyz (VST3 / Standalone)  
**Repository:** local AviatorKeyz VST 3 project  
**Last updated:** 2026-04-21

This document is a factual **inventory and intent** only. It is **not** legal advice. Open items and assignments require signed agreements and counsel in your jurisdiction.

---

## 1. Project and entity

| Item | Status |
|------|--------|
| Legal entity holding IP / receiving revenue | TBD (pre-inc, LLC, or other — **counsel to advise**) |
| Jurisdiction of formation / governing law | TBD |
| DBA / public name | AviatorKeyz (as used in `CMakeLists.txt` and plugin metadata) |

---

## 2. People and roles (non-equity)

| Person | Role (descriptive) |
|--------|---------------------|
| **Arian** | Lead developer; product and engineering direction; code, DSP, and plugin UI implementation. |
| **Keyz** | Brand and public presence; industry connections, partnerships, content, and marketing. |

**Joint / approval areas (intended, subject to written agreement):** public-facing commitments, product name and logo, major spend, and use of any person’s name or likeness in paid advertising.

---

## 3. IP inventory (asset classes)

### 3.1 Source code and plugin UI (C++ / JUCE)

| Asset | Creator / author | Pre-existing or for this project? | Assignment to project entity |
|------|------------------|-------------------------------------|-------------------------------|
| `Source/**` (processor, editor, state, DSP, GUI, MIDI) | Arian (primary) | TBD: confirm any pre-venture code | TBD: signed IP assignment or employment/contractor agreement |
| `CMakeLists.txt` and build | Arian (primary) | For this project | Same as above |

**Third-party code:** JUCE 8.0.9 (see [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) and [JUCE_DISTRIBUTION.md](JUCE_DISTRIBUTION.md)). No other vendored source trees in-repo as of this inventory.

### 3.2 Embedded binary data (shipped in plugin)

Declared in [CMakeLists.txt](../CMakeLists.txt) via `juce_add_binary_data(AviatorKeyzData ...)`:

| File (repo path) | Type | Provenance to confirm | Rights / notes |
|-------------------|------|------------------------|----------------|
| `Resources/Factory/factory_default.wav` | Audio sample (WAV) | TBD: author / recording / license | Confirm composition + sound recording; clearance for redistribution in a commercial binary |
| `Resources/Presets/Factory/Leads/Init.xml` | Preset (XML) | TBD: author | Preset as data; if derived from third-party content, follow same terms as source material |
| `Resources/Presets/Factory/Pads/Pad_Warm.xml` | Preset (XML) | TBD: author | Same as above |

*Note: If `factory_default.wav` is not present in the working tree, it is still a **build input** in CMake; keep this row until the file is present or the build list is updated.*

### 3.3 Fonts and typography

| Asset | Bundled? | License |
|------|----------|--------|
| (none identified in-repo) | UI uses JUCE default system fonts; no custom `.ttf`/`.otf` in `Resources` | N/A for embedded fonts at this time — **re-audit before ship** if you add bundled fonts (OFL / embedding rights) |

### 3.4 Name, logo, and marketing

| Asset | Who created / owns | Notes |
|--------|--------------------|--------|
| **Name “AviatorKeyz”** / company string in `COMPANY_NAME` | TBD: confirm designers and any prior use | Trademark clearance and **assignment to entity** — open for counsel |
| **Logo / artwork** | TBD (Keyz-originated vs commission vs Arian) | Marketing assets: specify assignment vs personal brand use |
| **Website / social / video content** | TBD per asset | List Keyz-originated vs project-owned; reduce ambiguity with written license or work-made-for-hire where applicable |

---

## 4. Third-party and distribution (summary pointer)

- **JUCE** — [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md), [SOURCES.md](SOURCES.md), [JUCE_DISTRIBUTION.md](JUCE_DISTRIBUTION.md), [vendors/JUCE-8.0.9-LICENSE.md](vendors/JUCE-8.0.9-LICENSE.md)  
- **VST3 SDK** — Brought in via JUCE; proprietary Steinberg VST3 license (see JUCE’s dependency list; building/selling VST3 requires compliance with Steinberg and your JUCE path).

---

## 5. Commercialization (intent only)

| Topic | Status |
|-------|--------|
| Distribution channels | TBD (direct, marketplaces, etc.) |
| Payout account (who receives store revenue) | TBD; align with entity and tax (see [GOVERNANCE_COUNSEL_QUESTIONS.md](GOVERNANCE_COUNSEL_QUESTIONS.md)) |
| Price tiers, upgrades, education pricing, refunds | Policy level — align with each platform’s rules; **TBD** until product launch plan is fixed |

---

## 6. Open items for counsel (checklist)

- [ ] Founder or partnership agreement: equity, vesting, roles, decision rights, exit/dispute process.  
- [ ] IP assignments: code, content, name/logo, from each individual (and any contractors) to the **chosen entity**.  
- [ ] **JUCE** license path: proprietary product distribution vs AGPL — confirm active JUCE commercial/Indie subscription or GPL compliance.  
- [ ] **Steinberg VST3** and **Avid** (if AAX later) — developer agreements and redistribution.  
- [ ] **Samples and presets** — chain of title, any library EULAs, and AI-generated content policy if used.  
- [ ] **Trademark** — search and application strategy for “AviatorKeyz” and logo.  
- [ ] **Keyz** — contractor vs partner; likeness / publicity rights in marketing; exclusivity and deliverables.  
- [ ] **Tax and payouts** — entity choice, 1099 vs W-2 vs distributions, and multi-jurisdiction nexus.  

---

*Prepared as a one-pass review pack alongside `licenses/THIRD_PARTY_NOTICES.md` and `licenses/GOVERNANCE_COUNSEL_QUESTIONS.md`.*
