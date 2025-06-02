cmake_minimum_required(VERSION 3.22.1)

project(
  air2.0_algo_server
  VERSION 2.0.6
  LANGUAGES CXX)

configure_file(${PROJECT_SOURCE_DIR}/version.hpp.in
               ${PROJECT_SOURCE_DIR}/src/algo_server/version.hpp @ONLY)

set(CMAKE_CXX_COMPILER g++)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT SHELL_BUILD_TYPE)
  set(SHELL_BUILD_TYPE lib)
endif()

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

if(NOT CMAKE_VERBOSE_MAKEFILE)
  set(CMAKE_VERBOSE_MAKEFILE OFF)
endif()

if(NOT EXECUTABLE_OUTPUT_PATH)
  set(EXECUTABLE_OUTPUT_PATH bin)
endif()

if(NOT LIBRARY_OUTPUT_PATH)
  set(LIBRARY_OUTPUT_PATH lib)
endif()

add_subdirectory(src)
