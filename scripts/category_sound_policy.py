#!/usr/bin/env python3
"""Category + soundType policy — shared by import, validate, and audit scripts."""

from __future__ import annotations

import re
from dataclasses import dataclass
from enum import IntEnum
from typing import Optional

CANONICAL_CATEGORIES = [
    "Leads",
    "Brass",
    "Ensembles",
    "Strings",
    "Pads",
    "Chords",
    "Synths",
    "Arps",
    "Vocals",
    "Bells",
]

CHROMATIC_CATEGORIES = {"Leads", "Brass", "Strings", "Synths", "Bells"}
PHRASE_CATEGORIES = {"Vocals", "Arps", "Pads", "Chords", "Ensembles"}


class SoundType(IntEnum):
    ONE_SHOT = 0
    PHRASE = 1
    LOOP = 2
    SLICE = 3


class PlaybackMode(IntEnum):
    ONE_SHOT_ORIGINAL = 0
    PHRASE_ORIGINAL = 1
    CHROMATIC_RESAMPLE = 2
    PHRASE_TIME_STRETCH = 3
    SLICE_PHRASE = 4


@dataclass(frozen=True)
class CategoryPolicy:
    name: str
    default_sound_type: SoundType
    default_playback_mode: PlaybackMode
    keytrack: bool = False


def category_slug(category: str) -> str:
    return category.lower()


def get_policy(category: str) -> CategoryPolicy:
    if category in CHROMATIC_CATEGORIES:
        return CategoryPolicy(category, SoundType.ONE_SHOT, PlaybackMode.CHROMATIC_RESAMPLE)
    return CategoryPolicy(category, SoundType.PHRASE, PlaybackMode.PHRASE_ORIGINAL)


def playback_mode_for(category: str, sound_type: SoundType) -> PlaybackMode:
    if sound_type == SoundType.SLICE:
        return PlaybackMode.SLICE_PHRASE
    if sound_type == SoundType.ONE_SHOT:
        # Ensembles one-shots are single pitched notes and must track MIDI.
        if category in CHROMATIC_CATEGORIES or category == "Ensembles":
            return PlaybackMode.CHROMATIC_RESAMPLE
        return PlaybackMode.ONE_SHOT_ORIGINAL
    if sound_type == SoundType.LOOP:
        return PlaybackMode.PHRASE_ORIGINAL
    return PlaybackMode.PHRASE_ORIGINAL


def _stem_lower(stem: str) -> str:
    return stem.lower().replace("-", "_")


def infer_sound_type(category: str, display_name: str) -> SoundType:
    text = _stem_lower(display_name)
    has_arp = "arp" in text or "arpeggio" in text
    has_vocal = "vocal" in text or "vox" in text
    has_loop = "loop" in text or "chop_loop" in text
    has_phrase = "phrase" in text or "motif" in text or "riff" in text
    has_one_shot = (
        "one_shot" in text
        or "oneshot" in text
        or re.search(r"(?<![a-z])os(?![a-z])", text) is not None
        or "stab" in text
        or re.search(r"(?<![a-z])hit(?![a-z])", text) is not None
    )

    if has_loop and not has_one_shot:
        return SoundType.LOOP
    if has_arp or (has_phrase and not has_one_shot):
        return SoundType.PHRASE
    if has_vocal and category == "Vocals":
        return SoundType.PHRASE
    if has_one_shot:
        return SoundType.ONE_SHOT
    if category in CHROMATIC_CATEGORIES:
        return SoundType.ONE_SHOT
    if category in PHRASE_CATEGORIES:
        return SoundType.PHRASE
    return get_policy(category).default_sound_type


def suggest_category(display_name: str, current: str) -> Optional[str]:
    """Return a better category when filename strongly disagrees with folder."""
    text = _stem_lower(display_name)
    has_arp = "arp" in text or "arpeggio" in text
    has_vocal = ("vocal" in text or "vox" in text) and not has_arp
    has_flute = "flute" in text or "woodwind" in text
    has_brass = "brass" in text or "horn" in text or "trumpet" in text
    has_string = any(k in text for k in ("violin", "viola", "cello", "string", "bow"))
    has_pad = "pad" in text or "ambient" in text or "texture" in text
    has_bell = "bell" in text or "mallet" in text
    has_lead = "lead" in text
    has_guitar = "guitar" in text
    has_chord = "chord" in text or "piano" in text

    if has_vocal and current != "Vocals":
        return "Vocals"
    if has_arp and current not in ("Arps", "Vocals"):
        return "Arps"
    if has_flute and current == "Vocals":
        return "Arps"
    if has_brass and current != "Brass":
        return "Brass"
    if has_string and current not in ("Strings", "Ensembles"):
        return "Strings"
    if has_pad and current != "Pads":
        return "Pads"
    if has_bell and current != "Bells":
        return "Bells"
    if has_lead and current not in ("Leads", "Synths"):
        return "Leads"
    if has_chord and has_guitar and current not in ("Chords", "Arps"):
        return "Chords"
    if has_guitar and has_arp and current != "Arps":
        return "Arps"
    return None


def is_sample_id_compatible(sample_id: str, category: str) -> bool:
    if sample_id in ("factory_default",):
        return True
    if not sample_id.startswith("factory_"):
        return False
    prefix = f"factory_{category_slug(category)}_"
    return sample_id.startswith(prefix)


def loop_mode_for(category: str, sound_type: SoundType) -> int:
  if sound_type == SoundType.LOOP:
    return 1
  if (
      sound_type == SoundType.ONE_SHOT
      and category not in CHROMATIC_CATEGORIES
      and category != "Ensembles"
  ):
    return 0
  return 2


def playback_params_for(category: str, display_name: str) -> dict[str, float | int]:
    sound_type = infer_sound_type(category, display_name)
    mode = playback_mode_for(category, sound_type)
    loop_mode = loop_mode_for(category, sound_type)
    bpm_sync = (
        1
        if sound_type in (SoundType.LOOP, SoundType.PHRASE)
        and category in PHRASE_CATEGORIES
        and sound_type != SoundType.ONE_SHOT
        else 0
    )
    return {
        "src_playback_mode": int(mode),
        "src_keytrack": 1 if mode == PlaybackMode.CHROMATIC_RESAMPLE else 0,
        "src_loop_mode": loop_mode,
        "src_bpm_sync": bpm_sync,
    }


SOUND_TYPE_NAMES = {
    SoundType.ONE_SHOT: "one_shot",
    SoundType.PHRASE: "phrase",
    SoundType.LOOP: "loop",
    SoundType.SLICE: "slice",
}
