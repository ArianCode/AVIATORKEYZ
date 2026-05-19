#!/usr/bin/env python3
"""
Tests for state/parameter schema consistency.

Parses StateSchema.h to extract authoritative parameter IDs,
then verifies all preset XML files are consistent with the schema.
Also checks that APVTS layout (PluginProcessor.cpp) defines the
same parameters as StateSchema.h declares.
"""

from __future__ import annotations

import re
import unittest
import xml.etree.ElementTree as ET
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SCHEMA_H   = ROOT / "Source" / "State" / "StateSchema.h"
PROCESSOR_CPP = ROOT / "Source" / "PluginProcessor.cpp"
PRESETS_DIR = ROOT / "Resources" / "Presets" / "Factory"


def _extract_param_ids_from_schema() -> set[str]:
    """Read all string literals under namespace ParamID in StateSchema.h."""
    if not SCHEMA_H.exists():
        return set()
    text = SCHEMA_H.read_text(encoding="utf-8")
    # Find the ParamID namespace block
    m = re.search(r'namespace ParamID\s*\{(.+?)\}', text, re.DOTALL)
    if not m:
        return set()
    block = m.group(1)
    # Extract all string literals assigned to constexpr const char*
    return set(re.findall(r'"([a-z_][a-z0-9_]*)"', block))


def _extract_param_ids_from_processor() -> set[str]:
    """Extract ParameterID string literals from createParameterLayout() in PluginProcessor.cpp."""
    if not PROCESSOR_CPP.exists():
        return set()
    text = PROCESSOR_CPP.read_text(encoding="utf-8")
    # Match: ParameterID { ParamID::FOO, 1 } or ParameterID { "literal", 1 }
    return set(re.findall(r'ParamID::([A-Z_]+)', text))


def _extract_param_ids_from_presets() -> set[str]:
    ids: set[str] = set()
    for xml in PRESETS_DIR.rglob("*.xml"):
        try:
            root = ET.parse(xml).getroot()
        except ET.ParseError:
            continue
        state = root.find("AviatorKeyzState")
        if state is None:
            continue
        for param in state.findall("PARAM"):
            pid = param.get("id", "")
            if pid:
                ids.add(pid)
    return ids


def _extract_sample_ids_from_schema() -> set[str]:
    if not SCHEMA_H.exists():
        return set()
    text = SCHEMA_H.read_text(encoding="utf-8")
    m = re.search(r'namespace SampleID\s*\{(.+?)\}', text, re.DOTALL)
    if not m:
        return set()
    block = m.group(1)
    return set(re.findall(r'"([a-z_][a-z0-9_]*)"', block))


def _extract_category_names_from_schema() -> set[str]:
    if not SCHEMA_H.exists():
        return set()
    text = SCHEMA_H.read_text(encoding="utf-8")
    m = re.search(r'namespace Category\s*\{(.+?)\}', text, re.DOTALL)
    if not m:
        return set()
    block = m.group(1)
    return set(re.findall(r'"([A-Za-z][A-Za-z0-9]*)"', block))


_SCHEMA_PARAM_IDS = _extract_param_ids_from_schema()
_PRESET_PARAM_IDS = _extract_param_ids_from_presets()
_SCHEMA_SAMPLE_IDS = _extract_sample_ids_from_schema()
_SCHEMA_CATEGORY_NAMES = _extract_category_names_from_schema()


class TestStateSchemaH(unittest.TestCase):

    def test_schema_h_exists(self):
        self.assertTrue(SCHEMA_H.exists(),
                        f"StateSchema.h not found at {SCHEMA_H}")

    def test_schema_defines_12_param_ids(self):
        self.assertEqual(len(_SCHEMA_PARAM_IDS), 12,
                         f"Expected 12 param IDs in StateSchema.h, "
                         f"found {len(_SCHEMA_PARAM_IDS)}: {sorted(_SCHEMA_PARAM_IDS)}")

    def test_schema_has_all_expected_param_ids(self):
        expected = {
            "input_gain", "output_gain", "reverse", "glide_time", "smear", "tone",
            "reverb_amount", "reverb_size", "stereo_width", "env_attack", "env_release", "pan",
        }
        missing = expected - _SCHEMA_PARAM_IDS
        self.assertEqual(missing, set(),
                         f"StateSchema.h missing param IDs: {missing}")

    def test_schema_defines_11_sample_ids(self):
        self.assertEqual(len(_SCHEMA_SAMPLE_IDS), 11,
                         f"Expected 11 sample IDs in StateSchema.h SampleID namespace, "
                         f"found {len(_SCHEMA_SAMPLE_IDS)}: {sorted(_SCHEMA_SAMPLE_IDS)}")

    def test_schema_defines_10_categories(self):
        self.assertEqual(len(_SCHEMA_CATEGORY_NAMES), 10,
                         f"Expected 10 categories in StateSchema.h, "
                         f"found {len(_SCHEMA_CATEGORY_NAMES)}: {sorted(_SCHEMA_CATEGORY_NAMES)}")

    def test_schema_categories_match_expected(self):
        expected = {"Leads", "Brass", "Ensembles", "Strings", "Pads",
                    "Chords", "Synths", "Arps", "Vocals", "Bells"}
        self.assertEqual(_SCHEMA_CATEGORY_NAMES, expected,
                         f"Extra: {_SCHEMA_CATEGORY_NAMES - expected}, "
                         f"Missing: {expected - _SCHEMA_CATEGORY_NAMES}")

    def test_schema_version_is_1(self):
        if not SCHEMA_H.exists():
            self.skipTest("StateSchema.h not found")
        text = SCHEMA_H.read_text(encoding="utf-8")
        m = re.search(r'STATE_SCHEMA_VERSION\s*=\s*(\d+)', text)
        self.assertIsNotNone(m, "STATE_SCHEMA_VERSION not found in StateSchema.h")
        self.assertEqual(m.group(1), "1",
                         f"STATE_SCHEMA_VERSION should be 1, got {m.group(1)}")

    def test_schema_default_sample_id_exists(self):
        self.assertIn("factory_default", _SCHEMA_SAMPLE_IDS,
                      "StateSchema.h SampleID::DEFAULT ('factory_default') must be defined")


