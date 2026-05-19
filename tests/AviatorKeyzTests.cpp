// =============================================================================
//  AviatorKeyz — C++ unit test entry point
//
//  Uses JUCE's built-in UnitTest / UnitTestRunner (in juce_core).
//  No message loop or GUI initialisation needed for DSP-only tests.
//
//  Exit code 0 = all passed, 1 = one or more failures.
//
//  Usage:
//    ./AviatorKeyzTests                         -- run all tests
//    ./AviatorKeyzTests --category AviatorKeyz  -- run one category
// =============================================================================

#include <juce_core/juce_core.h>

int main (int argc, char* argv[])
{
    juce::UnitTestRunner runner;
    runner.setAssertOnFailure (false);

    juce::String category;
    for (int i = 1; i < argc - 1; ++i)
    {
        if (juce::String (argv[i]) == "--category")
            category = argv[i + 1];
    }

    if (category.isNotEmpty())
        runner.runTestsInCategory (category);
    else
        runner.runAllTests();

    const int total    = runner.getNumResults();
    int       failures = 0;
    for (int i = 0; i < total; ++i)
    {
        const auto* r = runner.getResult (i);
        if (r != nullptr && r->failures > 0)
            ++failures;
    }

    juce::Logger::writeToLog (juce::String (total) + " test(s) run, "
                              + juce::String (failures) + " failure(s).");

    return failures > 0 ? 1 : 0;
}
