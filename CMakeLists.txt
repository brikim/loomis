cmake_minimum_required(VERSION 3.25)
project(loomis LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# --- Global Compiler Options ---
if(MSVC)
    add_compile_options(/utf-8 /wd4129 /MP)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
else()
    # Optimization: Strip symbols in Release to counteract the static linking size increase
    add_link_options("$<$<CONFIG:Release>:-s>")
endif()

# --- External Dependencies ---
add_subdirectory(external)

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
    src       # Allows #include "api/api-base.h"
    external
    external/spdlog/include
    external/glaze/include
)

target_link_libraries(loomis PRIVATE
    pugixml-static
    spdlog
    glaze::glaze
)

# --- Portability & Static Linking ---
if(NOT MSVC)
    target_link_options(loomis PRIVATE -static-libstdc++ -static-libgcc)
endif()

# --- Extra: Compile Definitions ---
target_compile_definitions(loomis PRIVATE 
    $<$<CONFIG:Debug>:DEBUG>
    $<$<CONFIG:Release>:NDEBUG>
)