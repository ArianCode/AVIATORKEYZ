cmake_minimum_required(VERSION 3.22)

# =============================================================================
#  AviatorKeyz — Premium Sample-Based VST3 Instrument
#  Build system: CMake + JUCE (fetched automatically)
#  Targets: VST3  |  Platforms: Windows, macOS
# =============================================================================

project(AviatorKeyz VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---------------------------------------------------------------------------
# Options
# ---------------------------------------------------------------------------
option(AVIATORKEYZ_COPY_AFTER_BUILD "Copy VST3 to system plugin folder after build" OFF)

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
# Regenerate assets: python3 scripts/generate_factory_assets.py
# ---------------------------------------------------------------------------
file(GLOB_RECURSE AVIATORKEYZ_FACTORY_PRESETS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/Resources/Presets/Factory/*.xml")
file(GLOB AVIATORKEYZ_FACTORY_SAMPLES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/Resources/Factory/*.wav")

juce_add_binary_data(AviatorKeyzData SOURCES
    ${AVIATORKEYZ_FACTORY_SAMPLES}
    ${AVIATORKEYZ_FACTORY_PRESETS}
    HEADER_NAME BinaryData.h
    NAMESPACE AviatorKeyzBinary
)

# ---------------------------------------------------------------------------
# Plugin target
# ---------------------------------------------------------------------------
juce_add_plugin(AviatorKeyz
    # Identity
    COMPANY_NAME                "AviatorKeyz"
    COMPANY_WEBSITE             "https://aviatorkeyz.com"
    COMPANY_EMAIL               ""

    # VST3 metadata — PLUGIN_CODE must be unique 4-char code (uppercase)
    # WARNING: Do NOT change these after shipping — hosts store them in projects
    PLUGIN_MANUFACTURER_CODE    "Avkz"
    PLUGIN_CODE                 "Avk1"
    PLUGIN_NAME                 "AviatorKeyz"
    PRODUCT_NAME                "AviatorKeyz"

    # Format — VST3 ships; Standalone is the fast dev loop (Milestone 0)
    FORMATS                     VST3 Standalone

    # Capabilities
    IS_SYNTH                    TRUE
    NEEDS_MIDI_INPUT            TRUE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE

    # VST3 category — must be "Instrument|Synth" to appear correctly in FL Studio
    VST3_CATEGORIES             "Instrument|Synth"

    # Post-build copy (set AVIATORKEYZ_COPY_AFTER_BUILD=ON to enable)
    COPY_PLUGIN_AFTER_BUILD     ${AVIATORKEYZ_COPY_AFTER_BUILD}
)

# ---------------------------------------------------------------------------
# Source files
# Grouped by module. Add new files here as modules are introduced.
# ---------------------------------------------------------------------------
target_sources(AviatorKeyz
    PRIVATE
        # Core plugin
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp

        # State layer
        Source/State/PresetManager.cpp
        Source/State/FactoryResources.cpp
        Source/State/SampleLibrary.cpp

        # DSP layer (stubs — filled in during M1/M2)
        Source/DSP/SamplerEngine.cpp
        Source/DSP/ToneShaper.cpp
        Source/DSP/SmearProcessor.cpp
        Source/DSP/GlideEngine.cpp
        Source/DSP/ReverbTail.cpp
        Source/DSP/ReversePlayer.cpp

        # GUI layer (stubs — filled in during M3/M4)
        Source/GUI/LuxuryLookAndFeel.cpp
        Source/GUI/MainPanel.cpp
        Source/GUI/HeaderBar.cpp
        Source/GUI/KnobComponent.cpp
        Source/GUI/WaveformDisplay.cpp
        Source/GUI/PresetBrowser.cpp

        # MIDI layer
        Source/MIDI/MidiHandler.cpp
)

# ---------------------------------------------------------------------------
# Include paths
# ---------------------------------------------------------------------------
target_include_directories(AviatorKeyz
    PRIVATE
        Source
)

# ---------------------------------------------------------------------------
# Compile definitions
# ---------------------------------------------------------------------------
target_compile_definitions(AviatorKeyz
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
)

# ---------------------------------------------------------------------------
# Link JUCE modules
# ---------------------------------------------------------------------------
target_link_libraries(AviatorKeyz
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
    target_compile_options(AviatorKeyz PRIVATE /W4 /WX-)
endif()

# ---------------------------------------------------------------------------
# Install target (copies VST3 bundle to standard location for manual use)
# Usage: cmake --install build/ --prefix /path/to/vst3/folder
# ---------------------------------------------------------------------------
install(
    DIRECTORY "$<TARGET_FILE_DIR:AviatorKeyz_VST3>/../"
    DESTINATION "AviatorKeyz.vst3"
    USE_SOURCE_PERMISSIONS
)
