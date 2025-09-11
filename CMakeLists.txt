cmake_minimum_required(VERSION 3.10)
project(brainco_hand)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

if (CMAKE_BUILD_TYPE MATCHES "Debug")
  message("Debug mode")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -g -O0 -fPIC")
else()
  message("Release mode")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -O3 -DNDEBUG -fPIC")
endif()

find_package(Boost REQUIRED COMPONENTS program_options)
find_package(yaml-cpp REQUIRED)


include_directories(
  /usr/local/include/ddscxx
  /usr/local/include/iceoryx/v2.0.2
  include
  thirdparty/stark-serialport-example/dist/include
)

link_directories(thirdparty/stark-serialport-example/dist/shared/linux)
link_libraries(
  unitree_sdk2 ddsc ddscxx rt pthread fmt
  libboost_program_options.a yaml-cpp
  bc_stark_sdk
)

add_executable(brainco_hand brainco_service.cpp)
# add_executable(example_brainco_hand example/example_brainco_hand.cpp)
# add_executable(test_hand_comm example/test_hand_comm.cpp)
