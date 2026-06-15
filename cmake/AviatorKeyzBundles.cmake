# =============================================================================
#  Production bundle finalization (sign + verify after JUCE assembly)
# =============================================================================

set(AVIATORKEYZ_CODESIGN_IDENTITY "" CACHE STRING
    "macOS codesign identity for production bundles. Empty = ad-hoc (-). Set to Developer ID for release distribution.")

function(aviatorkeyz_bundle_path_for_target target_name out_var)
    if(NOT TARGET ${target_name})
        set(${out_var} "" PARENT_SCOPE)
        return()
    endif()
    if(${target_name} STREQUAL "AviatorKeyz_VST3")
        get_target_property(_bundle_dir ${target_name} JUCE_PLUGIN_ARTEFACTS_DIR)
        if(_bundle_dir)
            set(${out_var} "${_bundle_dir}/VST3/AviatorKeyz.vst3" PARENT_SCOPE)
        else()
            set(${out_var} "$<TARGET_BUNDLE_DIR:${target_name}>" PARENT_SCOPE)
        endif()
    elseif(${target_name} STREQUAL "AviatorKeyz_AU")
        get_target_property(_bundle_dir ${target_name} JUCE_PLUGIN_ARTEFACTS_DIR)
        if(_bundle_dir)
            set(${out_var} "${_bundle_dir}/AU/AviatorKeyz.component" PARENT_SCOPE)
        else()
            set(${out_var} "$<TARGET_BUNDLE_DIR:${target_name}>" PARENT_SCOPE)
        endif()
    elseif(${target_name} STREQUAL "AviatorKeyz_Standalone")
        set(${out_var} "$<TARGET_BUNDLE_DIR:${target_name}>" PARENT_SCOPE)
    else()
        set(${out_var} "" PARENT_SCOPE)
    endif()
endfunction()

function(aviatorkeyz_add_production_bundle_finalization target_name)
    if(NOT APPLE)
        return()
    endif()
    if(NOT TARGET ${target_name})
        return()
    endif()

    set(_finalize_script "${CMAKE_SOURCE_DIR}/scripts/finalize_production_bundle.sh")
    if(NOT EXISTS "${_finalize_script}")
        message(WARNING "Missing ${_finalize_script}; skipping bundle finalization for ${target_name}")
        return()
    endif()

    if(${target_name} STREQUAL "AviatorKeyz_VST3")
        set(_bundle "$<TARGET_BUNDLE_DIR:${target_name}>")
    elseif(${target_name} STREQUAL "AviatorKeyz_AU")
        set(_bundle "$<TARGET_BUNDLE_DIR:${target_name}>")
    elseif(${target_name} STREQUAL "AviatorKeyz_Standalone")
        set(_bundle "$<TARGET_BUNDLE_DIR:${target_name}>")
    else()
        return()
    endif()

    add_custom_command(TARGET ${target_name} POST_BUILD
        COMMAND "${_finalize_script}"
                "${_bundle}"
                "$<TARGET_FILE:${target_name}>"
                "${target_name}"
                "${AVIATORKEYZ_CODESIGN_IDENTITY}"
        COMMENT "Finalizing ${target_name} bundle (sign + verify)"
        VERBATIM
    )
endfunction()
