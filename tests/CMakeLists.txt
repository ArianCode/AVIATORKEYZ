# =============================================================================
#  AviatorKeyz — C++ unit tests
#
#  Build:  cmake --build build --target AviatorKeyzTests
#  Run:    ./build/tests/AviatorKeyzTests_artefacts/Release/AviatorKeyzTests
#
#  Sanitizer build (tests-only tree):
#    cmake -S . -B build-asan -G Ninja -DCMAKE_BUILD_TYPE=Debug \
#      -DAVIATORKEYZ_BUILD_PLUGIN=OFF -DAVIATORKEYZ_BUILD_TESTS=ON \
#      -DAVIATORKEYZ_ENABLE_SANITIZERS=ON
#    cmake --build build-asan --target AviatorKeyzSanitizerTests
# =============================================================================

if(AVIATORKEYZ_ENABLE_SANITIZERS)
    set(_AVIATORKEYZ_TEST_TARGET AviatorKeyzSanitizerTests)
    set(_AVIATORKEYZ_TEST_PRODUCT "AviatorKeyz Sanitizer Tests")
else()
    set(_AVIATORKEYZ_TEST_TARGET AviatorKeyzTests)
    set(_AVIATORKEYZ_TEST_PRODUCT "AviatorKeyz Unit Tests")
endif()

juce_add_console_app(${_AVIATORKEYZ_TEST_TARGET}
    PRODUCT_NAME "${_AVIATORKEYZ_TEST_PRODUCT}"
)

target_sources(${_AVIATORKEYZ_TEST_TARGET}
    PRIVATE
        AviatorKeyzTests.cpp
        ApvtsStateHelpersTests.cpp
        FastMathTests.cpp
        RealtimeSafetyTests.cpp
        SamplerEngineTests.cpp
        GlideEngineTests.cpp
        SampleLibraryTests.cpp
        StateSchemaTests.cpp
        ParameterWiringTests.cpp
        TextureEngineTests.cpp
        PerformanceMacroEngineTests.cpp
        MacroMapperTests.cpp
        MacroMapperTests.cpp
        PresetManagerTests.cpp
        CategoryPolicyTests.cpp
        PitchAlignmentTests.cpp
        PerformanceBenchmarkTests.cpp
        PresetPlaybackTests.cpp
        RawSamplerTests.cpp
        VoiceLifecycleTests.cpp
        MidiHandlerTests.cpp
        ${CMAKE_SOURCE_DIR}/Source/DSP/SamplerEngine.cpp
        ${CMAKE_SOURCE_DIR}/Source/DSP/GlideEngine.cpp
        ${CMAKE_SOURCE_DIR}/Source/DSP/FilterProcessor.cpp
        ${CMAKE_SOURCE_DIR}/Source/DSP/SynthEngine.cpp
        ${CMAKE_SOURCE_DIR}/Source/DSP/TextureEngine.cpp
        ${CMAKE_SOURCE_DIR}/Source/DSP/ReversePlayer.cpp
        ${CMAKE_SOURCE_DIR}/Source/DSP/PitchProbe.cpp
        ${CMAKE_SOURCE_DIR}/Source/DSP/ModMatrix.cpp
        ${CMAKE_SOURCE_DIR}/Source/DSP/LfoEngine.cpp
        ${CMAKE_SOURCE_DIR}/Source/DSP/PerformanceMacroEngine.cpp
        ${CMAKE_SOURCE_DIR}/Source/DSP/Performance/PerformanceApvtsReader.cpp
        ${CMAKE_SOURCE_DIR}/Source/DSP/Performance/MacroMapper.cpp
        ${CMAKE_SOURCE_DIR}/Source/DSP/Performance/PhraseChopper.cpp
        ${CMAKE_SOURCE_DIR}/Source/DSP/Performance/MotionEngine.cpp
        ${CMAKE_SOURCE_DIR}/Source/DSP/ToneShaper.cpp
        ${CMAKE_SOURCE_DIR}/Source/State/SampleLibrary.cpp
        ${CMAKE_SOURCE_DIR}/Source/State/PresetManager.cpp
        ${CMAKE_SOURCE_DIR}/Source/State/MacroPresetParser.cpp
        ${CMAKE_SOURCE_DIR}/Source/State/FactoryResources.cpp
        ${CMAKE_SOURCE_DIR}/Source/State/ParameterLayout.cpp
        ${CMAKE_SOURCE_DIR}/Source/State/ApvtsStateHelpers.cpp
        ${CMAKE_SOURCE_DIR}/Source/GUI/Advanced/AdvancedParameterLayout.cpp
        ${CMAKE_SOURCE_DIR}/Source/GUI/Advanced/PerformanceParameterLayout.cpp
        ${CMAKE_SOURCE_DIR}/Source/MIDI/MidiHandler.cpp
)

target_include_directories(${_AVIATORKEYZ_TEST_TARGET}
    PRIVATE
        ${CMAKE_SOURCE_DIR}/Source
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_definitions(${_AVIATORKEYZ_TEST_TARGET}
    PRIVATE
        JUCE_STANDALONE_APPLICATION=1
        JUCE_USE_CURL=0
        JUCE_WEB_BROWSER=0
)

target_link_libraries(${_AVIATORKEYZ_TEST_TARGET}
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

aviatorkeyz_apply_sanitizers_to_test_target(${_AVIATORKEYZ_TEST_TARGET})

# Convenience alias so existing scripts can keep using AviatorKeyzTests when not sanitizing.
if(AVIATORKEYZ_ENABLE_SANITIZERS)
    add_custom_target(AviatorKeyzTests DEPENDS AviatorKeyzSanitizerTests)
endif()
