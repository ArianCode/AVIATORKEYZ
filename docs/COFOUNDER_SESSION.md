# Cofounder session — questions, accountability, and “we forgot that”

**Product:** AviatorKeyz (premium sample-based VST3 instrument)  
**Use:** Working agenda for founder + creative producer. Fill **Owner**, **Decide by**, and **Output** in each section before or during the meeting.

**Related repo docs:** [PRODUCT_SPEC.md](PRODUCT_SPEC.md) · [DELIVERABLES.md](DELIVERABLES.md) · [SOUND_AND_PRESET_SPEC.md](SOUND_AND_PRESET_SPEC.md) · [COFOUNDER_MUST_DECIDE.md](COFOUNDER_MUST_DECIDE.md)

---

## Session log

| Field | Value |
|-------|--------|
| **Meeting date** | |
| **Attendees** | |
| **Next sync** | |

---

## 1) North star and success (alignment)

| | |
|--|--|
| **Owner** | |
| **Decide by** | |
| **Output** | One-page vision + “not doing” list |

**Questions**

- What problem are we solving, for whom, in one sentence? Who is *not* the target user?
- What does “success in 6–12 months” look like: revenue, users, press, artist adoption, portfolio piece?
- What is the **one non-negotiable** product promise (e.g. every preset feels alive via Reverse / Glide / Smear / Tone)?
- What would make us **pivot** vs. **double down**?

**Notes**

---

## 2) Roles, accountability, and decision rights (RACI-lite)

| | |
|--|--|
| **Owner** | |
| **Decide by** | |
| **Output** | Signed “who owns what” (1 page) |

**Questions**

- Who owns: **product vision**, **sound/DSP direction**, **UI/UX**, **release quality**, **marketing narrative**, **partnerships**, **money/legal**?
- For conflicts: who has **final say** in which domain?
- What are each person’s **weekly time budgets** and **hard constraints**?
- When blocked: **escalation** path and max wait time?

**RACI draft (fill initials)**

| Domain | Responsible | Accountable | Consulted | Informed |
|--------|-------------|-------------|-----------|----------|
| Product vision | | | | |
| Sound / factory content | | | | |
| DSP / engineering | | | | |
| UI / brand | | | | |
| Release / QA | | | | |
| Marketing / launch | | | | |
| Legal / money | | | | |

**Notes**

---

## 3) Scope and “definition of done” (what we actually ship)

| | |
|--|--|
| **Owner** | |
| **Decide by** | |
| **Output** | Scope doc (must/should/could) + release checklist |

**Questions**

- **MVP vs v1.0 vs later:** What ships first; what is explicitly out? (See [MILESTONES.md](MILESTONES.md).)
- **Host/format:** VST3 only for v1? **macOS first** (then Windows)? AU / AAX later?
- **Factory content:** 11 category WAVs + 50 presets — minimum bar for “complete”? ([DELIVERABLES.md](DELIVERABLES.md))
- **User sample import:** In v1.0 or post-v1? ([PRODUCT_SPEC.md](PRODUCT_SPEC.md))
- **Accessibility / localization:** e.g. English-only UI for v1?

**Notes**

---

## 4) Creative direction (the producer lens)

| | |
|--|--|
| **Owner** | |
| **Decide by** | |
| **Output** | Creative brief (1–2 pages) + private reference playlist |

**Questions**

- Reference artists, albums, adjectives for sonic identity — shared vocabulary?
- What **musical use cases** must feel great first (pads vs leads vs keys)?
- **A/B** against which commercial plugins? What are we *not* copying?
- **Content pipeline:** Who records demos, which DAWs, tempos, genres?

**Notes**

---

## 4b) Factory sound content (one-shots, melodies, launch bank)

| | |
|--|--|
| **Owner** | |
| **Decide by** | |
| **Output** | [SOUND_AND_PRESET_SPEC.md](SOUND_AND_PRESET_SPEC.md) + [factory_content_tracker.csv](factory_content_tracker.csv) |

**Questions**

- **Engine fit:** v1.0 = **one embedded WAV per category** + presets that set `sampleId` — not a one-shot rompler. Agreed?
- **What to record:** Category-defining **sustained / playable** sources per `factory_*.wav`, not hundreds of one-shots unless scope changes.
- **Melody loops:** Marketing-only vs shipped in-product?
- **Minimum factory bar:** All 11 WAVs cleared + 50 presets approved?
- **Loudness / tuning:** See spec (peak ≤ -1 dBFS, A4 = 440 Hz, etc.).
- **Demo pack vs factory:** What stays off-product (social, reels)?

