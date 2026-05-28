/** Normalized zone bounds (0–1) relative to cockpit photo. Shared by HTML prototype. */
const CockpitZones = {
  leftMfd:         { left: 0.02, top: 0.48, right: 0.16, bottom: 0.80, shape: 'rounded-rect' },
  rightMfd:        { left: 0.84, top: 0.48, right: 0.98, bottom: 0.80, shape: 'rounded-rect' },
  radarAdsr:       { left: 0.28, top: 0.50, right: 0.44, bottom: 0.75, shape: 'circle' },
  radarLfo:        { left: 0.56, top: 0.50, right: 0.72, bottom: 0.75, shape: 'circle' },
  autopilotStrip:  { left: 0.10, top: 0.42, right: 0.90, bottom: 0.47, shape: 'rect' },
  overhead:        { left: 0.00, top: 0.00, right: 1.00, bottom: 0.28, shape: 'rect' },
  throttleQuadrant: { left: 0.42, top: 0.60, right: 0.58, bottom: 0.90, shape: 'rect' },
};

function zoneToCSS(z) {
  return {
    left:   (z.left * 100) + '%',
    top:    (z.top * 100) + '%',
    width:  ((z.right - z.left) * 100) + '%',
    height: ((z.bottom - z.top) * 100) + '%',
  };
}

if (typeof module !== 'undefined')
  module.exports = { CockpitZones, zoneToCSS };
