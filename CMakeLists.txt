cmake_minimum_required(VERSION 3.5)
project(fsfreader)

find_package(PkgConfig)

pkg_check_modules(ZLIB REQUIRED zlib)
include_directories(${ZLIB_INCLUDE_DIRS})
link_directories(${ZLIB_LIBRARY_DIRS})
set(LIBS ${ZLIB_LIBRARIES})

add_subdirectory(src)
