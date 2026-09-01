# Generates <build>/generated/BuildInfo.h with git identity and prototype version label.
#
# NOTE: this is captured at CONFIGURE time. After committing/tagging an RC you MUST
# reconfigure (or delete the build tree) or the binary will carry a stale SHA.
# scripts/package_prototype_macos.sh and build_windows.bat use a fresh tree for this reason.

set(AVIATORKEYZ_PROTOTYPE_VERSION "0.1.0-rc1" CACHE STRING "Prototype release label shown in UI and VERSION.txt")
set(AVIATORKEYZ_JUCE_VERSION "8.0.9")

set(_AVIATORKEYZ_GIT_SHA "unknown")
set(_AVIATORKEYZ_GIT_TAG "untagged")
set(_AVIATORKEYZ_GIT_DIRTY "")

find_program(_AVIATORKEYZ_GIT_EXECUTABLE git)
if(_AVIATORKEYZ_GIT_EXECUTABLE)
    execute_process(
        COMMAND "${_AVIATORKEYZ_GIT_EXECUTABLE}" rev-parse --short=10 HEAD
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        OUTPUT_VARIABLE _AVIATORKEYZ_GIT_SHA
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )

    execute_process(
        COMMAND "${_AVIATORKEYZ_GIT_EXECUTABLE}" describe --tags --always --dirty
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        OUTPUT_VARIABLE _AVIATORKEYZ_GIT_TAG
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )

    # Uncommitted changes make the SHA non-reproducible — mark the binary loudly.
    execute_process(
        COMMAND "${_AVIATORKEYZ_GIT_EXECUTABLE}" status --porcelain --untracked-files=no
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        OUTPUT_VARIABLE _AVIATORKEYZ_GIT_STATUS
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    if(NOT _AVIATORKEYZ_GIT_STATUS STREQUAL "")
        set(_AVIATORKEYZ_GIT_DIRTY "-dirty")
    endif()
endif()

set(AVIATORKEYZ_GIT_SHA "${_AVIATORKEYZ_GIT_SHA}${_AVIATORKEYZ_GIT_DIRTY}")
set(AVIATORKEYZ_GIT_TAG "${_AVIATORKEYZ_GIT_TAG}")

# A release artifact must never be cut from a dirty tree: the recorded SHA would not
# rebuild the binary the client is running. Opt out only for local experiments.
option(AVIATORKEYZ_REQUIRE_CLEAN_TREE "Fail configure if the git working tree is dirty" OFF)
if(AVIATORKEYZ_REQUIRE_CLEAN_TREE AND NOT _AVIATORKEYZ_GIT_DIRTY STREQUAL "")
    message(FATAL_ERROR
        "Working tree is dirty — refusing to configure a release build.\n"
        "Commit or stash before cutting an RC, or configure with -DAVIATORKEYZ_REQUIRE_CLEAN_TREE=OFF.")
endif()

message(STATUS "Aviation build identity: ${AVIATORKEYZ_PROTOTYPE_VERSION} / ${AVIATORKEYZ_GIT_SHA} / ${AVIATORKEYZ_GIT_TAG}")

set(_AVIATORKEYZ_BUILD_INFO_DIR "${CMAKE_BINARY_DIR}/generated")
file(MAKE_DIRECTORY "${_AVIATORKEYZ_BUILD_INFO_DIR}")
configure_file(
    "${CMAKE_SOURCE_DIR}/cmake/BuildInfo.h.in"
    "${_AVIATORKEYZ_BUILD_INFO_DIR}/BuildInfo.h"
    @ONLY
)

function(aviatorkeyz_apply_build_info_include target)
    target_include_directories(${target} PRIVATE "${_AVIATORKEYZ_BUILD_INFO_DIR}")
endfunction()
