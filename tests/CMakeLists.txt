# =============================================================================
#  AviatorKeyz — C++ unit tests
#
#  Build:  cmake --build build --target AviatorKeyzTests
#  Run:    ./build/tests/AviatorKeyzTests_artefacts/Release/AviatorKeyzTests
#
#  What is tested here:
#    - SamplerEngine DSP (AudioSnapshot API)
#    - SampleLibrary load/publish (JUCE 8)
#    - StateSchema constants
#    - APVTS layout ↔ schema parity
# =============================================================================

juce_add_console_app(AviatorKeyzTests
    PRODUCT_NAME "AviatorKeyz Unit Tests"
)

target_sources(AviatorKeyzTests
    PRIVATE
        AviatorKeyzTests.cpp
        ApvtsStateHelpersTests.cpp
        SamplerEngineTests.cpp
        SampleLibraryTests.cpp
        StateSchemaTests.cpp
        ParameterWiringTests.cpp
        TextureEngineTests.cpp
        PerformanceMacroEngineTests.cpp
        PresetManagerTests.cpp
        PitchAlignmentTests.cpp
        ${CMAKE_SOURCE_DIR}/Source/DSP/SamplerEngine.cpp
        ${CMAKE_SOURCE_DIR}/Source/DSP/TextureEngine.cpp
        ${CMAKE_SOURCE_DIR}/Source/DSP/ReversePlayer.cpp
        ${CMAKE_SOURCE_DIR}/Source/DSP/PitchProbe.cpp
        ${CMAKE_SOURCE_DIR}/Source/DSP/ModMatrix.cpp
        ${CMAKE_SOURCE_DIR}/Source/DSP/LfoEngine.cpp
        ${CMAKE_SOURCE_DIR}/Source/DSP/PerformanceMacroEngine.cpp
        ${CMAKE_SOURCE_DIR}/Source/State/SampleLibrary.cpp
        ${CMAKE_SOURCE_DIR}/Source/State/PresetManager.cpp
        ${CMAKE_SOURCE_DIR}/Source/State/FactoryResources.cpp
        ${CMAKE_SOURCE_DIR}/Source/State/ParameterLayout.cpp
        ${CMAKE_SOURCE_DIR}/Source/State/ApvtsStateHelpers.cpp
        ${CMAKE_SOURCE_DIR}/Source/GUI/Advanced/AdvancedParameterLayout.cpp
)

target_include_directories(AviatorKeyzTests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/Source
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_definitions(AviatorKeyzTests
    PRIVATE
        JUCE_STANDALONE_APPLICATION=1
        JUCE_USE_CURL=0
        JUCE_WEB_BROWSER=0
)

target_link_libraries(AviatorKeyzTests
    PRIVATE
        AviatorKeyzData
        juce::juce_audio_basics
        juce::juce_audio_formats
        juce::juce_audio_processors
        juce::juce_dsp
        juce::juce_core
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
)
