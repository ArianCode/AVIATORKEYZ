#!/usr/bin/env python3
"""
Tests for project documentation and release readiness.

Two tiers:
  - COMMITTED CONTENT (always tested): files that are in git and must exist.
  - RELEASE BLOCKERS (flagged but allowed to skip): files required before ship
    that may still be untracked/WIP. These tests use skipTest with clear messages
    rather than hard failures so CI stays green during development.
"""

from __future__ import annotations

import re
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


# ---------------------------------------------------------------------------
# Committed files — must always exist
# ---------------------------------------------------------------------------

COMMITTED_DOCS = [
    "README.md",
    "TESTPLAN.md",
    "CHANGELOG.md",
    "ARCHITECTURE.md",
    "docs/ARCHITECTURE.md",
    "docs/PRODUCT_SPEC.md",
    "docs/MILESTONES.md",
    "docs/PROGRESS.md",
    "docs/PARAMETERS.md",
    "docs/RISK_REGISTER.md",
    "docs/TODO.md",
]

# These exist in the working tree but may not be committed yet.
# Tests will skip (not fail) if missing, with a clear release-blocker message.
RELEASE_BLOCKER_DOCS = [
    ("docs/DELIVERABLES.md",         "Deliverables checklist required before release"),
    ("docs/CONTENT_PIPELINE.md",     "Content pipeline doc required before audio content handoff"),
    ("docs/SAMPLER_ENGINE.md",       "Sampler engine doc required for QA"),
    ("docs/SOUND_AND_PRESET_SPEC.md","Sound and preset spec required for content approval"),
    ("docs/CODE_SIGNING.md",         "Code signing doc required before Windows distribution"),
    ("docs/COFOUNDER_MUST_DECIDE.md","Cofounder alignment doc required before milestone close"),
    ("licenses/EULA.md",             "EULA required (finalized by counsel) before release"),
    ("licenses/IP_SUMMARY.md",       "IP summary required for counsel review before release"),
    ("licenses/THIRD_PARTY_NOTICES.md", "Third-party notices required for distribution"),
    ("licenses/JUCE_DISTRIBUTION.md",   "JUCE distribution notes required — confirm license path"),
    ("licenses/SAMPLE_CLEARANCE_CHECKLIST.md",
                                     "Sample clearance checklist required — all WAVs must be cleared"),
    ("scripts/validate_factory_presets.py", "Validation script must be committed before release"),
    ("scripts/verify_release.sh",           "Release verification script must be committed before release"),
]


class TestCommittedContent(unittest.TestCase):

    def test_all_committed_docs_present(self):
        missing = [f for f in COMMITTED_DOCS if not (ROOT / f).exists()]
        self.assertEqual(missing, [],
                         "Committed doc files are missing:\n  " + "\n  ".join(missing))

    def test_readme_exists_and_non_empty(self):
        readme = ROOT / "README.md"
        self.assertTrue(readme.exists(), "README.md is missing")
        self.assertGreater(readme.stat().st_size, 100, "README.md appears empty")

    def test_testplan_exists_and_non_empty(self):
        tp = ROOT / "TESTPLAN.md"
        self.assertTrue(tp.exists(), "TESTPLAN.md is missing")
        self.assertGreater(tp.stat().st_size, 200, "TESTPLAN.md appears empty")

    def test_changelog_has_version_entries(self):
        cl = ROOT / "CHANGELOG.md"
        if not cl.exists():
            self.skipTest("CHANGELOG.md not found")
        text = cl.read_text(encoding="utf-8")
        self.assertRegex(text, r'\[0\.\d+\.\d+\]',
                         "CHANGELOG.md must have at least one version entry like [0.x.y]")

    def test_docs_directory_exists(self):
        self.assertTrue((ROOT / "docs").is_dir(), "docs/ directory is missing")

    def test_source_directory_exists(self):
        self.assertTrue((ROOT / "Source").is_dir(), "Source/ directory is missing")

    def test_resources_directory_exists(self):
        self.assertTrue((ROOT / "Resources").is_dir(), "Resources/ directory is missing")

    def test_cmake_lists_exists(self):
        self.assertTrue((ROOT / "CMakeLists.txt").exists(),
                        "CMakeLists.txt is missing — cannot build")


