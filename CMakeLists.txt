cmake_minimum_required(VERSION 3.25)

project(loomis)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_CXX_STANDARD 20)

add_subdirectory(external)

add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/wd4129>")
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/MP>")

add_compile_definitions("$<$<CONFIG:Debug>:DEBUG>")

add_executable(loomis)
target_sources(loomis PRIVATE
	src/api/api-base.cpp
	src/api/api-emby.cpp
	src/api/api-jellystat.cpp
	src/api/api-manager.cpp
	src/api/api-plex.cpp
	src/api/api-tautulli.cpp
	src/config-reader/config-reader.cpp
	src/logger/ansii-formatter.cpp
	src/logger/ansii-remove-formatter.cpp
	src/logger/log-apprise.cpp
	src/logger/logger.cpp
	src/services/service-base.cpp
	src/services/playlist-sync/playlist-sync-service.cpp
	src/base.cpp
	src/main.cpp
	src/cron-scheduler.cpp
	src/service-manager.cpp
)

target_include_directories(loomis PRIVATE include
	external
	src
)

target_link_libraries(loomis PRIVATE
	pugixml-static
)
