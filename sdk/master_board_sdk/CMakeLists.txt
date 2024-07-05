#
# Copyright (c) 2019 CNRS Copyright (c) 2024 CNRS INRIA
#

cmake_minimum_required(VERSION 3.14)

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

option(BUILD_PYTHON_INTERFACE "Build the python binding" ON)
option(PYTHON_STANDARD_LAYOUT "Enable standard Python package layout" ON)
option(PYTHON_DEB_LAYOUT "Enable Debian-style Python package layout" OFF)

# Check if the submodule cmake have been initialized
set(JRL_CMAKE_MODULES "${CMAKE_CURRENT_LIST_DIR}/cmake")
if(EXISTS "${JRL_CMAKE_MODULES}/base.cmake")
  message(STATUS "JRL cmakemodules found in 'cmake/' git submodule")
else()
  find_package(jrl-cmakemodules QUIET CONFIG)
  if(jrl-cmakemodules_FOUND)
    get_property(
      JRL_CMAKE_MODULES
      TARGET jrl-cmakemodules::jrl-cmakemodules
      PROPERTY INTERFACE_INCLUDE_DIRECTORIES)
    message(STATUS "JRL cmakemodules found on system at ${JRL_CMAKE_MODULES}")
  elseif(${CMAKE_VERSION} VERSION_LESS "3.14.0")
    message(
      FATAL_ERROR
        "\nCan't find jrl-cmakemodules. Please either:\n"
        "  - use git submodule: 'git submodule update --init'\n"
        "  - or install https://github.com/jrl-umi3218/jrl-cmakemodules\n"
        "  - or upgrade your CMake version to >= 3.14 to allow automatic fetching\n"
    )
  else()
    message(STATUS "JRL cmakemodules not found. Let's fetch it.")
    include(FetchContent)
    FetchContent_Declare(
      "jrl-cmakemodules"
      GIT_REPOSITORY "https://github.com/jrl-umi3218/jrl-cmakemodules.git")
    FetchContent_MakeAvailable("jrl-cmakemodules")
    FetchContent_GetProperties("jrl-cmakemodules" SOURCE_DIR JRL_CMAKE_MODULES)
  endif()
endif()

include("${JRL_CMAKE_MODULES}/base.cmake")
include("${JRL_CMAKE_MODULES}/boost.cmake")
include("${JRL_CMAKE_MODULES}/python.cmake")
include("${JRL_CMAKE_MODULES}/ide.cmake")
include("${JRL_CMAKE_MODULES}/apple.cmake")

compute_project_args(PROJECT_ARGS LANGUAGES CXX)
project(${PROJECT_NAME} ${PROJECT_ARGS})
apply_default_apple_configuration()
check_minimal_cxx_standard(11 ENFORCE)

# ----------------------------------------------------
# --- DEPENDENCIES -----------------------------------
# ----------------------------------------------------

# Set component to fetch from boost Get the python interface for the bindings
if(BUILD_PYTHON_INTERFACE)
  set(PYTHON_COMPONENTS Interpreter Development.Module)
  findpython(REQUIRED)
  set_boost_default_options()
  search_for_boost_python(REQUIRED)
endif(BUILD_PYTHON_INTERFACE)

# ----------------------------------------------------
# --- INCLUDE ----------------------------------------
# ----------------------------------------------------

# --- MAIN LIBRARY -------------------------------------------------------------
set(${PROJECT_NAME}_SOURCES
    src/ETHERNET_types.cpp src/Link_manager.cpp src/master_board_interface.cpp
    src/motor.cpp src/motor_driver.cpp)
if(NOT APPLE)
  list(APPEND ${PROJECT_NAME}_SOURCES src/ESPNOW_manager.cpp
       src/ESPNOW_types.cpp)
endif(NOT APPLE)
set(${PROJECT_NAME}_HEADERS
    "include/${PROJECT_NAME}/defines.h"
    "include/${PROJECT_NAME}/ESPNOW_manager.h"
    "include/${PROJECT_NAME}/ESPNOW_types.h"
    "include/${PROJECT_NAME}/ETHERNET_manager.h"
    "include/${PROJECT_NAME}/ETHERNET_types.h"
    "include/${PROJECT_NAME}/Link_manager.h"
    "include/${PROJECT_NAME}/Link_types.h"
    "include/${PROJECT_NAME}/master_board_interface.h"
    "include/${PROJECT_NAME}/motor.h"
    "include/${PROJECT_NAME}/motor_driver.h"
    "include/${PROJECT_NAME}/protocol.h")