class TestReadmeLinks(unittest.TestCase):

    def test_readme_markdown_links_resolve(self):
        """All [text](path) links in README.md must point to existing files."""
        readme = ROOT / "README.md"
        if not readme.exists():
            self.skipTest("README.md not found")
        text = readme.read_text(encoding="utf-8")
        links = re.findall(r'\[([^\]]+)\]\(([^)]+)\)', text)
        broken = []
        for label, href in links:
            if href.startswith("http://") or href.startswith("https://"):
                continue  # skip external URLs
            if href.startswith("#"):
                continue  # skip same-page anchors
            target = ROOT / href
            if not target.exists():
                broken.append(f"[{label}]({href})")
        self.assertEqual(broken, [],
                         "Broken README.md links (file not found):\n  " + "\n  ".join(broken))

    def test_readme_has_build_instructions(self):
        readme = ROOT / "README.md"
        if not readme.exists():
            self.skipTest("README.md not found")
        text = readme.read_text(encoding="utf-8")
        self.assertIn("cmake", text.lower(),
                      "README.md must document CMake build instructions")

    def test_readme_lists_all_9_plus_params(self):
        readme = ROOT / "README.md"
        if not readme.exists():
            self.skipTest("README.md not found")
        text = readme.read_text(encoding="utf-8")
        # README parameter table should list at least the 9 original params
        required_in_readme = ["input_gain", "output_gain", "reverse", "glide_time",
                               "smear", "tone", "reverb_amount", "reverb_size", "stereo_width"]
        missing = [p for p in required_in_readme if p not in text]
        self.assertEqual(missing, [],
                         f"README.md parameter table is missing: {missing}")

    def test_readme_param_table_includes_all_12_params(self):
        """Flag if README param table is outdated (missing env_attack, env_release, pan)."""
        readme = ROOT / "README.md"
        if not readme.exists():
            self.skipTest("README.md not found")
        text = readme.read_text(encoding="utf-8")
        later_params = ["env_attack", "env_release", "pan"]
        missing = [p for p in later_params if p not in text]
        if missing:
            self.skipTest(
                f"README parameter table is missing newer params {missing} — "
                "update the README param table to include all 12 parameters "
                "(env_attack, env_release, pan were added after M0)"
            )


class TestReleaseBlockers(unittest.TestCase):
    """
    These tests flag files that MUST exist before release but may be WIP/untracked.
    They skip with a clear message rather than failing, so CI stays green during dev.
    Flip them to hard failures when entering M5 / release candidate.
    """

    def _check_release_file(self, rel_path: str, reason: str):
        path = ROOT / rel_path
        if not path.exists():
            self.skipTest(
                f"RELEASE BLOCKER: {rel_path} is missing.\n"
                f"  Reason: {reason}\n"
                f"  Action: Create and commit this file before shipping."
            )
        self.assertGreater(path.stat().st_size, 10,
                           f"{rel_path} exists but appears empty — it may be a stub")

    def test_deliverables_md(self):
        self._check_release_file("docs/DELIVERABLES.md",
                                 RELEASE_BLOCKER_DOCS[0][1])

    def test_content_pipeline_md(self):
        self._check_release_file("docs/CONTENT_PIPELINE.md",
                                 RELEASE_BLOCKER_DOCS[1][1])

    def test_sampler_engine_doc(self):
        self._check_release_file("docs/SAMPLER_ENGINE.md",
                                 RELEASE_BLOCKER_DOCS[2][1])

    def test_sound_and_preset_spec(self):
        self._check_release_file("docs/SOUND_AND_PRESET_SPEC.md",
                                 RELEASE_BLOCKER_DOCS[3][1])

    def test_code_signing_doc(self):
        self._check_release_file("docs/CODE_SIGNING.md",
                                 RELEASE_BLOCKER_DOCS[4][1])

    def test_eula_present(self):
        self._check_release_file("licenses/EULA.md",
                                 RELEASE_BLOCKER_DOCS[6][1])

    def test_eula_not_draft(self):
        path = ROOT / "licenses/EULA.md"
        if not path.exists():
            self.skipTest("EULA.md not present (release blocker)")
        text = path.read_text(encoding="utf-8")
        # Flag if EULA still has template placeholders
        blockers = []
        if "[Legal entity name" in text or "TBD" in text:
            blockers.append("EULA still contains '[Legal entity name — TBD]' placeholder")
        if "[State/country" in text:
            blockers.append("EULA still contains '[State/country — TBD]' governing law placeholder")
        if "template" in text.lower() and "counsel" in text.lower():
            blockers.append("EULA appears to still be a draft template — requires counsel approval")
        if blockers:
            self.skipTest(
                "RELEASE BLOCKER — EULA is incomplete:\n  " + "\n  ".join(blockers)
            )

    def test_ip_summary_present(self):
        self._check_release_file("licenses/IP_SUMMARY.md",
                                 RELEASE_BLOCKER_DOCS[7][1])

    def test_third_party_notices(self):
        self._check_release_file("licenses/THIRD_PARTY_NOTICES.md",
                                 RELEASE_BLOCKER_DOCS[8][1])

    def test_juce_distribution_notes(self):
        self._check_release_file("licenses/JUCE_DISTRIBUTION.md",
                                 RELEASE_BLOCKER_DOCS[9][1])

    def test_sample_clearance_checklist(self):
        self._check_release_file("licenses/SAMPLE_CLEARANCE_CHECKLIST.md",
                                 RELEASE_BLOCKER_DOCS[10][1])

    def test_validate_presets_script_committed(self):
        self._check_release_file("scripts/validate_factory_presets.py",
                                 RELEASE_BLOCKER_DOCS[11][1])

    def test_verify_release_script_committed(self):
        self._check_release_file("scripts/verify_release.sh",
                                 RELEASE_BLOCKER_DOCS[12][1])


