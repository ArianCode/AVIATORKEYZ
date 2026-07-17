cmake_minimum_required(VERSION 3.22)

# =============================================================================
#  AviatorKeyz — Premium Sample-Based VST3 Instrument
#  Build system: CMake + JUCE (fetched automatically)
#  Targets: VST3 + Standalone  |  Primary platform: macOS (Windows secondary)
# =============================================================================

project(AviatorKeyz VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(cmake/AviatorKeyzSanitizers.cmake)
include(cmake/AviatorKeyzBundles.cmake)
aviatorkeyz_reject_global_sanitizer_flags()

# ---------------------------------------------------------------------------
# Options
# ---------------------------------------------------------------------------
option(AVIATORKEYZ_COPY_AFTER_BUILD "Copy VST3 to system plugin folder after build" OFF)
option(AVIATORKEYZ_PROFILE "Enable audio callback microsecond profiling (non-release diagnostics)" OFF)

aviatorkeyz_configure_sanitizer_mode()

# ---------------------------------------------------------------------------
# JUCE — fetched via CMake FetchContent (no manual install required)
# Pin to a specific tag for reproducible builds.
# To upgrade: change the GIT_TAG and clear your build cache.
# ---------------------------------------------------------------------------
include(FetchContent)

FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    # JUCE 8+ required for macOS 15 SDK (juceaide / CGWindowListCreateImage)
    GIT_TAG        8.0.9
    GIT_SHALLOW    TRUE
    GIT_PROGRESS   TRUE
)

FetchContent_MakeAvailable(JUCE)

# ---------------------------------------------------------------------------
# Embedded factory assets (wav + preset XML). See StateSchema.h "Content pipeline".
# Placeholders: python3 scripts/generate_factory_assets.py
# Licensed bank: python3 scripts/import_factory_bank.py --copy-only --clean
# ---------------------------------------------------------------------------
file(GLOB_RECURSE AVIATORKEYZ_FACTORY_PRESETS CONFIGURE_DEPENDS
    LIST_DIRECTORIES false
    "${CMAKE_CURRENT_SOURCE_DIR}/Resources/Presets/Factory/*/*.xml")
