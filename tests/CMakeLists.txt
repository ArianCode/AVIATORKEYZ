# =============================================================================
#  AviatorKeyz — C++ unit tests
#
#  Build:  cmake --build build --target AviatorKeyzTests
#  Run:    ./build/Tests/AviatorKeyzTests          (macOS/Linux)
#          build\Tests\Release\AviatorKeyzTests.exe  (Windows)
#
#  What is tested here:
#    - SamplerEngine DSP math (setSampleTable API, committed version)
#    - StateSchema constants (compile-time + runtime value checks)
#
#  What is NOT tested here (tested in Python tests/ instead):
#    - SampleLibrary: committed SampleLibrary.cpp uses JUCE 7 API
#      (createReaderFor(&stream) raw pointer) that was removed in JUCE 8.
#      The working-tree fix (std::make_unique<MemoryInputStream>) is not yet
#      committed. Once committed, add SampleLibraryTests.cpp back here:
#        ${CMAKE_SOURCE_DIR}/Source/State/SampleLibrary.cpp
#    - PresetManager / FactoryResources: depend on BinaryData (generated assets)
#      and juce_audio_processors — covered by Python tests.
#
#  NOTE: SampleLibraryTests.cpp is included in the source list below
#  but its body is guarded with #if COMPILE_SAMPLE_LIBRARY_TESTS.
#  Remove the guard once SampleLibrary.cpp is updated to JUCE 8 API.
# =============================================================================

juce_add_console_app(AviatorKeyzTests
    PRODUCT_NAME "AviatorKeyz Unit Tests"
)

target_sources(AviatorKeyzTests
    PRIVATE
        AviatorKeyzTests.cpp
        SamplerEngineTests.cpp
        StateSchemaTests.cpp
        # SampleLibraryTests.cpp — re-enable when SampleLibrary.cpp uses JUCE 8 API
        # DSP sources included directly — avoids JucePlugin_xxx macro requirements
        ${CMAKE_SOURCE_DIR}/Source/DSP/SamplerEngine.cpp
        ${CMAKE_SOURCE_DIR}/Source/DSP/ReversePlayer.cpp
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
        juce::juce_dsp
        juce::juce_core
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
)
