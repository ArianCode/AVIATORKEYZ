// =============================================================================
//  ParameterWiringTests — APVTS layout ↔ StateSchema parity
//
//  Ensures createParameterLayout() registers exactly the ParamIDs declared in
//  StateSchema.h (via SchemaParamList.h canonical list).
// =============================================================================

#include <juce_core/juce_core.h>
#include <juce_audio_processors/juce_audio_processors.h>

#include "SchemaParamList.h"
#include "State/ParameterLayout.h"

class ParameterWiringTests : public juce::UnitTest
{
public:
    ParameterWiringTests() : juce::UnitTest ("Parameter_Wiring", "AviatorKeyz") {}

    void runTest() override
    {
        beginTest ("createParameterLayout registers expected param count");
        {
            const auto registered = AviatorKeyz::getRegisteredParameterIds();
            expectEquals (registered.size(), kExpectedSchemaParamCount,
                          "APVTS layout size must match schema param count");
        }

        beginTest ("Every schema ParamID is registered in APVTS layout");
        {
            const auto registered = AviatorKeyz::getRegisteredParameterIds();
            const auto schemaIds = allSchemaParamIDs();
            for (const auto& id : schemaIds)
                expect (registered.contains (id),
                        "Schema ParamID missing from APVTS layout: " + id);
        }

        beginTest ("No extra APVTS params outside schema list");
        {
            const auto registered = AviatorKeyz::getRegisteredParameterIds();
            const auto schemaIds = allSchemaParamIDs();

            for (const auto& id : registered)
                expect (schemaIds.contains (id),
                        "APVTS param not declared in schema list: " + id);
        }

        beginTest ("No duplicate APVTS param IDs in layout");
        {
            const auto registered = AviatorKeyz::getRegisteredParameterIds();

            for (int i = 0; i < registered.size(); ++i)
                for (int j = i + 1; j < registered.size(); ++j)
                    expect (registered[i] != registered[j],
                            "Duplicate APVTS param ID: " + registered[i]);
        }
    }
};

static ParameterWiringTests parameterWiringTests;
