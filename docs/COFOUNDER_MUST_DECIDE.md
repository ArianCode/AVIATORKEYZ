# Three must-decide items (before next build milestone)

**Purpose:** Lock decisions that unblock engineering, factory content, and commercial ship.  
**Status:** Proposed defaults based on current repo state — **confirm owners and dates in [COFOUNDER_SESSION.md](COFOUNDER_SESSION.md).**

**Suggested “next milestone” anchor:** Exit criteria for **M1 — Core Sampler Engine** ([MILESTONES.md](MILESTONES.md)) or, if M1 is done, **M3 — Preset System** factory content freeze.

---

## 1) Factory audio model and clearance (creative + legal)

**Why now:** `Resources/Factory/*.wav` are **synthetic placeholders** — not cleared for commercial use. Every factory preset points at a `sampleId` that must map to a real, licensed file ([CONTENT_PIPELINE.md](CONTENT_PIPELINE.md), [licenses/SAMPLE_CLEARANCE_CHECKLIST.md](../licenses/SAMPLE_CLEARANCE_CHECKLIST.md)).

**Decide**

| Question | Options / notes |
|----------|-----------------|
| Content model for v1.0 | **Recommended (already in pipeline):** one **category-defining** WAV per bucket (11 files), not a large one-shot library. |
| Creative direction per category | What each `factory_leads.wav`, `factory_pads.wav`, etc. *is* (instrument, texture, length, root note). |
| Who approves final WAV | Single accountable ear for “ships in v1.0.” |
| Clearance deadline | Date all rows in SAMPLE_CLEARANCE_CHECKLIST are complete. |

**Output**

- Approved [SOUND_AND_PRESET_SPEC.md](SOUND_AND_PRESET_SPEC.md)
- [factory_content_tracker.csv](factory_content_tracker.csv) — all 11 WAVs `approved`
- Signed or filed license evidence per sample (outside repo if needed)

**Default owner split (fill in meeting)**

| Role | Suggested owner |
|------|-----------------|
| Record / design sources | Creative producer |
| Technical ingest (naming, peaks, embed) | Engineering |
| Clearance paperwork | Both / legal counsel |

**Decide by:** _______________  
**Owner:** _______________

---

## 2) v1.0 scope and QA matrix (product + release)

**Why now:** [PRODUCT_SPEC.md](PRODUCT_SPEC.md) locks VST3 + Windows + FL Studio; other formats are easy to assume but expensive to deliver.

**Decide**

| Question | Recommended default |
|----------|---------------------|
| Ship formats | **VST3 instrument only** for v1.0 |
| Platforms | **macOS 13+** (ship first); Windows 10/11 x64 secondary |
| Primary DAW | **Logic / Ableton / Reaper** on macOS — golden project + test matrix |
| AU / AAX | Explicitly **out of v1.0** unless you accept milestone slip |
| User sample import | In v1.0 vs post-v1 (spec mentions import; confirm priority) |
| Minimum preset count | **50 factory presets** across 10 categories (current tree) |
| “Complete” definition | All M3 preset tests pass + replaced factory WAVs + `verify_release.sh` clean |

**Output**

- Updated scope section in PRODUCT_SPEC or a one-page “v1.0 ship list”
- Assigned tester + **golden FL Studio project** path
- QA checklist row in [COFOUNDER_SESSION.md](COFOUNDER_SESSION.md) §9

**Decide by:** _______________  
**Owner:** _______________

---

## 3) IP, equity, and money (entity + ownership)

**Why now:** Code, UI, presets, and **embedded samples** must have one owner of record before paid launch or third-party collaborators.

**Decide**

| Question | Notes |
|----------|--------|
| Entity | LLC / corp / partnership — who signs vendor agreements |
| Equity & vesting | Split, cliff, departure |
| IP assignment | Code, presets, factory WAVs, brand assets → entity |
| Revenue | Price point, platform (Gumroad, Plugin Boutique, own site), split of payouts |
| Work-for-hire | If producer records samples — assignment in writing |
| Third-party packs | **Do not** drop unlicensed “royalty-free” one-shots into `Resources/Factory/` |

**Output**

- [licenses/IP_SUMMARY.md](../licenses/IP_SUMMARY.md) filled (no TBD on ownership)
- Counsel review of [EULA.md](../licenses/EULA.md) + sample terms
- Bank / payout account identified

**Decide by:** _______________  
**Owner:** _______________

---

## After these three

1. Schedule weekly **factory content review** until all WAVs are `approved`.
2. Re-run `./scripts/verify_release.sh` after each factory drop.
3. Revisit full [COFOUNDER_SESSION.md](COFOUNDER_SESSION.md) punch list before beta.
