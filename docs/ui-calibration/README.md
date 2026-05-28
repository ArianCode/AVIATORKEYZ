# Cockpit UI Calibration

Initial zone bounds (normalized 0–1) match the photo-anchored spec. Refine with the HTML prototype:

1. Open `aviatorkeyz_cockpit_prototype.html` in a browser.
2. Add `?debug=1` to show zone outlines and anchor circles.
3. Add `?calibrate=1` and click hardware; copy logged `(x%, y%)` into `knobAnchors.json` and `CockpitZones.cpp`.
4. Press `B` to toggle the photo off (acceptance: overlays should look orphaned without the photo).

## Initial zones

| Zone | left | top | right | bottom |
|------|------|-----|-------|--------|
| leftMfd | 0.02 | 0.48 | 0.16 | 0.80 |
| rightMfd | 0.84 | 0.48 | 0.98 | 0.80 |
| radarAdsr | 0.28 | 0.50 | 0.44 | 0.75 |
| radarLfo | 0.56 | 0.50 | 0.72 | 0.75 |
| autopilotStrip | 0.10 | 0.42 | 0.90 | 0.47 |
| overhead | 0.00 | 0.00 | 1.00 | 0.28 |
| throttleQuadrant | 0.42 | 0.60 | 0.58 | 0.90 |

Tolerance target: ±1% on bezels and radar rings at 1600×900.