add_library(${PROJECT_NAME} SHARED ${${PROJECT_NAME}_SOURCES}
                                   ${${PROJECT_NAME}_HEADERS})
target_link_libraries(${PROJECT_NAME} PUBLIC pthread)
target_include_directories(
  ${PROJECT_NAME} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
                         $<INSTALL_INTERFACE:include>)
install(
  TARGETS ${PROJECT_NAME}
  EXPORT ${TARGETS_EXPORT_NAME}
  DESTINATION lib)

install(FILES package.xml DESTINATION share/${PROJECT_NAME})
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
  set_target_properties(${PYWRAP} PROPERTIES SUFFIX ${PYTHON_EXT_SUFFIX})
  target_link_libraries(${PYWRAP} PUBLIC ${PROJECT_NAME})
  target_link_boost_python(${PYWRAP} PUBLIC)
  if(UNIX)
    get_relative_rpath(${${PYWRAP}_INSTALL_DIR} ${PYWRAP}_INSTALL_RPATH)
    set_target_properties(${PYWRAP} PROPERTIES INSTALL_RPATH
                                               "${${PYWRAP}_INSTALL_RPATH}")
  endif()

  install(TARGETS ${PYWRAP} DESTINATION ${${PYWRAP}_INSTALL_DIR})

  # --- Allow to do: make python --- #
  add_custom_target(python)
  set_target_properties(python PROPERTIES EXCLUDE_FROM_DEFAULT_BUILD True)
  add_dependencies(python ${PYWRAP})

  # --- build the python files to check them --- #
  python_build(example example.py)
  python_build(example listener.py)
  python_build(example com_analyser.py)
endif(BUILD_PYTHON_INTERFACE)

# --- CHECK EXAMPLES -----------------------------------------------------------
add_executable(master_board_example example/example.cpp)
target_link_libraries(master_board_example PRIVATE ${PROJECT_NAME})
set_target_properties(master_board_example
                      PROPERTIES INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
install(TARGETS master_board_example DESTINATION bin)

add_executable(master_board_example_pd example/example_pd.cpp)
target_link_libraries(master_board_example_pd PRIVATE ${PROJECT_NAME})
set_target_properties(master_board_example_pd
                      PROPERTIES INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
install(TARGETS master_board_example_pd DESTINATION bin)

add_executable(master_board_example_imu_data_collection
               example/example_imu_data_collection.cpp)
target_link_libraries(master_board_example_imu_data_collection
                      PRIVATE ${PROJECT_NAME})
install(TARGETS master_board_example_imu_data_collection DESTINATION bin)

# --- UNIT TESTS -----------------------------------------------------------
if(BUILD_TESTING)
  find_package(Catch2)
  add_definitions(-DFOUND_CATCH2_MAJOR_VERSION=${Catch2_VERSION_MAJOR})
  add_unit_test(test_protocol tests/test_protocol.cpp)
  target_link_libraries(test_protocol PRIVATE Catch2::Catch2WithMain)
endif()
# Allows Colcon to find non-Ament packages when using workspace underlays
file(
  WRITE
  ${CMAKE_CURRENT_BINARY_DIR}/share/ament_index/resource_index/packages/${PROJECT_NAME}
  "")
install(
  FILES
    ${CMAKE_CURRENT_BINARY_DIR}/share/ament_index/resource_index/packages/${PROJECT_NAME}
  DESTINATION share/ament_index/resource_index/packages)
file(
  WRITE
  ${CMAKE_CURRENT_BINARY_DIR}/share/${PROJECT_NAME}/hook/ament_prefix_path.dsv
  "prepend-non-duplicate;AMENT_PREFIX_PATH;")
install(
  FILES
    ${CMAKE_CURRENT_BINARY_DIR}/share/${PROJECT_NAME}/hook/ament_prefix_path.dsv
  DESTINATION share/${PROJECT_NAME}/hook)
file(WRITE
     ${CMAKE_CURRENT_BINARY_DIR}/share/${PROJECT_NAME}/hook/python_path.dsv
     "prepend-non-duplicate;PYTHONPATH;${PYTHON_SITELIB}")
install(
  FILES ${CMAKE_CURRENT_BINARY_DIR}/share/${PROJECT_NAME}/hook/python_path.dsv
  DESTINATION share/${PROJECT_NAME}/hook)
