#!/usr/bin/env python3
"""Tests for factory preset XML files — structure, parameters, categories, sample references."""

from __future__ import annotations

import unittest
import xml.etree.ElementTree as ET
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PRESETS_DIR = ROOT / "Resources" / "Presets" / "Factory"
FACTORY_DIR = ROOT / "Resources" / "Factory"

EXPECTED_CATEGORIES = {
    "Leads", "Brass", "Ensembles", "Strings", "Pads",
    "Chords", "Synths", "Arps", "Vocals", "Bells",
}

REQUIRED_PARAMS = {
    "input_gain", "output_gain", "reverse", "glide_time", "smear", "tone",
    "reverb_amount", "reverb_size", "stereo_width", "env_attack", "env_release", "pan",
}

ALLOWED_FACTORY_PARAMS = REQUIRED_PARAMS | {
    "src_playback_mode", "src_keytrack", "src_loop_mode", "src_bpm_sync",
}

PARAM_RANGES = {
    "input_gain":    (-24.0, 12.0),
    "output_gain":   (-24.0, 12.0),
    "reverse":       (0.0, 1.0),
    "glide_time":    (0.0, 500.0),
    "smear":         (0.0, 1.0),
    "tone":          (-1.0, 1.0),
    "reverb_amount": (0.0, 1.0),
    "reverb_size":   (0.0, 1.0),
    "stereo_width":  (0.0, 2.0),
    "env_attack":    (0.0, 5000.0),
    "env_release":   (5.0, 10000.0),
    "pan":           (-1.0, 1.0),
    "src_playback_mode": (0.0, 4.0),
    "src_keytrack":      (0.0, 1.0),
    "src_loop_mode":     (0.0, 2.0),
    "src_bpm_sync":      (0.0, 1.0),
}


def _load_all_presets() -> tuple[list[tuple[Path, ET.Element]], list[tuple[Path, str]]]:
    parsed: list[tuple[Path, ET.Element]] = []
    errors: list[tuple[Path, str]] = []
    for xml in sorted(PRESETS_DIR.rglob("*.xml")):
        try:
            root = ET.parse(xml).getroot()
            parsed.append((xml, root))
        except ET.ParseError as exc:
            errors.append((xml, str(exc)))
    return parsed, errors


_PARSED, _ERRORS = _load_all_presets()
_WAV_IDS = {p.stem for p in FACTORY_DIR.glob("*.wav")}


class TestPresetXMLStructure(unittest.TestCase):

    def test_presets_directory_exists(self):
        self.assertTrue(PRESETS_DIR.exists(),
                        f"Factory presets directory missing: {PRESETS_DIR}")

    def test_minimum_factory_preset_count(self):
        total = len(_PARSED) + len(_ERRORS)
        self.assertGreaterEqual(total, 50,
                                f"Expected at least 50 factory presets, found {total}")

    def test_no_xml_parse_errors(self):
        if _ERRORS:
            msgs = [f"  {p.name}: {e}" for p, e in _ERRORS]
            self.fail("XML parse errors:\n" + "\n".join(msgs))

    def test_all_presets_have_Preset_root_tag(self):
        bad = [p.name for p, r in _PARSED if r.tag != "Preset"]
        self.assertEqual(bad, [],
                         "Files with wrong root tag:\n  " + "\n  ".join(bad))

    def test_all_presets_have_name_attribute(self):
        bad = [p.name for p, r in _PARSED if not r.get("name")]
        self.assertEqual(bad, [],
                         "Presets missing name attribute:\n  " + "\n  ".join(bad))

    def test_all_presets_have_sampleId_attribute(self):
        bad = [p.name for p, r in _PARSED if not r.get("sampleId")]
        self.assertEqual(bad, [],
                         "Presets missing sampleId attribute:\n  " + "\n  ".join(bad))

    def test_all_presets_have_schemaVersion_1(self):
        bad = []
        for p, r in _PARSED:
            ver = r.get("schemaVersion")
            if ver is None:
                bad.append(f"{p.name}: missing schemaVersion")
            elif ver != "1":
                bad.append(f"{p.name}: unexpected schemaVersion '{ver}'")
        self.assertEqual(bad, [], "\n  ".join(bad))

    def test_all_presets_have_AviatorKeyzState_child(self):
        bad = [p.name for p, r in _PARSED if r.find("AviatorKeyzState") is None]
        self.assertEqual(bad, [],
                         "Presets missing <AviatorKeyzState>:\n  " + "\n  ".join(bad))

    def test_no_extra_XML_siblings_in_AviatorKeyzState(self):
        bad = []
        for p, r in _PARSED:
            state = r.find("AviatorKeyzState")
            if state is None:
                continue
            non_param = [child.tag for child in state if child.tag != "PARAM"]
            if non_param:
                bad.append(f"{p.name}: unexpected children {non_param}")
        self.assertEqual(bad, [], "\n  ".join(bad))


