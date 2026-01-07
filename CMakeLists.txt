cmake_minimum_required(VERSION 3.25...3.30)
project(loomis LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

include(FetchContent)
include(Dependencies.cmake)

FetchContent_Declare(glaze   GIT_REPOSITORY ${GLAZE_REPO}   GIT_TAG ${GLAZE_VERSION})
FetchContent_Declare(spdlog  GIT_REPOSITORY ${SPDLOG_REPO}  GIT_TAG ${SPDLOG_VERSION})
FetchContent_Declare(httplib GIT_REPOSITORY ${HTTPLIB_REPO} GIT_TAG ${HTTPLIB_VERSION})
FetchContent_Declare(pugixml GIT_REPOSITORY ${PUGIXML_REPO} GIT_TAG ${PUGIXML_VERSION})
FetchContent_Declare(croncpp GIT_REPOSITORY ${CRONCPP_REPO} GIT_TAG ${CRONCPP_VERSION})

# Configure deps before loading
set(HTTPLIB_COMPILE OFF CACHE BOOL "" FORCE)
set(PUGIXML_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(SPDLOG_MSVC_RUNTIME_LIBRARY "${CMAKE_MSVC_RUNTIME_LIBRARY}" CACHE STRING "" FORCE)

FetchContent_MakeAvailable(glaze spdlog httplib pugixml croncpp)

set_property(DIRECTORY PROPERTY CONFIGURE_DEPENDS ON)

file(GLOB_RECURSE LOOMIS_SOURCES 
    "src/*.cpp"
    "src/*.h"
)

add_executable(loomis ${LOOMIS_SOURCES})

target_compile_options(loomis PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/utf-8 /wd4129 /MP>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic>
)

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
    message(STATUS "Note: SSL support not configured for Windows build.")
endif()

target_precompile_headers(loomis PRIVATE
    <httplib.h>
    <croncpp.h>
    <spdlog/spdlog.h>
    <glaze/glaze.hpp>
    <pugixml.hpp>
)

target_include_directories(loomis PRIVATE include src)

target_link_libraries(loomis PRIVATE
    pugixml::pugixml
    spdlog::spdlog
    glaze::glaze
    httplib::httplib
    croncpp::croncpp
)