**Notes**

---

## 5) Brand, story, and marketing

| | |
|--|--|
| **Owner** | |
| **Decide by** | |
| **Output** | Store page draft (copy + bullets + screenshot list) |

**Questions**

- Name, tagline, positioning (one paragraph for press/store).
- Visual identity: logo, wordmark, UI reference — who owns art direction?
- Launch narrative: why now, for who, proof (quotes, testers).
- Post-launch update cadence and version communication.

**Notes**

---

## 6) Users, feedback, and community

| | |
|--|--|
| **Owner** | |
| **Decide by** | |
| **Output** | Beta plan + feedback table template |

**Questions**

- **10–30 target users:** How to recruit; compensation (license, $, credit)?
- **Feedback protocol:** Bugs vs taste; response SLA; public vs private channels.
- **Beta:** Duration, NDA, exit criteria.

**Notes**

---

## 7) Legal, IP, and money

| | |
|--|--|
| **Owner** | |
| **Decide by** | |
| **Output** | Updated [licenses/IP_SUMMARY.md](../licenses/IP_SUMMARY.md) + completed [SAMPLE_CLEARANCE_CHECKLIST.md](../licenses/SAMPLE_CLEARANCE_CHECKLIST.md) |

**Questions**

- Entity, equity, vesting, departure scenarios (lawyer review).
- IP ownership: code, UI, samples, presets, name/logo.
- Third-party audio/libs — redistribution in a VST3 binary ([licenses/](../licenses/)).
- Revenue model: price, sales, upgrades, education, refunds.
- Tax and payouts: who receives store revenue.

**Notes**

---

## 8) Operations: tools and single source of truth

| | |
|--|--|
| **Owner** | |
| **Decide by** | |
| **Output** | “Stack” doc (tools + admin per tool) |

**Questions**

- Where do **tasks**, **files**, and **decisions** live?
- **Naming/versioning** for builds and presets.
- **Backups:** repo, installers, signing certs ([CODE_SIGNING.md](CODE_SIGNING.md)).

**Notes**

---

## 9) Quality bar and risk (plugin-specific)

| | |
|--|--|
| **Owner** | |
| **Decide by** | |
| **Output** | QA checklist + known-issues list for ship |

**Questions**

- **Test matrix:** DAWs × OS — supported vs best effort. (v1 target: FL Studio + Windows per [PRODUCT_SPEC.md](PRODUCT_SPEC.md).)
- **Release criteria:** No crashers; automation; preset load; CPU bounds ([TESTPLAN.md](../TESTPLAN.md)).
- **Bad release:** Rollback, comms template, hotfix owner.

**Notes**

---

## 10) Ethical and professional guardrails

| | |
|--|--|
| **Owner** | |
| **Decide by** | |
| **Output** | Written defaults (analytics, marketing honesty, crunch boundaries) |

**Questions**

- Analytics / crash reports / privacy policy timing.
- Marketing language: honest positioning vs hype.
- Sustainability: crunch limits and recovery.

**Notes**

---

## 11) Relationship and process

| | |
|--|--|
| **Owner** | |
| **Decide by** | |
| **Output** | Optional “working together” doc (meeting rhythm, conflict rules) |

**Questions**

- **Meeting rhythm:** Weekly product sync, async updates, off boundaries.
- **How we fight:** Good intent, time-boxed debates, no silent treatment.
- **Exit scenarios:** IP and name if someone leaves.

**Notes**

---

## Quick punch list (check before public launch)

- [ ] One-page **vision** and **not doing** list
- [ ] **Scope** + MVP vs v1.0 locked
- [ ] **RACI** and weekly capacity
- [ ] **Creative brief** + references
- [ ] **Store / marketing** copy draft
- [ ] **Factory sound** spec + tracker + clearance ([SOUND_AND_PRESET_SPEC.md](SOUND_AND_PRESET_SPEC.md))
- [ ] **Beta** + feedback process
- [ ] **IP / licenses / money / entity**
- [ ] **QA** matrix + known issues
- [ ] **Runbooks** for release, bad release, support

---

**After the meeting:** Copy action items into your task tracker; revisit this doc after first beta and before launch.
