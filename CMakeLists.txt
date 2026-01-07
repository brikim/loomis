cmake_minimum_required(VERSION 3.25)
project(loomis LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" CACHE STRING "" FORCE)

include(FetchContent)

FetchContent_Declare(
  glaze
  GIT_REPOSITORY https://github.com/stephenberry/glaze.git
  GIT_TAG        v6.5.1
)

# --- Dependencies: spdlog ---
FetchContent_Declare(
  spdlog
  GIT_REPOSITORY https://github.com/gabime/spdlog.git
  GIT_TAG        v1.17.0
)
set(SPDLOG_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" CACHE STRING "" FORCE)

FetchContent_Declare(
  httplib
  GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
  GIT_TAG        v0.30.0
)
set(HTTPLIB_COMPILE OFF CACHE BOOL "" FORCE)

set(PUGIXML_BUILD_SHARED OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
  pugixml
  GIT_REPOSITORY https://github.com/zeux/pugixml.git
  GIT_TAG        v1.15
)

FetchContent_Declare(
  croncpp
  GIT_REPOSITORY https://github.com/mariusbancila/croncpp.git
  GIT_TAG        v2023.03.30
)

FetchContent_MakeAvailable(glaze spdlog httplib pugixml croncpp)

# --- Global Compiler Options ---
if(MSVC)
    add_compile_options(/utf-8 /wd4129 /MP)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
else()
    # Optimization: Strip symbols in Release to counteract the static linking size increase
    add_link_options("$<$<CONFIG:Release>:-s>")
endif()

# --- Source Discovery ---
# This automatically finds all .cpp files so you don't have to list them manually
file(GLOB_RECURSE LOOMIS_SOURCES 
    "src/*.cpp"
    "src/*.h"
)

# --- Main Target ---
add_executable(loomis ${LOOMIS_SOURCES})

target_include_directories(loomis PRIVATE 
    include
    src
)

target_link_libraries(loomis PRIVATE
    pugixml::pugixml
    spdlog::spdlog
    glaze::glaze
    httplib::httplib
    croncpp::croncpp
)

# --- Portability & Static Linking ---
if(UNIX)
    target_link_options(loomis PRIVATE -static-libstdc++ -static-libgcc)

    find_package(OpenSSL REQUIRED)
    add_definitions(-DCPPHTTPLIB_OPENSSL_SUPPORT)
    target_link_libraries(loomis PRIVATE OpenSSL::SSL OpenSSL::Crypto)
else()
    # On Windows, you can skip SSL or use a different method 
    # if you aren't testing HTTPS locally.
    message(STATUS "Skipping SSL for non-Linux build")
endif()

# --- Extra: Compile Definitions ---
target_compile_definitions(loomis PRIVATE 
    $<$<CONFIG:Debug>:DEBUG>
    $<$<CONFIG:Release>:NDEBUG>
)