class TestPresetParameters(unittest.TestCase):

    def test_all_required_params_present(self):
        bad = []
        for p, r in _PARSED:
            state = r.find("AviatorKeyzState")
            if state is None:
                continue
            ids = {el.get("id") for el in state.findall("PARAM")}
            missing = REQUIRED_PARAMS - ids
            if missing:
                bad.append(f"{p.name}: missing {sorted(missing)}")
        self.assertEqual(bad, [], "\n  ".join(bad))

    def test_no_unknown_param_ids(self):
        bad = []
        for p, r in _PARSED:
            state = r.find("AviatorKeyzState")
            if state is None:
                continue
            for el in state.findall("PARAM"):
                pid = el.get("id", "")
                if pid not in ALLOWED_FACTORY_PARAMS:
                    bad.append(f"{p.name}: unknown param id '{pid}'")
        self.assertEqual(bad, [], "\n  ".join(bad))

    def test_all_param_values_are_numeric(self):
        bad = []
        for p, r in _PARSED:
            state = r.find("AviatorKeyzState")
            if state is None:
                continue
            for el in state.findall("PARAM"):
                val_str = el.get("value", "")
                try:
                    float(val_str)
                except ValueError:
                    bad.append(f"{p.name}: param '{el.get('id')}' value '{val_str}' is not numeric")
        self.assertEqual(bad, [], "\n  ".join(bad))

    def test_all_param_values_within_range(self):
        bad = []
        for p, r in _PARSED:
            state = r.find("AviatorKeyzState")
            if state is None:
                continue
            for el in state.findall("PARAM"):
                pid = el.get("id", "")
                if pid not in PARAM_RANGES:
                    continue
                try:
                    val = float(el.get("value", "nan"))
                except ValueError:
                    continue
                lo, hi = PARAM_RANGES[pid]
                if not (lo - 1e-6 <= val <= hi + 1e-6):
                    bad.append(f"{p.name}: '{pid}'={val} outside [{lo}, {hi}]")
        self.assertEqual(bad, [], "\n  ".join(bad))

    def test_no_duplicate_param_ids_within_preset(self):
        bad = []
        for p, r in _PARSED:
            state = r.find("AviatorKeyzState")
            if state is None:
                continue
            ids = [el.get("id") for el in state.findall("PARAM")]
            seen: set[str] = set()
            for pid in ids:
                if pid in seen:
                    bad.append(f"{p.name}: duplicate param id '{pid}'")
                seen.add(pid)
        self.assertEqual(bad, [], "\n  ".join(bad))


class TestPresetCategories(unittest.TestCase):

    def test_all_10_expected_categories_present(self):
        cats_in_presets = {r.get("category") for _, r in _PARSED}
        missing = EXPECTED_CATEGORIES - cats_in_presets
        self.assertEqual(missing, set(),
                         f"Missing categories: {missing}")

    def test_no_unexpected_categories(self):
        unknown = {r.get("category") for _, r in _PARSED} - EXPECTED_CATEGORIES
        self.assertEqual(unknown, set(),
                         f"Unknown categories found: {unknown}")

    def test_at_least_5_presets_per_category(self):
        by_cat: dict[str, int] = {c: 0 for c in EXPECTED_CATEGORIES}
        for _, r in _PARSED:
            cat = r.get("category", "")
            if cat in by_cat:
                by_cat[cat] += 1
        bad = [f"{c}: {n} presets" for c, n in by_cat.items() if n < 5]
        self.assertEqual(bad, [],
                         "Categories with fewer than 5 presets:\n  " + "\n  ".join(bad))

    def test_category_subdir_names_match_expected(self):
        actual_dirs = {p.name for p in PRESETS_DIR.iterdir() if p.is_dir()}
        self.assertEqual(actual_dirs, EXPECTED_CATEGORIES,
                         f"Extra dirs: {actual_dirs - EXPECTED_CATEGORIES}, "
                         f"Missing dirs: {EXPECTED_CATEGORIES - actual_dirs}")

    def test_presets_in_matching_category_subdir(self):
        bad = []
        for p, r in _PARSED:
            cat_attr = r.get("category", "")
            dir_name = p.parent.name
            if cat_attr != dir_name:
                bad.append(f"{p.name}: XML category='{cat_attr}' but in dir '{dir_name}'")
        self.assertEqual(bad, [], "\n  ".join(bad))

    def test_preset_filename_matches_xml_name(self):
        """Filename stem should equal XML name (with _ replacing spaces)."""
        bad = []
        for p, r in _PARSED:
            xml_name = r.get("name", "").replace(" ", "_")
            file_stem = p.stem.replace(" ", "_")
            if xml_name != file_stem:
                bad.append(f"{p.name}: XML name='{r.get('name')}' vs filename='{p.stem}'")
        self.assertEqual(bad, [], "\n  ".join(bad))


