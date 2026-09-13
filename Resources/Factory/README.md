# Factory samples

One WAV per factory preset, named `factory_<category>_<slug>.wav`. The category
in the filename *is* the browser tab the sound appears in, so moving a sound to
another tab means renaming the file and rewriting its preset XML — there is no
separate index to update.

Tabs, in browser order (see `getCanonicalCategories()` in
`Source/State/CategorySoundPolicy.h`):

| Tab | Sample prefix | Notes |
|-----|---------------|-------|
| Bass | `factory_bass_` | Loads monophonic — each note cuts the previous one |
| Leads | `factory_leads_` | |
| Keys | `factory_keys_` | Pianos, Rhodes/EPs, organs, clavs |
| Brass | `factory_brass_` | |
| Phrases | `factory_phrases_` | The only STRETCH tab — speed never changes key |
| Arps | `factory_arps_` | |
| Synths | `factory_synths_` | |
| Bells | `factory_bells_` | |
| Strings | `factory_strings_` | |
| Plucks | `factory_plucks_` | |
| Ensembles | `factory_ensembles_` | |
| Pads | `factory_pads_` | Sustained/atmospheric material, incl. textures |
| Vocals | `factory_vocals_` | |
| — | `factory_default.wav` | Fallback when a sampleId cannot be resolved |

`factory_default.wav` is a synthetic tone; every other file is licensed content.

## Rebuilding and re-filing

- Rebuild the whole bank from staging: `python3 Scripts/import_factory_bank.py`
  (reads `ContentImport/<Tab>/`, which must mirror the tab list above).
- Move sounds to the tab they belong in: `python3 Scripts/refile_factory_categories.py`
  for a dry run, then `--apply`. It renames the WAVs, rewrites each preset's
  `category`/`sampleId`, and mirrors the moves into `ContentImport/`.

Sessions saved before a re-file still resolve: `FactoryResources::findWavResource`
falls back to matching on the stem when the category prefix no longer lines up.
