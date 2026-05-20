# =============================================================================
#  AviatorKeyz — C++ unit tests
#
#  Build:  cmake --build build --target AviatorKeyzTests
#  Run:    ./build/Tests/AviatorKeyzTests_artefacts/AviatorKeyzTests   (macOS, Ninja)
#
#  What is tested here:
#    - SamplerEngine DSP (AudioSnapshot API)
#    - SampleLibrary load/publish (JUCE 8)
#    - StateSchema constants
# =============================================================================

juce_add_console_app(AviatorKeyzTests
    PRODUCT_NAME "AviatorKeyz Unit Tests"
)

target_sources(AviatorKeyzTests
    PRIVATE
        AviatorKeyzTests.cpp
        SamplerEngineTests.cpp
        SampleLibraryTests.cpp
        StateSchemaTests.cpp
        ${CMAKE_SOURCE_DIR}/Source/DSP/SamplerEngine.cpp
        ${CMAKE_SOURCE_DIR}/Source/DSP/ReversePlayer.cpp
        ${CMAKE_SOURCE_DIR}/Source/State/SampleLibrary.cpp
)

target_include_directories(AviatorKeyzTests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/Source
)

target_compile_definitions(AviatorKeyzTests
    PRIVATE
        JUCE_STANDALONE_APPLICATION=1
        JUCE_USE_CURL=0
        JUCE_WEB_BROWSER=0
)

target_link_libraries(AviatorKeyzTests
    PRIVATE
        juce::juce_audio_basics
        juce::juce_audio_formats
        juce::juce_dsp
        juce::juce_core
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
)
