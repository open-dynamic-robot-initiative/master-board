######################
# set up the project #
######################
cmake_minimum_required(VERSION 3.5.1)

# required to use std::shared_ptr in code and to link the python bindings
if(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wl,--no-as-needed")
endif()
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Wextra")

project(master_board_sdk)

# ensuring path to libraries are set during install
set(CMAKE_SKIP_BUILD_RPATH  FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
set(CMAKE_INSTALL_RPATH ${CMAKE_INSTALL_PREFIX}/lib)

##########################################
# export the package as a catkin package #
##########################################

catkin_package(
  INCLUDE_DIRS ${master_board_sdk_dir}/include
  LIBRARIES master-board-sdk
)

################################################
# Load the local pybind11 download throw treep #
################################################

set(master_board_sdk_dir "${CMAKE_SOURCE_DIR}/../sdk")
add_subdirectory(${master_board_sdk_dir} tmp_${PROJECT_NAME})

##########################
# building documentation #
##########################
build_doxygen_documentation()