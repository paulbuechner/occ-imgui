# Copy target runtime dlls to the build directory by manually specifying the dlls to copy
macro(occ_imgui_copy_runtime_dlls_manual target)
    message(STATUS "OpenCASCADE_BINARY_DIR: ${OpenCASCADE_BINARY_DIR}")
    add_custom_command(
        TARGET ${target}
        POST_BUILD
        COMMAND
        $<$<CONFIG:DEBUG,RELEASE>:${CMAKE_COMMAND}> -E copy_directory
        $<$<CONFIG:DEBUG>:${OpenCASCADE_BINARY_DIR}d>$<$<NOT:$<CONFIG:DEBUG>>:${OpenCASCADE_BINARY_DIR}>
        $<TARGET_FILE_DIR:${target}>)
endmacro()

# Dynamically resolve target runtime dlls with $<TARGET_RUNTIME_DLLS:tgt>
# available with CMake 3.21+
# https://cmake.org/cmake/help/latest/manual/cmake-generator-expressions.7.html#genex:TARGET_RUNTIME_DLLS
#
# Not working currently: https://gitlab.kitware.com/cmake/cmake/-/issues/22845
macro(occ_imgui_copy_runtime_dlls_dynamic target)
    # Print runtime dlls
    add_custom_command(
        TARGET ${target}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "Runtime Dlls:
   $<TARGET_RUNTIME_DLLS:${target}>")

    # Copy dlls to build directory dynamically resolving target runtime dlls
    add_custom_command(
        TARGET ${target}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:${target}>
        $<TARGET_FILE_DIR:${target}>
        COMMAND_EXPAND_LISTS)
endmacro()

# Install MinGW target runtime dependencies (windows)
function(occ_imgui_install_target_deps_mingw)
    # Static link compiler libs for MinGW
    # set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libgcc -static-libstdc++ -static")

    # Install dynamically linked runtime dependencies
    #
    # Resolve compiler dependencies path from compiler path
    get_filename_component(MINGW_BIN_PATH ${CMAKE_CXX_COMPILER} DIRECTORY)
    option(MINGW_BIN_PATH "Path to MinGW runtime libraries. Usually the same as the compiler path." "${MINGW_BIN_PATH}")

    # Install the runtime dependencies
    install(CODE "set(MINGW_BIN_PATH \"${MINGW_BIN_PATH}\")")
    install(CODE [[
    file(GET_RUNTIME_DEPENDENCIES
         RESOLVED_DEPENDENCIES_VAR RESOLVED_DEPS
         UNRESOLVED_DEPENDENCIES_VAR UNRESOLVED_DEPS
         EXECUTABLES $<TARGET_FILE:occ-imgui>
         DIRECTORIES ${MINGW_BIN_PATH}
         PRE_EXCLUDE_REGEXES "api-ms-*" "ext-ms-*"
         POST_EXCLUDE_REGEXES ".*system32/.*\\.dll"
         )
    message(STATUS "Resolving runtime dependencies for $<TARGET_FILE:occ-imgui>")
    foreach(dep ${RESOLVED_DEPS})
      file(INSTALL ${dep} DESTINATION "${CMAKE_INSTALL_PREFIX}/bin")
    endforeach()
    foreach(dep ${UNRESOLVED_DEPS})
      message(WARNING "Runtime dependency ${dep} could not be resolved.")
    endforeach()
  ]])
endfunction()

# Install Linux target runtime dependencies
function(occ_imgui_install_target_deps_linux)
    install(CODE [[
    function (install_library_with_deps LIBRARY)
      file(INSTALL
           DESTINATION "${CMAKE_INSTALL_PREFIX}/lib"
           TYPE SHARED_LIBRARY
           FOLLOW_SYMLINK_CHAIN
           FILES "${LIBRARY}"
           )
      file(GET_RUNTIME_DEPENDENCIES
           LIBRARIES ${LIBRARY}
           RESOLVED_DEPENDENCIES_VAR RESOLVED_DEPS
           UNRESOLVED_DEPENDENCIES_VAR UNRESOLVED_DEPS
           )
      foreach (FILE ${RESOLVED_DEPS})
        if (NOT IS_SYMLINK ${FILE})
          install_library_with_deps(${FILE})
        endif ()
      endforeach ()
      foreach (FILE ${UNRESOLVED_DEPS})
        message(STATUS "Unresolved from ${LIBRARY}: ${FILE}")
      endforeach ()
    endfunction ()
    file(GET_RUNTIME_DEPENDENCIES
         EXECUTABLES $<TARGET_FILE:occ-imgui>
         RESOLVED_DEPENDENCIES_VAR RESOLVED_DEPS
         UNRESOLVED_DEPENDENCIES_VAR UNRESOLVED_DEPS
         )
    foreach (FILE ${RESOLVED_DEPS})
      install_library_with_deps(${FILE})
    endforeach ()
    foreach (FILE ${UNRESOLVED_DEPS})
      message(STATUS "Unresolved: ${FILE}")
    endforeach ()
  ]])
endfunction()

########################################################################
#
# Helper functions for creating build targets.

# local_occ_imgui_cxx_executable_with_flags(name cxx_flags libs srcs...)
#
# creates a named C++ executable that depends on the given libraries and
# is built from the given source files with the given compiler flags.
function(local_occ_imgui_cxx_executable_with_flags name cxx_flags libs)
    add_executable(${name} ${ARGN})
    if(cxx_flags)
        set_target_properties(${name}
                              PROPERTIES
                              COMPILE_FLAGS "${cxx_flags}")
    endif()

    # Make version information available to the target.
    # Replace dashes with underscores before uppercasing to create valid macro names
    string(REPLACE "-" "_" NAME_SANITIZED "${name}")
    string(TOUPPER "${NAME_SANITIZED}" NAME_UPPER)
    target_compile_definitions(${name} PRIVATE
                               "${NAME_UPPER}_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}"
                               "${NAME_UPPER}_VERSION_MINOR=${PROJECT_VERSION_MINOR}"
                               "${NAME_UPPER}_VERSION_PATCH=${PROJECT_VERSION_PATCH}"
                               "${NAME_UPPER}_VERSION=\"${PROJECT_VERSION}\""
    )

    # To support mixing linking in static and dynamic libraries, link each
    # library in with an extra call to target_link_libraries.
    foreach (lib ${libs})
        target_link_libraries(${name} ${lib})
    endforeach ()
endfunction()

# occ_imgui_cxx_executable(name srcs libs...)
#
# creates a named target that is built from the given source files and depends
# on the given libs.
function(occ_imgui_cxx_executable name srcs libs)
    local_occ_imgui_cxx_executable_with_flags(
        ${name} "${cxx_default}" "${libs}" "${srcs}" ${ARGN})
endfunction()

########################################################################
#
# Configure compiler warnings

# Turn on warnings on the given target
function(occ_imgui_target_enable_warnings target_name)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        list(APPEND MSVC_OPTIONS "/W4")
        list(APPEND MSVC_OPTIONS "/wd4100") # TODO: remove this once we fix all warnings
        if(MSVC_VERSION GREATER 1900) # Allow non fatal security warnings for msvc 2015
            list(APPEND MSVC_OPTIONS "/WX")
        endif()
    endif()

    target_compile_options(
        ${target_name}
        PRIVATE $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:GNU>>:
        -Wall
        -Wextra
        -Wconversion
        -pedantic
        -Werror
        -Wfatal-errors
        -Wno-error=unused-parameter  # TODO: remove this once we fix all warnings
        -Wno-error=unused-function>  # TODO: remove this once we fix all warnings
        $<$<CXX_COMPILER_ID:MSVC>:${MSVC_OPTIONS}>)
endfunction()

########################################################################
#
# Utility functions

# Install 3rd party copyright notices
function(occ_imgui_build_3rd_party_copyright)
    set(LICENSE_3RD_PARTY_FILE ${CMAKE_CURRENT_BINARY_DIR}/LICENSE-3RD-PARTY.txt)
    file(REMOVE ${LICENSE_3RD_PARTY_FILE}) # Delete the old file

    file(GLOB SEARCH_RESULT "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/share/**/copyright")
    set(COPYRIGHT_FILES ${SEARCH_RESULT} CACHE INTERNAL "copyright files")

    # Handle duplicate copyright files
    local_occ_imgui_build_3rd_party_copyright("${COPYRIGHT_FILES}" "COPYRIGHT_FILES" "Boost Software License")

    # Exclude libraries by name
    set(EXCLUDED_LIBRARIES "vcpkg-*")

    foreach (copyright_file ${COPYRIGHT_FILES})
        string(REGEX MATCH ".*/(.*)/copyright" _ ${copyright_file})
        if("${CMAKE_MATCH_1}" STREQUAL "")
            string(REGEX MATCH ".*/(.*)/LICENSE" _ ${copyright_file})
        endif()
        set(LIBRARY_NAME ${CMAKE_MATCH_1})

        # Check if the library name matches any of the excluded patterns
        set(EXCLUDE_LIBRARY FALSE)
        foreach (pattern IN ITEMS ${EXCLUDED_LIBRARIES})
            if(LIBRARY_NAME MATCHES ${pattern})
                set(EXCLUDE_LIBRARY TRUE)
                break() # Exit the inner loop
            endif()
        endforeach ()

        if(EXCLUDE_LIBRARY)
            continue() # Skip the current iteration
        endif()

        file(APPEND ${LICENSE_3RD_PARTY_FILE} "-------------------------------------------------------------------------------\n")
        file(APPEND ${LICENSE_3RD_PARTY_FILE} "${LIBRARY_NAME}\n")
        file(APPEND ${LICENSE_3RD_PARTY_FILE} "-------------------------------------------------------------------------------\n")
        file(READ ${copyright_file} COPYRIGHT_CONTENTS)
        file(APPEND ${LICENSE_3RD_PARTY_FILE} "${COPYRIGHT_CONTENTS}\n")
    endforeach ()
endfunction()

# Remove duplicate from list by content
function(local_occ_imgui_build_3rd_party_copyright list list_name filter_by)
    set(INCLUDED OFF)
    foreach (file ${list})
        file(STRINGS ${file} content)
        string(FIND "${content}" ${filter_by} FOUND)
        if(NOT ${FOUND} EQUAL -1)
            if(INCLUDED)
                list(REMOVE_ITEM list ${file})
            else()
                set(INCLUDED ON)
            endif()
        endif()
    endforeach ()
    set(${list_name} ${list} PARENT_SCOPE)
endfunction()