class TestSchemaVsPresets(unittest.TestCase):

    def test_presets_only_use_schema_param_ids(self):
        if not _SCHEMA_PARAM_IDS:
            self.skipTest("Could not parse StateSchema.h")
        extra = _PRESET_PARAM_IDS - _SCHEMA_PARAM_IDS
        self.assertEqual(extra, set(),
                         f"Presets reference param IDs not in StateSchema.h: {extra}")

    def test_all_schema_params_used_in_at_least_one_preset(self):
        if not _SCHEMA_PARAM_IDS:
            self.skipTest("Could not parse StateSchema.h")
        unused = _SCHEMA_PARAM_IDS - _PRESET_PARAM_IDS
        self.assertEqual(unused, set(),
                         f"Schema params never referenced in any preset: {unused}")

    def test_preset_sample_ids_are_valid_schema_ids(self):
        if not _SCHEMA_SAMPLE_IDS:
            self.skipTest("Could not parse StateSchema.h SampleID namespace")
        bad = []
        for xml in sorted(PRESETS_DIR.rglob("*.xml")):
            try:
                root = ET.parse(xml).getroot()
            except ET.ParseError:
                continue
            sid = root.get("sampleId", "")
            if sid and sid not in _SCHEMA_SAMPLE_IDS:
                bad.append(f"{xml.name}: sampleId '{sid}' not in StateSchema.h SampleID")
        self.assertEqual(bad, [], "\n  ".join(bad))

    def test_preset_categories_are_valid_schema_categories(self):
        if not _SCHEMA_CATEGORY_NAMES:
            self.skipTest("Could not parse StateSchema.h Category namespace")
        bad = []
        for xml in sorted(PRESETS_DIR.rglob("*.xml")):
            try:
                root = ET.parse(xml).getroot()
            except ET.ParseError:
                continue
            cat = root.get("category", "")
            if cat and cat not in _SCHEMA_CATEGORY_NAMES:
                bad.append(f"{xml.name}: category '{cat}' not in StateSchema.h Category namespace")
        self.assertEqual(bad, [], "\n  ".join(bad))


class TestPluginProcessorCpp(unittest.TestCase):

    def test_processor_cpp_exists(self):
        self.assertTrue(PROCESSOR_CPP.exists(),
                        f"PluginProcessor.cpp not found at {PROCESSOR_CPP}")

    def test_processor_defines_all_schema_params(self):
        """Every ParamID constant in StateSchema.h should be referenced in createParameterLayout().

        PluginProcessor.cpp accesses params via ParamID::CONSTANT_NAME (not string literals),
        so this test checks for the C++ constant name (upper_snake), not the string value.
        """
        if not PROCESSOR_CPP.exists():
            self.skipTest("PluginProcessor.cpp not found")
        if not SCHEMA_H.exists():
            self.skipTest("StateSchema.h not found")

        schema_text = SCHEMA_H.read_text(encoding="utf-8")
        processor_text = PROCESSOR_CPP.read_text(encoding="utf-8")

        # Extract constant names (e.g. INPUT_GAIN, SMEAR) from namespace ParamID in StateSchema.h
        m = re.search(r'namespace ParamID\s*\{(.+?)\}', schema_text, re.DOTALL)
        if not m:
            self.skipTest("Could not parse namespace ParamID block in StateSchema.h")

        block = m.group(1)
        # Match: static constexpr const char* CONSTANT_NAME = "value";
        constant_names = re.findall(r'constexpr\s+const\s+char\*\s+([A-Z_]+)\s*=', block)

        missing = [name for name in constant_names
                   if f"ParamID::{name}" not in processor_text]
        self.assertEqual(missing, [],
                         f"These ParamID constants are not used in PluginProcessor.cpp "
                         f"(expected ParamID::NAME): {missing}")

    def test_processor_saves_state_version(self):
        """getStateInformation must store stateVersion for future migration."""
        if not PROCESSOR_CPP.exists():
            self.skipTest("PluginProcessor.cpp not found")
        text = PROCESSOR_CPP.read_text(encoding="utf-8")
        self.assertIn("stateVersion", text,
                      "PluginProcessor.cpp must store 'stateVersion' in getStateInformation")

    def test_processor_reads_state_version(self):
        """setStateInformation must read stateVersion for migration logic."""
        if not PROCESSOR_CPP.exists():
            self.skipTest("PluginProcessor.cpp not found")
        text = PROCESSOR_CPP.read_text(encoding="utf-8")
        self.assertIn("savedVersion", text,
                      "setStateInformation should read the savedVersion for migration")

    def test_init_preset_loaded_in_constructor(self):
        """Constructor must load a factory preset so the plugin starts with valid state."""
        if not PROCESSOR_CPP.exists():
            self.skipTest("PluginProcessor.cpp not found")
        text = PROCESSOR_CPP.read_text(encoding="utf-8")
        self.assertIn("loadPreset", text,
                      "AviatorKeyzProcessor constructor must call loadPreset on startup")


if __name__ == "__main__":
    unittest.main(verbosity=2)
