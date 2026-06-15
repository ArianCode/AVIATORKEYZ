# =============================================================================
#  AviatorKeyz — sanitizer configuration (test targets only)
#
#  Production plugin bundles must never link sanitizer runtimes. When an
#  ASan-instrumented plugin is dlopen'd into an already-running non-ASan host,
#  interceptor verification may abort before plugin init. Linked-binary
#  verification remains the authoritative guard.
# =============================================================================

option(AVIATORKEYZ_ENABLE_SANITIZERS
    "Enable AddressSanitizer + UndefinedBehaviorSanitizer on approved test targets only"
    OFF)

option(AVIATORKEYZ_BUILD_PLUGIN
    "Build production plugin bundles (VST3/AU/Standalone)"
    ON)

option(AVIATORKEYZ_BUILD_TESTS
    "Build C++ unit test target(s)"
    ON)

set(_AVIATORKEYZ_PRODUCTION_TARGETS
    AviatorKeyz
    AviatorKeyz_VST3
    AviatorKeyz_AU
    AviatorKeyz_Standalone
)

function(_aviatorkeyz_string_contains_sanitize out_var input)
    if("${input}" MATCHES "(-fsanitize|-sanitize=)")
        set(${out_var} TRUE PARENT_SCOPE)
    else()
        set(${out_var} FALSE PARENT_SCOPE)
    endif()
endfunction()

function(_aviatorkeyz_option_list_contains_sanitize opts result_var)
    set(_found FALSE)
    if(NOT ${opts})
        set(${result_var} FALSE PARENT_SCOPE)
        return()
    endif()
    if(${opts} STREQUAL "NOTFOUND")
        set(${result_var} FALSE PARENT_SCOPE)
        return()
    endif()
    foreach(_opt IN LISTS ${opts})
        _aviatorkeyz_string_contains_sanitize(_hit "${_opt}")
        if(_hit)
            set(_found TRUE)
            break()
        endif()
    endforeach()
    set(${result_var} ${_found} PARENT_SCOPE)
endfunction()

function(aviatorkeyz_reject_global_sanitizer_flags)
    foreach(_var IN ITEMS
        CMAKE_C_FLAGS CMAKE_CXX_FLAGS
        CMAKE_C_FLAGS_DEBUG CMAKE_CXX_FLAGS_DEBUG
        CMAKE_C_FLAGS_RELEASE CMAKE_CXX_FLAGS_RELEASE
        CMAKE_C_FLAGS_RELWITHDEBINFO CMAKE_CXX_FLAGS_RELWITHDEBINFO
        CMAKE_C_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_MINSIZEREL
        CMAKE_EXE_LINKER_FLAGS CMAKE_SHARED_LINKER_FLAGS CMAKE_MODULE_LINKER_FLAGS
        CMAKE_EXE_LINKER_FLAGS_DEBUG CMAKE_SHARED_LINKER_FLAGS_DEBUG
        CMAKE_EXE_LINKER_FLAGS_RELEASE CMAKE_SHARED_LINKER_FLAGS_RELEASE
        CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO CMAKE_SHARED_LINKER_FLAGS_RELWITHDEBINFO
        CMAKE_MODULE_LINKER_FLAGS_DEBUG CMAKE_MODULE_LINKER_FLAGS_RELEASE
        CMAKE_MODULE_LINKER_FLAGS_RELWITHDEBINFO CMAKE_MODULE_LINKER_FLAGS_MINSIZEREL
        CMAKE_C_FLAGS_INIT CMAKE_CXX_FLAGS_INIT
        CMAKE_EXE_LINKER_FLAGS_INIT CMAKE_SHARED_LINKER_FLAGS_INIT CMAKE_MODULE_LINKER_FLAGS_INIT
    )
        if(DEFINED ${_var})
            _aviatorkeyz_string_contains_sanitize(_hit "${${_var}}")
            if(_hit)
                message(FATAL_ERROR
                    "Global sanitizer flags detected in ${_var}=${${_var}}. "
                    "Remove them from the environment or toolchain. "
                    "Use -DAVIATORKEYZ_ENABLE_SANITIZERS=ON with -DAVIATORKEYZ_BUILD_PLUGIN=OFF for test targets only.")
            endif()
        endif()
    endforeach()

    foreach(_env_var IN ITEMS CFLAGS CXXFLAGS LDFLAGS CPPFLAGS)
        if(DEFINED ENV{${_env_var}} AND NOT "$ENV{${_env_var}}" STREQUAL "")
            _aviatorkeyz_string_contains_sanitize(_hit "$ENV{${_env_var}}")
            if(_hit)
                message(FATAL_ERROR
                    "Sanitizer flags detected in environment variable ${_env_var}=$ENV{${_env_var}}. "
                    "Unset it before configuring production plugin builds.")
            endif()
        endif()
    endforeach()

    get_directory_property(_dir_compile_options COMPILE_OPTIONS)
    get_directory_property(_dir_link_options LINK_OPTIONS)
    _aviatorkeyz_option_list_contains_sanitize(_dir_compile_options _dir_compile_hit)
    _aviatorkeyz_option_list_contains_sanitize(_dir_link_options _dir_link_hit)
    if(_dir_compile_hit OR _dir_link_hit)
        message(FATAL_ERROR
            "Directory-level sanitizer compile/link options detected. "
            "Use target-local aviatorkeyz_apply_sanitizers_to_test_target() instead.")
    endif()
