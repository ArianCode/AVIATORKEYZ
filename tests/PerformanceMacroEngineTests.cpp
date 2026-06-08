#include <juce_core/juce_core.h>
#include "DSP/PerformanceMacroEngine.h"

class PerformanceMacroEngineTests : public juce::UnitTest
{
public:
    PerformanceMacroEngineTests() : juce::UnitTest ("PerformanceMacroEngine", "AviatorKeyz") {}

    void runTest() override
    {
        beginTest ("Neutral macro defaults leave filter cutoff offset at zero");
        {
            const auto macros = PerformanceMacroEngine::compute (0.5f, 0.5f, 0.5f, 0.5f);
            expectWithinAbsoluteError (macros.filterCutoff, 0.f, 0.001f);
            expectWithinAbsoluteError (macros.osc1Level, 0.f, 0.001f);
            expectWithinAbsoluteError (macros.osc2Level, 0.f, 0.001f);
        }

        beginTest ("Default filter staging stays above 1 kHz");
        {
            constexpr float filterCutoffHz = 8000.f;
            const auto macros = PerformanceMacroEngine::compute (0.5f, 0.5f, 0.5f, 0.5f);
            const float cutoffNorm = juce::jlimit (0.f, 1.f,
                juce::jmap (filterCutoffHz, 20.f, 20000.f, 0.f, 1.f) + macros.filterCutoff);
            const float cutoffHz = juce::jmap (cutoffNorm, 20.f, 20000.f);
            expect (cutoffHz > 1000.f, "Neutral macros must not slam filter to sub-audible cutoff");
        }
    }
};

static PerformanceMacroEngineTests performanceMacroEngineTests;
