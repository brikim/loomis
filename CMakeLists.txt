cmake_minimum_required(VERSION 3.25...3.30)

# 1. SILENCE DEPRECATION WARNINGS (Must be before project())
set(CMAKE_POLICY_DEFAULT_CMP0000 NEW)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(loomis LANGUAGES CXX)

# 2. GLOBAL SETTINGS
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

# 3. IPO/LTO SUPPORT CHECK
include(CheckIPOSupported)
check_ipo_supported(RESULT ipo_supported OUTPUT ipo_error)

if(WIN32)
    # 1. Point CMake to the OpenSSL installation directory
    if(NOT OPENSSL_ROOT_DIR)
        set(OPENSSL_ROOT_DIR "C:/Program Files/OpenSSL-Win64" CACHE PATH "Directory containing OpenSSL")
    endif()

    # 2. Force CMake to look for static libraries if warp links statically
    set(OPENSSL_USE_STATIC_LIBS ON CACHE BOOL "Use static OpenSSL libraries")
endif()

# Disable test suites and targets for sub-dependencies
set(LIBCRON_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)

# 4. DEPENDENCIES (FetchContent)
include(FetchContent)
include(Dependencies.cmake)

FetchContent_Declare(warp     GIT_REPOSITORY ${WARP_REPO}    GIT_TAG ${WARP_VERSION})
FetchContent_Declare(glaze    GIT_REPOSITORY ${GLAZE_REPO}   GIT_TAG ${GLAZE_VERSION})

FetchContent_MakeAvailable(warp glaze)

# 5. SOURCE DISCOVERY
file(GLOB_RECURSE LOOMIS_SOURCES 
    "src/*.cpp"
    "src/*.h"
)

# 6. CREATE THE TARGET (Only call this ONCE)
add_executable(loomis ${LOOMIS_SOURCES})

# 7. APPLY TARGET PROPERTIES (Must be AFTER add_executable)
if(ipo_supported)
    set_property(TARGET loomis PROPERTY
        INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE
        INTERPROCEDURAL_OPTIMIZATION_MINSIZEREL TRUE
    )
endif()

target_compile_options(loomis PRIVATE
    # MSVC / clang-cl flags
    $<$<CXX_COMPILER_ID:MSVC>:/utf-8 /MP>
    
    # Standard Clang/GCC flags (C++ only, skips OpenSSL's applink.c)
    $<$<AND:$<COMPILE_LANGUAGE:CXX>,$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:ClangCl>>>:
        -Wall 
        -Wextra 
        -Wpedantic 
        -Wshadow 
        -Wconversion 
        -Wno-c++98-compat 
        -Wno-c++98-compat-pedantic 
        -Wno-exit-time-destructors 
        -Wno-global-constructors 
        -Wno-padded
        -Wno-covered-switch-default
        -Wno-ctad-maybe-unsupported
        -Wno-switch-enum
    >
)

# 8. PLATFORM SPECIFIC (UNIX/SSL)
if(UNIX)
    target_link_options(loomis PRIVATE 
        "$<$<CONFIG:Release>:-s>"
        -static-libstdc++
        -static-libgcc
    )
else()
    # Apply MSVC Link-Time Code Generation (The Windows version of LTO)
    target_link_options(loomis PRIVATE $<$<CONFIG:Release>:/LTCG>)
endif()

# 9. PRECOMPILED HEADERS
target_precompile_headers(loomis PRIVATE
    <format>
    <string>
    <vector>
    <memory>
    <regex>
    <chrono>
    <iostream>
    <string_view>

    # Third-Party
    <glaze/glaze.hpp>
)

# 10. INCLUDES & LINKING
target_include_directories(loomis PRIVATE 
    "${PROJECT_SOURCE_DIR}/include"
    "${PROJECT_SOURCE_DIR}/src"
    "${PROJECT_SOURCE_DIR}/src/services"
)

target_link_libraries(loomis PRIVATE
    warp::warp
    glaze::glaze
)