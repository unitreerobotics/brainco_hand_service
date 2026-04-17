cmake_minimum_required(VERSION 3.10)
project(brainco_hand)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

message(STATUS "Current system architecture: ${CMAKE_SYSTEM_PROCESSOR}")

if (NOT CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64" AND NOT CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
  message(FATAL_ERROR "Unsupported architecture: ${CMAKE_SYSTEM_PROCESSOR}. Supported architectures are aarch64 and x86_64.")
endif()


if (CMAKE_BUILD_TYPE MATCHES "Debug")
  message("-- BUILD_TYPE: Debug")
  add_compile_options(-g -O0 -fPIC)
else()
  message("-- BUILD_TYPE: Release")
  add_compile_options(-O3 -DNDEBUG -fPIC)
endif()

if(POLICY CMP0167)
  cmake_policy(SET CMP0167 NEW)
endif()

find_package(Boost REQUIRED COMPONENTS program_options)
find_package(yaml-cpp REQUIRED)


include_directories(
  /usr/local/include/ddscxx
  ${CMAKE_SOURCE_DIR}/include
)

link_directories(
  ${CMAKE_SOURCE_DIR}/lib/${CMAKE_SYSTEM_PROCESSOR}
)

link_libraries(
  unitree_sdk2 ddsc ddscxx rt pthread
  libboost_program_options.a yaml-cpp fmt
  bc_stark_sdk
)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
add_executable(brainco_hand_server main.cpp)
add_executable(test_brainco_hand_server test/test_brainco_hand_server.cpp)