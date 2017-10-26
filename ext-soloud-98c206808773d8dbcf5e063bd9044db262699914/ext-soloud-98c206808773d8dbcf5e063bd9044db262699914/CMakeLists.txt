cmake_minimum_required(VERSION 2.8.11)

project(soloud CXX)

find_package(NerdCMakeModules REQUIRED)
include(NerdPackageHelper)

add_package(soloud)

set(SOLOUD_BACKEND "SDL2_STATIC" CACHE STRING "SoLoud audio backend")

string(COMPARE EQUAL "${SOLOUD_BACKEND}" "SDL2_STATIC" SOLOUD_USE_SDL2_STATIC)
string(COMPARE EQUAL "${SOLOUD_BACKEND}" "ALSA"        SOLOUD_USE_ALSA)

set(SOLOUD_SRC_FILES
	"include/soloud.h"
	"include/soloud_audiosource.h"
	"include/soloud_bus.h"
	"include/soloud_error.h"
	"include/soloud_fader.h"
	"include/soloud_fft.h"
	"include/soloud_file.h"
	"include/soloud_file_hack_off.h"
	"include/soloud_file_hack_on.h"
	"include/soloud_filter.h"
	"include/soloud_internal.h"
	"include/soloud_thread.h"
	"include/soloud_wav.h"
	"include/soloud_wavstream.h"
	"src/core/soloud.cpp"
	"src/core/soloud_audiosource.cpp"
	"src/core/soloud_bus.cpp"
	"src/core/soloud_core_3d.cpp"
	"src/core/soloud_core_basicops.cpp"
	"src/core/soloud_core_faderops.cpp"
	"src/core/soloud_core_filterops.cpp"
	"src/core/soloud_core_getters.cpp"
	"src/core/soloud_core_setters.cpp"
	"src/core/soloud_core_voicegroup.cpp"
	"src/core/soloud_core_voiceops.cpp"
	"src/core/soloud_fader.cpp"
	"src/core/soloud_fft.cpp"
	"src/core/soloud_fft_lut.cpp"
	"src/core/soloud_file.cpp"
	"src/core/soloud_filter.cpp"
	"src/core/soloud_thread.cpp"
	"src/audiosource/wav/soloud_wav.cpp"
	"src/audiosource/wav/soloud_wavstream.cpp"
	"src/audiosource/wav/stb_vorbis.c"
	"src/audiosource/wav/stb_vorbis.h"
)

if (SOLOUD_USE_SDL2_STATIC)
	list(APPEND SOLOUD_SRC_FILES
		"src/backend/sdl2_static/soloud_sdl2_static.cpp"
	)
elseif (SOLOUD_USE_ALSA)
	list(APPEND SOLOUD_SRC_FILES
		"src/backend/alsa/soloud_alsa.cpp"
	)
endif()

source_group("src" FILES ${SOLOUD_SRC_FILES})

add_library(soloud ${SOLOUD_SRC_FILES})

if (NERD_COMPILER_IS_MSVC)
	target_compile_options(soloud PRIVATE /WX-) # Don't treat warnings as errors
endif()

target_include_directories(soloud PUBLIC
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>")

if (SOLOUD_USE_SDL2_STATIC)
	find_package(SDL2 REQUIRED NO_MODULE)
	target_link_libraries(soloud PUBLIC SDL2::SDL2)
	target_compile_definitions(soloud PUBLIC -DWITH_SDL2_STATIC)
elseif (SOLOUD_USE_ALSA)
	find_package(ALSA REQUIRED)
	target_link_libraries(soloud PUBLIC ${ALSA_LIBRARY})
	target_include_directories(soloud PUBLIC ${ALSA_INCLUDE_DIR})
	target_compile_definitions(soloud PUBLIC -DWITH_ALSA)
else()
	message(FATAL_ERROR "Invalid SoLoud backend: ${SOLOUD_BACKEND}")
endif()

message("SoLoud backend is ${SOLOUD_BACKEND}")


package_library(soloud)
install(DIRECTORY include/
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  FILES_MATCHING PATTERN "*.h")
