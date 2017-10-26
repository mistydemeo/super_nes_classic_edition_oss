if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
  message(FATAL_ERROR "Prevented in-tree built. Please create a build directory outside of the SDL source code and call cmake from there")
endif()

cmake_minimum_required(VERSION 2.8)
project(SDL2_mixer C)

set(SDL2_mixer_MAJOR_VERSION 2)
set(SDL2_mixer_MINOR_VERSION 0)
set(SDL2_mixer_MICRO_VERSION 0)
# set(SDL2_ttf_INTERFACE_AGE 2)
# set(SDL2_ttf_BINARY_AGE 12)
set(SDL2_mixer_VERSION "${SDL2_mixer_MAJOR_VERSION}.${SDL2_mixer_MINOR_VERSION}.${SDL2_mixer_MICRO_VERSION}")

find_package(NerdCMakeModules REQUIRED)
include(NerdPackageHelper)
find_package(SDL2 REQUIRED NO_MODULE)
add_package(SDL2_mixer VERSION ${SDL2_mixer_VERSION})

add_definitions(-DWAV_MUSIC)

add_library(SDL2_mixer
	dynamic_flac.c
	dynamic_flac.h
	dynamic_fluidsynth.c
	dynamic_fluidsynth.h
	dynamic_mod.c
	dynamic_mod.h
	dynamic_modplug.c
	dynamic_modplug.h
	dynamic_mp3.c
	dynamic_mp3.h
	dynamic_ogg.c
	dynamic_ogg.h
	effect_position.c
	effect_stereoreverse.c
	effects_internal.c
	effects_internal.h
	fluidsynth.c
	fluidsynth.h
	load_aiff.c
	load_aiff.h
	load_flac.c
	load_flac.h
	load_mp3.c
	load_mp3.h
	load_ogg.c
	load_ogg.h
	load_voc.c
	load_voc.h
	mixer.c
	music.c
	music_cmd.c
	music_cmd.h
	music_flac.c
	music_flac.h
	music_mad.c
	music_mad.h
	music_mod.c
	music_mod.h
	music_modplug.c
	music_modplug.h
	music_ogg.c
	music_ogg.h
	SDL_mixer.h
	wavestream.c
	wavestream.h
)
package_library(SDL2_mixer)

target_include_directories(SDL2_mixer PUBLIC
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
  "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/SDL2>"
)
target_link_libraries(SDL2_mixer PUBLIC
  SDL2::SDL2)

install(FILES SDL_mixer.h
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/SDL2")
