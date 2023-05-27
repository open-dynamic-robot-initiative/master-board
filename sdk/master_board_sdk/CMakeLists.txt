#
# Copyright (c) 2019 CNRS
#

cmake_minimum_required(VERSION 3.1)

# ----------------------------------------------------
# --- CXX FLAGS --------------------------------------
# ----------------------------------------------------

set(CXX_DISABLE_WERROR True)
set(CMAKE_VERBOSE_MAKEFILE True)

# These variables have to be defined before running SETUP_PROJECT
set(PROJECT_NAME master_board_sdk)
set(PROJECT_DESCRIPTION
    "This project contains the sdk for the communication between a computer and the master-board"
)
set(PROJECT_URL https://github.com/open-dynamic-robot-initiative/master-board)
set(PROJECT_USE_CMAKE_EXPORT TRUE)
set(PROJECT_VERSION_COMPUTATION_METHODS ROS_PACKAGE_XML_FILE)

# --- OPTIONS ----------------------------------------
if(APPLE)
  set(BUILD_PYTHON_INTERFACE
      OFF
      CACHE BOOL "Build the python binding")
endif()

option(BUILD_PYTHON_INTERFACE "Build the python binding" ON)
option(PYTHON_STANDARD_LAYOUT "Enable standard Python package layout" ON)
option(PYTHON_DEB_LAYOUT "Enable Debian-style Python package layout" OFF)

include(cmake/base.cmake)

compute_project_args(PROJECT_ARGS LANGUAGES CXX)
project(${PROJECT_NAME} ${PROJECT_ARGS})

include(cmake/boost.cmake)
include(cmake/python.cmake)
include(cmake/ide.cmake)
include(cmake/apple.cmake)

check_minimal_cxx_standard(11 ENFORCE)

# ----------------------------------------------------
# --- DEPENDENCIES -----------------------------------
# ----------------------------------------------------

# Set component to fetch from boost Get the python interface for the bindings
if(BUILD_PYTHON_INTERFACE)
  set(PYTHON_EXPORT_DEPENDENCY ON)

  findpython(REQUIRED)
  search_for_boost_python(REQUIRED)
   
endif(BUILD_PYTHON_INTERFACE)

# ----------------------------------------------------
# --- INCLUDE ----------------------------------------
# ----------------------------------------------------

# --- MAIN LIBRARY -------------------------------------------------------------
set(MASTER_BOARD_SRC
    src/ETHERNET_types.cpp src/Link_manager.cpp src/master_board_interface.cpp
    src/motor.cpp src/motor_driver.cpp)
if(NOT APPLE)
  list(APPEND MASTER_BOARD_SRC src/ESPNOW_manager.cpp src/ESPNOW_types.cpp)
endif(NOT APPLE)

add_library(${PROJECT_NAME} SHARED ${MASTER_BOARD_SRC})
target_link_libraries(${PROJECT_NAME} pthread)
target_include_directories(
  ${PROJECT_NAME} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
                         $<INSTALL_INTERFACE:include>)
install(
  TARGETS ${PROJECT_NAME}
  EXPORT ${TARGETS_EXPORT_NAME}
  DESTINATION lib)
install(DIRECTORY include/ DESTINATION include)

# --- BINDINGS -----------------------------------------------------------------

if(BUILD_PYTHON_INTERFACE)
  # --- Setup the wrapper name and source files --- #
  set(PYWRAP ${PROJECT_NAME}_pywrap)
  set(${PYWRAP}_HEADERS srcpy/my_bindings_headr.h)
  set(${PYWRAP}_SOURCES srcpy/my_bindings_headr.cpp)

  # --- Build the wrapper library --- #
  set(${PYWRAP}_INSTALL_DIR ${PYTHON_SITELIB})
  add_library(${PYWRAP} SHARED ${${PYWRAP}_SOURCES} ${${PYWRAP}_HEADERS})
  target_include_directories(${PYWRAP} SYSTEM PRIVATE ${PYTHON_INCLUDE_DIR})
  target_link_libraries(${PYWRAP} ${PROJECT_NAME})
  target_link_boost_python(${PYWRAP})
  set_target_properties(${PYWRAP} PROPERTIES INSTALL_RPATH
                                             "${CMAKE_INSTALL_PREFIX}/lib")
  install(TARGETS ${PYWRAP} DESTINATION ${${PYWRAP}_INSTALL_DIR})

  # --- Allow to do: make python --- #
  add_custom_target(python)
  set_target_properties(python PROPERTIES EXCLUDE_FROM_DEFAULT_BUILD True)
  add_dependencies(python ${PYWRAP})

  # --- Pre-build the python files for efficiency --- #
  python_build(example example.py)
  python_build(example listener.py)
  python_build(example com_analyser.py)
endif(BUILD_PYTHON_INTERFACE)

# --- CHECK EXAMPLES -----------------------------------------------------------
add_executable(master_board_example example/example.cpp)
target_link_libraries(master_board_example ${PROJECT_NAME})
install(TARGETS master_board_example DESTINATION bin)

add_executable(master_board_example_pd example/example_pd.cpp)
target_link_libraries(master_board_example_pd ${PROJECT_NAME})
install(TARGETS master_board_example_pd DESTINATION bin)

add_executable(master_board_example_imu_data_collection
               example/example_imu_data_collection.cpp)
target_link_libraries(master_board_example_imu_data_collection ${PROJECT_NAME})
install(TARGETS master_board_example_imu_data_collection DESTINATION bin)

# --- UNIT TESTS -----------------------------------------------------------
if(BUILD_TESTING)
  find_package(Catch2 3.1 QUIET)
  if(NOT Catch2_FOUND)
    include(FetchContent)
    FetchContent_Declare(
      Catch2
      GIT_REPOSITORY https://github.com/catchorg/Catch2.git
      GIT_TAG v3.1.0)
    FetchContent_MakeAvailable(Catch2)
  endif()
  add_unit_test(test_protocol tests/test_protocol.cpp)
  target_link_libraries(test_protocol PRIVATE Catch2::Catch2WithMain)
endif()