endfunction()

function(aviatorkeyz_configure_sanitizer_mode)
    if(NOT AVIATORKEYZ_ENABLE_SANITIZERS)
        return()
    endif()

    if(AVIATORKEYZ_BUILD_PLUGIN)
        message(FATAL_ERROR
            "AVIATORKEYZ_ENABLE_SANITIZERS=ON cannot be combined with AVIATORKEYZ_BUILD_PLUGIN=ON. "
            "Configure tests-only sanitizer builds with "
            "-DAVIATORKEYZ_BUILD_PLUGIN=OFF -DAVIATORKEYZ_BUILD_TESTS=ON -DAVIATORKEYZ_ENABLE_SANITIZERS=ON")
    endif()

    if(NOT AVIATORKEYZ_BUILD_TESTS)
        message(FATAL_ERROR
            "AVIATORKEYZ_ENABLE_SANITIZERS=ON requires AVIATORKEYZ_BUILD_TESTS=ON.")
    endif()

    if(AVIATORKEYZ_COPY_AFTER_BUILD)
        message(FATAL_ERROR
            "AVIATORKEYZ_COPY_AFTER_BUILD must remain OFF for sanitizer builds.")
    endif()

    set(AVIATORKEYZ_COPY_AFTER_BUILD OFF CACHE BOOL "Copy VST3 to system plugin folder after build" FORCE)
    message(STATUS "Sanitizer mode: tests-only (production plugin targets disabled).")
endfunction()

function(aviatorkeyz_apply_sanitizers_to_test_target target_name)
    if(NOT AVIATORKEYZ_ENABLE_SANITIZERS)
        return()
    endif()
    if(NOT TARGET ${target_name})
        message(FATAL_ERROR "Sanitizer test target '${target_name}' does not exist.")
    endif()
    if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang")
        message(WARNING "AVIATORKEYZ_ENABLE_SANITIZERS is ON but compiler is not Clang; skipping ${target_name}")
        return()
    endif()
    target_compile_options(${target_name} PRIVATE
        -fsanitize=address,undefined
        -fno-omit-frame-pointer
    )
    target_link_options(${target_name} PRIVATE
        -fsanitize=address,undefined
    )
endfunction()

function(aviatorkeyz_verify_production_targets_clean)
    if(NOT AVIATORKEYZ_BUILD_PLUGIN)
        return()
    endif()

    foreach(_target IN LISTS _AVIATORKEYZ_PRODUCTION_TARGETS)
        if(NOT TARGET ${_target})
            continue()
        endif()
        get_target_property(_compile_opts ${_target} COMPILE_OPTIONS)
        get_target_property(_link_opts ${_target} LINK_OPTIONS)
        _aviatorkeyz_option_list_contains_sanitize(_compile_opts _compile_hit)
        _aviatorkeyz_option_list_contains_sanitize(_link_opts _link_hit)
        if(_compile_hit OR _link_hit)
            message(FATAL_ERROR
                "Sanitizer flags are attached to production target '${_target}'. "
                "Production plugin bundles must not use -fsanitize. "
                "Enable sanitizers only on approved test targets via AVIATORKEYZ_ENABLE_SANITIZERS.")
        endif()
    endforeach()
endfunction()