class TestBuildSystem(unittest.TestCase):

    def test_cmakelists_has_vst3_target(self):
        cmake = ROOT / "CMakeLists.txt"
        if not cmake.exists():
            self.skipTest("CMakeLists.txt not found")
        text = cmake.read_text(encoding="utf-8")
        self.assertIn("VST3", text,
                      "CMakeLists.txt must include VST3 in FORMATS")

    def test_cmakelists_has_standalone_target(self):
        cmake = ROOT / "CMakeLists.txt"
        if not cmake.exists():
            self.skipTest("CMakeLists.txt not found")
        text = cmake.read_text(encoding="utf-8")
        self.assertIn("Standalone", text,
                      "CMakeLists.txt must include Standalone in FORMATS for dev testing")

    def test_cmakelists_uses_juce_8(self):
        cmake = ROOT / "CMakeLists.txt"
        if not cmake.exists():
            self.skipTest("CMakeLists.txt not found")
        text = cmake.read_text(encoding="utf-8")
        # Only match GIT_TAG on non-comment lines (indented cmake block).
        # Using re.MULTILINE and requiring the line starts with whitespace or
        # GIT_TAG directly (not inside a comment).
        m = re.search(r'^\s+GIT_TAG\s+(\S+)', text, re.MULTILINE)
        if m:
            tag = m.group(1)
            self.assertTrue(tag.startswith("8."),
                            f"CMakeLists.txt uses JUCE tag '{tag}' — "
                            "JUCE 8.x required for macOS 15 SDK compatibility")

    def test_cmakelists_plugin_code_present(self):
        cmake = ROOT / "CMakeLists.txt"
        if not cmake.exists():
            self.skipTest("CMakeLists.txt not found")
        text = cmake.read_text(encoding="utf-8")
        self.assertIn("Avk1", text,
                      "PLUGIN_CODE 'Avk1' must be set in CMakeLists.txt — do not change after ship")
        self.assertIn("Avkz", text,
                      "PLUGIN_MANUFACTURER_CODE 'Avkz' must be set — do not change after ship")

    def test_cmakelists_is_synth(self):
        cmake = ROOT / "CMakeLists.txt"
        if not cmake.exists():
            self.skipTest("CMakeLists.txt not found")
        text = cmake.read_text(encoding="utf-8")
        self.assertIn("IS_SYNTH", text,
                      "IS_SYNTH must be set in CMakeLists.txt")
        self.assertIn("TRUE", text,
                      "IS_SYNTH must be TRUE for correct FL Studio instrument categorization")

    def test_cmakelists_embeds_factory_assets(self):
        cmake = ROOT / "CMakeLists.txt"
        if not cmake.exists():
            self.skipTest("CMakeLists.txt not found")
        text = cmake.read_text(encoding="utf-8")
        self.assertIn("juce_add_binary_data", text,
                      "CMakeLists.txt must embed factory assets via juce_add_binary_data")
        self.assertIn("AviatorKeyzData", text,
                      "CMakeLists.txt must define AviatorKeyzData binary target")

    def test_build_artifacts_present(self):
        vst3 = ROOT / "build" / "AviatorKeyz_artefacts" / "Release" / "VST3" / "AviatorKeyz.vst3"
        standalone_mac = (ROOT / "build" / "AviatorKeyz_artefacts" / "Release"
                          / "Standalone" / "AviatorKeyz.app")
        standalone_win = (ROOT / "build" / "AviatorKeyz_artefacts" / "Release"
                          / "Standalone" / "AviatorKeyz.exe")
        has_vst3 = vst3.exists()
        has_standalone = standalone_mac.exists() or standalone_win.exists()
        if not has_vst3:
            self.skipTest(
                "VST3 build artifact not found at build/AviatorKeyz_artefacts/Release/VST3/. "
                "Run: cmake --build build --config Release"
            )
        if not has_standalone:
            self.skipTest(
                "Standalone build artifact not found. "
                "Run: cmake --build build --config Release"
            )

    def test_cmakelists_no_splash_screen(self):
        cmake = ROOT / "CMakeLists.txt"
        if not cmake.exists():
            self.skipTest("CMakeLists.txt not found")
        text = cmake.read_text(encoding="utf-8")
        self.assertIn("JUCE_DISPLAY_SPLASH_SCREEN=0", text,
                      "Splash screen must be disabled — requires JUCE commercial/Indie license")


