cmake_minimum_required(VERSION 3.1)

project(lz4 C)

set(LZ4_MAJOR_VERSION 1)
set(LZ4_MINOR_VERSION 7)
set(LZ4_RELEASE_VERSION 5)
set(LZ4_VERSION "${LZ4_MAJOR_VERSION}.${LZ4_MINOR_VERSION}.${LZ4_RELEASE_VERSION}")

find_package(NerdCMakeModules REQUIRED)
include(NerdPackageHelper)
set(NerdPackageHelper_ENABLED TRUE)

add_package(Lz4 VERSION ${LZ4_VERSION})

set(LZ4_FILES
  lib/lz4.c
  lib/lz4.h
)

add_library(lz4 STATIC
  ${LZ4_FILES}
)

target_include_directories(lz4 PUBLIC
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib>"
  "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/lz4>"
)

package_library(lz4)
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lib/"
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/lz4"
  FILES_MATCHING
  PATTERN "dll" EXCLUDE
  PATTERN "lz4.h")

