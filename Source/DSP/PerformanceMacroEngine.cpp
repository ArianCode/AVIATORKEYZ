#include "PerformanceMacroEngine.h"

PerformanceMacroOffsets PerformanceMacroEngine::compute (float macro1,
                                                           float macro2,
                                                           float macro3,
                                                           float macro4) noexcept
{
    PerformanceMacroOffsets o;
    o.textureAmount = macro1 * 0.5f;
    o.filterCutoff  = (macro2 - 0.5f) * 2.f;
    o.reverbAmount  = macro3 * 0.4f;
    o.smear         = macro4 * 0.35f;
    o.tone          = (macro1 - 0.5f) * 0.4f;
    o.grainRate     = macro2 * 0.35f;
    o.osc1Level     = (macro3 - 0.5f) * 0.4f;
    o.osc2Level     = (macro4 - 0.5f) * 0.4f;
    return o;
}
