cmake_minimum_required(VERSION 3.1)

if (NOT WIN32)
	message(FATAL_ERROR "This is for Windows only")
endif()

if (NOT DEFINED ENV{PLUGIN_SDK_DIR})
	message(FATAL_ERROR "Environment variable PLUGIN_SDK_DIR is not defined.\nPlease use the plugin-sdk 'installer-launcher' program to set the location of plugin-sdk\n")
endif()

project(sm64-san-andreas)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "-m32 -fpermissive -fPIC")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O2")

set(SDL2 ${PROJECT_SOURCE_DIR}/SDL2)
set(SDL2_LIBRARY ${SDL2}/lib)
set(SDL2_INCLUDE_DIR ${SDL2}/include/SDL)

file(GLOB SM64_GTASA_SRC
	*.cpp
)

set(BUILD_GTAIII OFF CACHE BOOL "GTA III plugin")
set(BUILD_GTAVC OFF CACHE BOOL "GTA VC plugin")
add_subdirectory($ENV{PLUGIN_SDK_DIR} plugin-sdk)
add_subdirectory(libsm64)

add_library(sm64-san-andreas SHARED ${SM64_GTASA_SRC})

set_target_properties(sm64-san-andreas PROPERTIES PREFIX "")
set_target_properties(sm64-san-andreas PROPERTIES OUTPUT_NAME "sm64-san-andreas")
set_target_properties(sm64-san-andreas PROPERTIES SUFFIX ".asi")

target_include_directories(sm64-san-andreas PRIVATE
	$ENV{PLUGIN_SDK_DIR}/plugin_sa
	$ENV{PLUGIN_SDK_DIR}/plugin_sa/game_sa
	$ENV{PLUGIN_SDK_DIR}/shared
	$ENV{PLUGIN_SDK_DIR}/shared/game
	${SDL2_INCLUDE_DIR}
	libsm64/src
)
target_link_directories(sm64-san-andreas PRIVATE ${SDL2_LIBRARY})
target_link_libraries(sm64-san-andreas PRIVATE plugin_sa SDL2 sm64)
target_link_options(sm64-san-andreas PRIVATE -static-libgcc -static-libstdc++ -s)
target_compile_options(sm64-san-andreas PRIVATE -static-libgcc -static-libstdc++)
target_compile_definitions(sm64-san-andreas PRIVATE
	GTASA
	PLUGIN_SGV_10US
	GTAGAME_NAME="\"San Andreas"\"
	GTAGAME_ABBR="\"SA"\"
	GTAGAME_ABBRLOW="\"sa"\"
	GTAGAME_PROTAGONISTNAME="\"CJ"\"
	GTAGAME_CITYNAME="\"San Andreas"\"
)

get_filename_component(COMPILER_DIR ${CMAKE_CXX_COMPILER} DIRECTORY)
add_custom_command(TARGET sm64-san-andreas POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/scripts)
add_custom_command(TARGET sm64-san-andreas POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy
                   ${COMPILER_DIR}/libgcc_s_dw2-1.dll
                   ${COMPILER_DIR}/libwinpthread-1.dll
                   ${SDL2}/bin/SDL2.dll
                   ${CMAKE_CURRENT_BINARY_DIR}/libsm64/libsm64.dll
                   ${CMAKE_CURRENT_BINARY_DIR}/sm64-san-andreas.asi  ${CMAKE_CURRENT_BINARY_DIR}/scripts/)
