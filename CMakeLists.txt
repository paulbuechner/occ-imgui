cmake_minimum_required(VERSION 3.24 FATAL_ERROR)

set(VCPKG_OVERLAY_TRIPLETS "${CMAKE_CURRENT_LIST_DIR}/vcpkg/triplets" CACHE STRING "")

set(OCC_IMGUI_VERSION 0.1.0)

project(occ-imgui VERSION ${OCC_IMGUI_VERSION} LANGUAGES CXX)

option(OCC_IMGUI_BUILD_TESTS "Build ${PROJECT_NAME} tests" OFF)

# Set default build to release
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose Release or Debug" FORCE)
endif()

# ---------------------------------------------------------------------------------------
# Compiler config
# ---------------------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    # make sure __cplusplus is defined when using msvc and enable parallel build
    add_compile_options(/Zc:__cplusplus /MP)

    # enable macro expansion
    add_compile_options(/Zc:preprocessor)
endif()

if(NOT CYGWIN AND NOT MSYS AND NOT MINGW AND NOT ${CMAKE_SYSTEM_NAME} STREQUAL QNX)
    set(CMAKE_CXX_EXTENSIONS OFF)
endif()

# warning options
option(OCC_IMGUI_BUILD_WARNINGS "Enable compiler warnings for occ-imgui application." OFF)

include(GNUInstallDirs)

# Define helper functions and macros used by occ-imgui
include(cmake/occ_imgui_internal_utils.cmake)

# Licensing
occ_imgui_build_3rd_party_copyright()

# Dependencies
find_package(glfw3 CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(OpenCASCADE CONFIG REQUIRED)

# Set compile definitions
add_compile_definitions(
    $<$<CONFIG:DEBUG>:OCC_IMGUI_CONFIG_DEBUG>
    $<$<CONFIG:RELEASE>:OCC_IMGUI_CONFIG_RELEASE>
)

# Set libs to link against
list(APPEND occ_imgui_libs
     glfw
     imgui::imgui
     ${OpenCASCADE_LIBRARIES}
)

# Add executable
occ_imgui_cxx_executable(${PROJECT_NAME} src/occ-imgui.cc "${occ_imgui_libs}")

# Add target_include_directories
target_include_directories(${PROJECT_NAME} PRIVATE
                           "${CMAKE_CURRENT_LIST_DIR}/include"
)

target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE
                           ${OpenCASCADE_INCLUDE_DIR}
)

if(OCC_IMGUI_BUILD_WARNINGS)
    occ_imgui_target_enable_warnings(${PROJECT_NAME})
endif()

# ---------------------------------------------------------------------------------------
# Install
# ---------------------------------------------------------------------------------------
include(InstallRequiredSystemLibraries)
install(TARGETS ${PROJECT_NAME}
        DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES ${CMAKE_SOURCE_DIR}/README.md ${CMAKE_SOURCE_DIR}/LICENSE
        DESTINATION ${CMAKE_INSTALL_DOCDIR})
install(FILES ${CMAKE_BINARY_DIR}/LICENSE-3RD-PARTY.txt
        DESTINATION ${CMAKE_INSTALL_DOCDIR})
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/test/config
        DESTINATION ${CMAKE_INSTALL_DOCDIR})

# Install the runtime dependencies (DLLs, etc.)
if(MSVC)
    install(FILES $<TARGET_RUNTIME_DLLS:${PROJECT_NAME}>
            DESTINATION ${CMAKE_INSTALL_BINDIR})
elseif(MINGW)
    occ_imgui_install_target_deps_mingw()
else()
    occ_imgui_install_target_deps_linux()
endif()

# Support creation of installable packages
include(cmake/occ-imgui-cpack.cmake)
