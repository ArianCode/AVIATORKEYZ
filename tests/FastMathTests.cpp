// =============================================================================
//  AviatorFastMath tests — pan law, sine approximation, grain window
// =============================================================================

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "DSP/FastMath.h"
#include <cmath>

class FastMathTests : public juce::UnitTest
{
public:
    FastMathTests() : juce::UnitTest ("FastMath", "AviatorKeyz") {}

    void runTest() override
    {
        namespace FM = AviatorFastMath;
        constexpr float kInvSqrt2 = 0.70710678f;

        beginTest ("Pan: hard left is full left, silent right");
        {
            float l = 0.f, r = 0.f;
            FM::constantPowerPan (-1.f, l, r);
            expectWithinAbsoluteError (l, 1.f, 1.0e-5f);
            expectWithinAbsoluteError (r, 0.f, 1.0e-5f);
        }

        beginTest ("Pan: hard right is full right, silent left");
        {
            float l = 0.f, r = 0.f;
            FM::constantPowerPan (1.f, l, r);
            expectWithinAbsoluteError (l, 0.f, 1.0e-5f);
            expectWithinAbsoluteError (r, 1.f, 1.0e-5f);
        }

        beginTest ("Pan: center is equal-power balanced (L == R == 1/sqrt2)");
        {
            float l = 0.f, r = 0.f;
            FM::constantPowerPan (0.f, l, r);
            expectWithinAbsoluteError (l, kInvSqrt2, 1.0e-5f);
            expectWithinAbsoluteError (r, kInvSqrt2, 1.0e-5f);
            expectWithinAbsoluteError (l - r, 0.f, 1.0e-6f);
        }

        beginTest ("Pan: constant power (L^2 + R^2 == 1) across the range");
        {
            for (float p = -1.f; p <= 1.001f; p += 0.05f)
            {
                float l = 0.f, r = 0.f;
                FM::constantPowerPan (juce::jmin (1.f, p), l, r);
                expectWithinAbsoluteError (l * l + r * r, 1.f, 1.0e-5f);
            }
        }

        beginTest ("Pan: monotonic — left falls and right rises as pan moves right");
        {
            float prevL = 2.f, prevR = -1.f;
            for (float p = -1.f; p <= 1.001f; p += 0.05f)
            {
                float l = 0.f, r = 0.f;
                FM::constantPowerPan (juce::jmin (1.f, p), l, r);
                expect (l <= prevL + 1.0e-6f, "left gain must not increase");
                expect (r >= prevR - 1.0e-6f, "right gain must not decrease");
                prevL = l;
                prevR = r;
            }
        }

        beginTest ("Pan: symmetric — mirrored pan swaps channel gains");
        {
            for (float p = 0.f; p <= 1.001f; p += 0.1f)
            {
                const float pc = juce::jmin (1.f, p);
                float l1 = 0.f, r1 = 0.f, l2 = 0.f, r2 = 0.f;
                FM::constantPowerPan (pc, l1, r1);
                FM::constantPowerPan (-pc, l2, r2);
                expectWithinAbsoluteError (l1, r2, 1.0e-5f);
                expectWithinAbsoluteError (r1, l2, 1.0e-5f);
            }
        }
    }
};

static FastMathTests fastMathTests;