file(GLOB AVIATORKEYZ_FACTORY_SAMPLES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/Resources/Factory/*.wav")
file(GLOB AVIATORKEYZ_COCKPIT_UI CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/Resources/UI/Cockpit/*.*")

juce_add_binary_data(AviatorKeyzData SOURCES
    ${AVIATORKEYZ_FACTORY_SAMPLES}
    ${AVIATORKEYZ_FACTORY_PRESETS}
    ${AVIATORKEYZ_COCKPIT_UI}
    HEADER_NAME BinaryData.h
    NAMESPACE AviatorKeyzBinary
)

if(AVIATORKEYZ_BUILD_PLUGIN)
    # ---------------------------------------------------------------------------
    # Plugin target
    # ---------------------------------------------------------------------------
    juce_add_plugin(Aviation
        # Identity
        COMPANY_NAME                "Aviation"
        COMPANY_WEBSITE             "https://aviatorkeyz.com"
        COMPANY_EMAIL               ""

        # VST3 metadata — PLUGIN_CODE must be unique 4-char code (uppercase)
        # WARNING: Do NOT change these after shipping — hosts store them in projects
        PLUGIN_MANUFACTURER_CODE    "Avkz"
        PLUGIN_CODE                 "Avk1"
        PLUGIN_NAME                 "Aviation"
        PRODUCT_NAME                "Aviation"

        # Formats — cross-platform VST3 + macOS AU, with Standalone for fast UI iteration
        FORMATS                     VST3 AU Standalone

        # Capabilities
        IS_SYNTH                    TRUE
        NEEDS_MIDI_INPUT            TRUE
        NEEDS_MIDI_OUTPUT           FALSE
        IS_MIDI_EFFECT              FALSE
        EDITOR_WANTS_KEYBOARD_FOCUS FALSE

        # VST3 category — must be "Instrument|Synth" (Logic, Ableton, FL Studio, etc.)
        VST3_CATEGORIES             "Instrument|Synth"

        # Post-build copy (set AVIATORKEYZ_COPY_AFTER_BUILD=ON to enable)
        COPY_PLUGIN_AFTER_BUILD     ${AVIATORKEYZ_COPY_AFTER_BUILD}
    )

    # ---------------------------------------------------------------------------
    # Source files
    # Grouped by module. Add new files here as modules are introduced.
    # ---------------------------------------------------------------------------
    target_sources(Aviation
        PRIVATE
            # Core plugin
            Source/PluginProcessor.cpp
            Source/PluginEditor.cpp

            # State layer
            Source/State/PresetManager.cpp
            Source/State/ApvtsStateHelpers.cpp
            Source/State/FactoryResources.cpp
            Source/State/SampleLibrary.cpp
            Source/State/ParameterLayout.cpp

            # DSP layer (stubs — filled in during M1/M2)
            Source/DSP/SamplerEngine.cpp
            Source/DSP/SynthEngine.cpp
            Source/DSP/FilterProcessor.cpp
            Source/DSP/TextureEngine.cpp
            Source/DSP/OutputLimiter.cpp
            Source/DSP/PerformanceMacroEngine.cpp
            Source/DSP/Performance/PerformanceApvtsReader.cpp
            Source/DSP/Performance/MacroMapper.cpp
            Source/DSP/Performance/PhraseChopper.cpp
            Source/DSP/Performance/MotionEngine.cpp
            Source/DSP/Performance/TextureBlendEngine.cpp
            Source/DSP/Performance/PerformanceFxEngine.cpp
            Source/DSP/Performance/PerformanceTexturePipeline.cpp
            Source/State/MacroPresetParser.cpp
            Source/DSP/ToneShaper.cpp
            Source/DSP/SmearProcessor.cpp
            Source/DSP/GlideEngine.cpp
            Source/DSP/ReverbTail.cpp
            Source/DSP/LfoEngine.cpp
            Source/DSP/FxChain.cpp
            Source/DSP/ModMatrix.cpp
            Source/DSP/ReversePlayer.cpp
            Source/DSP/PitchProbe.cpp

            # GUI — photo-anchored cockpit
            Source/GUI/LuxuryLookAndFeel.cpp
            Source/GUI/MainPanel.cpp
            Source/GUI/FooterBar.cpp
            Source/GUI/ViewModeTabBar.cpp
            Source/GUI/PrecisionKnob.cpp
            Source/GUI/ReverseToggle.cpp
            Source/GUI/PresetBrowser.cpp
            Source/GUI/Cockpit/CockpitZones.cpp
            Source/GUI/Cockpit/CockpitPhotoBackground.cpp
            Source/GUI/Cockpit/PhotoAnchoredKnob.cpp
            Source/GUI/Cockpit/CockpitCrossworldPanel.cpp
            Source/GUI/Cockpit/AviationGauge.cpp
            Source/GUI/Cockpit/InstrumentPanelBar.cpp
            Source/GUI/Cockpit/PresetHorizonReadout.cpp
            Source/GUI/Cockpit/CockpitTopPresetBar.cpp
            Source/GUI/Cockpit/PresetNavButton.cpp
            Source/GUI/Cockpit/PresetCenterNavigator.cpp
            Source/GUI/Cockpit/PresetSearchOverlay.cpp
            Source/GUI/Cockpit/PresetLibraryOverlay.cpp
            Source/GUI/Cockpit/AboutOverlay.cpp
            Source/GUI/Cockpit/PresetIconButtons.cpp
            Source/GUI/Cockpit/CockpitPresetControlBar.cpp
            Source/GUI/Cockpit/SearchIconButton.cpp

            Source/GUI/Advanced/AdvancedPanel.cpp
            Source/GUI/Advanced/AdvancedPageContent.cpp
            Source/GUI/Advanced/AdvancedParameterLayout.cpp
            Source/GUI/Advanced/PerformanceParameterLayout.cpp
            Source/GUI/Advanced/AdvancedPresetSidebar.cpp
            Source/GUI/Advanced/BracketValueBox.cpp
            Source/GUI/Advanced/EffectCell.cpp
            Source/GUI/Advanced/EffectCellChoice.cpp
            Source/GUI/Advanced/EffectCellFormat.cpp
            Source/GUI/Advanced/EffectCellGrid.cpp
            Source/GUI/Advanced/EffectCellShell.cpp
            Source/GUI/Advanced/EffectParamRow.cpp
            Source/GUI/Advanced/EffectParamBox.cpp
            Source/GUI/Advanced/TextureVisualizerComponent.cpp
            Source/GUI/Advanced/PerformanceMacroStrip.cpp
            Source/GUI/Advanced/ModAmountSlider.cpp
            Source/GUI/Advanced/OscWaveformDisplay.cpp
            Source/GUI/Advanced/FilterCurveGraph.cpp
            Source/GUI/Advanced/LfoWaveformDisplay.cpp
            Source/GUI/Advanced/ModRoutingHub.cpp
            Source/GUI/Advanced/ModAssignCallout.cpp

            # MIDI layer
            Source/MIDI/MidiHandler.cpp
    )

    # ---------------------------------------------------------------------------
    # Include paths
    # ---------------------------------------------------------------------------
    target_include_directories(Aviation
        PRIVATE
            Source
    )

    # ---------------------------------------------------------------------------
    # Compile definitions
    # ---------------------------------------------------------------------------
    target_compile_definitions(Aviation
        PUBLIC
            # Disable unused JUCE features (faster builds, smaller binary)
            JUCE_WEB_BROWSER=0
            JUCE_USE_CURL=0
            JUCE_VST3_CAN_REPLACE_VST2=0

            # Disable splash screen (requires JUCE license or GPL)
            JUCE_DISPLAY_SPLASH_SCREEN=0
            JUCE_REPORT_APP_USAGE=0

            # Enable assert-based checks in debug only
            $<$<CONFIG:Debug>:AVIATORKEYZ_DEBUG=1>
            $<$<BOOL:${AVIATORKEYZ_PROFILE}>:AVIATORKEYZ_PROFILE=1>
    )

    # ---------------------------------------------------------------------------
    # Link JUCE modules
    # ---------------------------------------------------------------------------
    target_link_libraries(Aviation
        PRIVATE
            AviatorKeyzData
            juce::juce_audio_utils
            juce::juce_audio_processors
            juce::juce_audio_formats
            juce::juce_dsp
            juce::juce_gui_extra
            juce::juce_gui_basics
        PUBLIC
            juce::juce_recommended_config_flags
            juce::juce_recommended_lto_flags
            juce::juce_recommended_warning_flags
    )

    # ---------------------------------------------------------------------------
    # MSVC-specific flags
    # ---------------------------------------------------------------------------
    if(MSVC)
        target_compile_options(Aviation PRIVATE /W4 /WX-)
    endif()

    aviatorkeyz_verify_production_targets_clean()
    aviatorkeyz_add_production_bundle_finalization(Aviation_VST3)
    aviatorkeyz_add_production_bundle_finalization(Aviation_AU)
    aviatorkeyz_add_production_bundle_finalization(Aviation_Standalone)

    # ---------------------------------------------------------------------------
    # Install target (copies VST3 bundle to standard location for manual use)
    # Usage: cmake --install build/ --prefix /path/to/vst3/folder
    # ---------------------------------------------------------------------------
    install(
        DIRECTORY "$<TARGET_FILE_DIR:Aviation_VST3>/../"
        DESTINATION "Aviation.vst3"
        USE_SOURCE_PERMISSIONS
    )
endif()

# ---------------------------------------------------------------------------
# C++ unit tests (optional, excluded from default all-target build)
#
# Build:   cmake --build build --target AviatorKeyzTests
# Run:     ./build/tests/AviatorKeyzTests_artefacts/Release/AviatorKeyzTests   (macOS, Ninja)
#          build/tests/AviatorKeyzTests_artefacts/AviatorKeyzTests.exe (Windows)
# ---------------------------------------------------------------------------
if(AVIATORKEYZ_BUILD_TESTS)
    add_subdirectory(tests EXCLUDE_FROM_ALL)
endif()

aviatorkeyz_verify_production_targets_clean()