class TestPresetSampleReferences(unittest.TestCase):

    def test_factory_wav_dir_exists(self):
        self.assertTrue(FACTORY_DIR.exists(),
                        f"Factory WAV directory missing: {FACTORY_DIR}")

    def test_factory_wav_count_matches_per_preset_model(self):
        self.assertGreaterEqual(len(_WAV_IDS), 50,
                                f"Expected many per-preset factory WAVs, found {len(_WAV_IDS)}")

    def test_all_preset_sampleIds_have_matching_wav(self):
        bad = []
        for p, r in _PARSED:
            sid = r.get("sampleId", "")
            if sid not in _WAV_IDS:
                bad.append(f"{p.name}: sampleId '{sid}' has no WAV in Resources/Factory/")
        self.assertEqual(bad, [], "\n  ".join(bad))

    def test_factory_default_wav_present(self):
        self.assertIn("factory_default", _WAV_IDS,
                      "factory_default.wav is required as the fallback sample")


class TestSpecificPresets(unittest.TestCase):

    def _get(self, category: str, name: str):
        for p, r in _PARSED:
            if r.get("category") == category and r.get("name") == name:
                return r
        return None

    def _params(self, root) -> dict[str, float]:
        state = root.find("AviatorKeyzState")
        if state is None:
            return {}
        return {el.get("id"): float(el.get("value", 0)) for el in state.findall("PARAM")}

    def test_leads_init_exists(self):
        self.assertIsNotNone(self._get("Leads", "Init"),
                             "Leads/Init is required — it is the default startup preset")

    def test_leads_init_is_clean_starting_point(self):
        root = self._get("Leads", "Init")
        self.assertIsNotNone(root)
        params = self._params(root)
        self.assertEqual(params.get("input_gain"), 0.0, "Init: input_gain must be 0 dB")
        self.assertEqual(params.get("output_gain"), 0.0, "Init: output_gain must be 0 dB")
        self.assertEqual(params.get("reverse"), 0.0, "Init: reverse must be off")
        self.assertEqual(params.get("glide_time"), 0.0, "Init: glide_time must be 0")
        self.assertEqual(params.get("pan"), 0.0, "Init: pan must be center")

    def test_leads_init_sample_id_references_embedded_wav(self):
        root = self._get("Leads", "Init")
        self.assertIsNotNone(root)
        sid = root.get("sampleId", "")
        self.assertTrue(sid.startswith("factory_"),
                        f"Init sampleId should reference embedded factory WAV, got {sid!r}")
        self.assertIn(sid, _WAV_IDS,
                      f"Init sampleId {sid!r} has no matching WAV in Resources/Factory/")

    def test_any_pad_has_elevated_attack(self):
        """At least one pad preset should use a slow attack."""
        pads = [(p, r) for p, r in _PARSED if r.get("category") == "Pads"]
        any_slow = any(self._params(r).get("env_attack", 0.0) > 20.0 for _, r in pads)
        self.assertTrue(any_slow, "Expected at least one Pads preset with env_attack > 20ms")

    def test_any_reverb_preset_has_reverb_amount(self):
        """At least one pad preset should use reverb."""
        pads = [(p, r) for p, r in _PARSED if r.get("category") == "Pads"]
        any_reverb = any(
            self._params(r).get("reverb_amount", 0.0) > 0.0
            for _, r in pads
        )
        self.assertTrue(any_reverb, "No Pads preset uses reverb — expected at least one pad with reverb")

    def test_any_reverse_preset_has_reverse_enabled(self):
        """At least one preset per category with 'Reverse' in the name should have reverse=1."""
        bad = []
        for p, r in _PARSED:
            if "reverse" in r.get("name", "").lower():
                params = self._params(r)
                if params.get("reverse", 0.0) < 0.5:
                    bad.append(f"{p.name}: has 'Reverse' in name but reverse param is off")
        self.assertEqual(bad, [], "\n  ".join(bad))

    def test_any_glide_preset_has_glide_time(self):
        """Presets with 'Glide' in the name should have glide_time > 0."""
        bad = []
        for p, r in _PARSED:
            if "glide" in r.get("name", "").lower():
                params = self._params(r)
                if params.get("glide_time", 0.0) <= 0.0:
                    bad.append(f"{p.name}: name contains 'Glide' but glide_time is 0")
        self.assertEqual(bad, [], "\n  ".join(bad))


if __name__ == "__main__":
    unittest.main(verbosity=2)
