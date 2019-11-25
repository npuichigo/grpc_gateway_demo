# Minimum CMake required
cmake_minimum_required(VERSION 3.0)

# Project
project(grpc_gateway_demo C CXX)

include(CMakeDependentOption)
option(BUILD_TEST "Build C++ test binaries (need googletest and benchmark)" ON)
option(USE_SYSTEM_EIGEN_INSTALL
  "Use system Eigen instead of the one under third_party" OFF)
CMAKE_DEPENDENT_OPTION(
  grpc_gateway_demo_USE_MSVC_STATIC_RUNTIME "Using MSVC static runtime libraries" ON
  "NOT BUILD_SHARED_LIBS" OFF)

# CMake scripts + modules
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/Modules)

# CMake build directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Include utils
include(cmake/utils.cmake)

# Set default build type
if(NOT CMAKE_BUILD_TYPE)
    message(STATUS "Build type not set - defaulting to Release")
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build from: Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif()

# Dependencies
include(cmake/dependencies.cmake)

# Build flags
set(CMAKE_CXX_STANDARD 11)

# Prefix path to grpc_gateway_demo headers.
include_directories(BEFORE ${grpc_gateway_demo_SOURCE_DIR})

# Prefix path to generated grpc_gateway_demo headers.
include_directories(BEFORE ${PROJECT_BINARY_DIR})

# Main build
add_subdirectory(grpc_gateway_demo)
