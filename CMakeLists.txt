cmake_minimum_required(VERSION 3.25...3.30)

# 1. SILENCE DEPRECATION WARNINGS (Must be before project())
set(CMAKE_POLICY_DEFAULT_CMP0000 NEW)

project(loomis LANGUAGES CXX)

# 2. GLOBAL SETTINGS
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# 3. IPO/LTO SUPPORT CHECK
include(CheckIPOSupported)
check_ipo_supported(RESULT ipo_supported OUTPUT ipo_error)

# 4. DEPENDENCIES (FetchContent)
include(FetchContent)
include(Dependencies.cmake)

FetchContent_Declare(glaze   GIT_REPOSITORY ${GLAZE_REPO}   GIT_TAG ${GLAZE_VERSION})
FetchContent_Declare(spdlog  GIT_REPOSITORY ${SPDLOG_REPO}  GIT_TAG ${SPDLOG_VERSION})
FetchContent_Declare(httplib GIT_REPOSITORY ${HTTPLIB_REPO} GIT_TAG ${HTTPLIB_VERSION})
FetchContent_Declare(pugixml GIT_REPOSITORY ${PUGIXML_REPO} GIT_TAG ${PUGIXML_VERSION})
FetchContent_Declare(croncpp GIT_REPOSITORY ${CRONCPP_REPO} GIT_TAG ${CRONCPP_VERSION})

set(HTTPLIB_COMPILE OFF CACHE BOOL "" FORCE)
set(PUGIXML_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(SPDLOG_MSVC_RUNTIME_LIBRARY "${CMAKE_MSVC_RUNTIME_LIBRARY}" CACHE STRING "" FORCE)

FetchContent_MakeAvailable(glaze spdlog httplib pugixml croncpp)

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
    $<$<CXX_COMPILER_ID:MSVC>:/utf-8 /MP>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic>
)

# 8. PLATFORM SPECIFIC (UNIX/SSL)
if(UNIX)
    target_link_options(loomis PRIVATE 
        "$<$<CONFIG:Release>:-s>"
        -static-libstdc++ 
        -static-libgcc
    )
    find_package(OpenSSL REQUIRED)
    target_link_libraries(loomis PRIVATE OpenSSL::SSL OpenSSL::Crypto)
    target_compile_definitions(loomis PRIVATE CPPHTTPLIB_OPENSSL_SUPPORT)
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
    <httplib.h>
    <croncpp.h>
    <spdlog/spdlog.h>
    <glaze/glaze.hpp>
    <pugixml.hpp>
)

# 10. INCLUDES & LINKING
target_include_directories(loomis PRIVATE include src)

target_link_libraries(loomis PRIVATE
    pugixml::pugixml
    spdlog::spdlog
    glaze::glaze
    httplib::httplib
    croncpp::croncpp
)