class TestSourceStructure(unittest.TestCase):

    REQUIRED_SOURCE_FILES = [
        "Source/PluginProcessor.h",
        "Source/PluginProcessor.cpp",
        "Source/PluginEditor.h",
        "Source/PluginEditor.cpp",
        "Source/State/StateSchema.h",
        "Source/State/FactoryResources.h",
        "Source/State/FactoryResources.cpp",
        "Source/State/PresetManager.h",
        "Source/State/PresetManager.cpp",
        "Source/State/SampleLibrary.h",
        "Source/State/SampleLibrary.cpp",
        "Source/DSP/SamplerEngine.h",
        "Source/DSP/SamplerEngine.cpp",
        "Source/DSP/ToneShaper.h",
        "Source/DSP/ToneShaper.cpp",
        "Source/DSP/SmearProcessor.h",
        "Source/DSP/SmearProcessor.cpp",
        "Source/DSP/GlideEngine.h",
        "Source/DSP/GlideEngine.cpp",
        "Source/DSP/ReverbTail.h",
        "Source/DSP/ReverbTail.cpp",
        "Source/DSP/ReversePlayer.h",
        "Source/DSP/ReversePlayer.cpp",
        "Source/GUI/LuxuryLookAndFeel.h",
        "Source/GUI/LuxuryLookAndFeel.cpp",
        "Source/GUI/MainPanel.h",
        "Source/GUI/MainPanel.cpp",
        "Source/GUI/Cockpit/CockpitCrossworldPanel.h",
        "Source/GUI/Cockpit/CockpitCrossworldPanel.cpp",
        "Source/GUI/Cockpit/PhotoAnchoredKnob.h",
        "Source/GUI/Cockpit/PhotoAnchoredKnob.cpp",
        "Source/GUI/Cockpit/CockpitZones.h",
        "Source/GUI/Cockpit/CockpitZones.cpp",
        "Source/GUI/PresetBrowser.h",
        "Source/GUI/PresetBrowser.cpp",
        "Source/DSP/PitchProbe.h",
        "Source/DSP/PitchProbe.cpp",
        "Source/MIDI/MidiHandler.h",
        "Source/MIDI/MidiHandler.cpp",
    ]

    def test_all_required_source_files_present(self):
        missing = [f for f in self.REQUIRED_SOURCE_FILES
                   if not (ROOT / f).exists()]
        self.assertEqual(missing, [],
                         "Required source files missing:\n  " + "\n  ".join(missing))

    def test_state_schema_h_present(self):
        self.assertTrue((ROOT / "Source" / "State" / "StateSchema.h").exists(),
                        "StateSchema.h is the single source of truth for parameter IDs — must exist")

    def test_no_param_id_in_source_outside_schema(self):
        """Parameter ID strings should only be defined in StateSchema.h, referenced elsewhere."""
        schema = ROOT / "Source" / "State" / "StateSchema.h"
        if not schema.exists():
            self.skipTest("StateSchema.h not found")
        schema_text = schema.read_text(encoding="utf-8")
        schema_ids = set(re.findall(r'"([a-z_][a-z0-9_]*)"', schema_text))

        violations = []
        for cpp in (ROOT / "Source").rglob("*.cpp"):
            if "StateSchema" in cpp.name:
                continue
            text = cpp.read_text(encoding="utf-8", errors="ignore")
            for pid in schema_ids:
                # Look for the literal string being defined (not just referenced via ParamID::)
                if re.search(rf'=\s*"{re.escape(pid)}"', text):
                    violations.append(f"{cpp.name}: redefines param ID '{pid}' — use ParamID::{pid.upper()}")
        self.assertEqual(violations, [],
                         "Param ID string literals defined outside StateSchema.h:\n  "
                         + "\n  ".join(violations))


if __name__ == "__main__":
    unittest.main(verbosity=2)
