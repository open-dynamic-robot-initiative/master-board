#
# Copyright (c) 2019 CNRS
#

CMAKE_MINIMUM_REQUIRED(VERSION 3.1)

# ----------------------------------------------------
# --- CXX FLAGS --------------------------------------
# ----------------------------------------------------

SET(CXX_DISABLE_WERROR True)
SET(CMAKE_VERBOSE_MAKEFILE True)

# These variables have to be defined before running SETUP_PROJECT
SET(PROJECT_NAME master_board_sdk)
SET(PROJECT_DESCRIPTION "This project contains the sdk for the communication between a computer and the master-board")
SET(PROJECT_URL https://github.com/open-dynamic-robot-initiative/master-board)
set(PROJECT_USE_CMAKE_EXPORT TRUE)
SET(PROJECT_VERSION_COMPUTATION_METHODS ROS_PACKAGE_XML_FILE)

# --- OPTIONS ----------------------------------------
OPTION(BUILD_PYTHON_INTERFACE "Build the python binding" ON)
OPTION(PYTHON_STANDARD_LAYOUT "Enable standard Python package layout" ON)
OPTION(PYTHON_DEB_LAYOUT "Enable Debian-style Python package layout" OFF)

INCLUDE(cmake/base.cmake)
INCLUDE(cmake/boost.cmake)
INCLUDE(cmake/python.cmake)
INCLUDE(cmake/ide.cmake)

COMPUTE_PROJECT_ARGS(PROJECT_ARGS LANGUAGES CXX)
PROJECT(${PROJECT_NAME} ${PROJECT_ARGS})
CHECK_MINIMAL_CXX_STANDARD(11 ENFORCE)

# ----------------------------------------------------
# --- DEPENDENCIES -----------------------------------
# ----------------------------------------------------

# Set component to fetch from boost
# Get the python interface for the bindings
IF(BUILD_PYTHON_INTERFACE)
  FINDPYTHON()
  SEARCH_FOR_BOOST_PYTHON(REQUIRED)
ENDIF(BUILD_PYTHON_INTERFACE)

# ----------------------------------------------------
# --- INCLUDE ----------------------------------------
# ----------------------------------------------------

# --- MAIN LIBRARY -------------------------------------------------------------
SET(MASTER_BOARD_SRC
    src/ESPNOW_manager.cpp
    src/ESPNOW_types.cpp
    src/ETHERNET_types.cpp
    src/Link_manager.cpp
    src/master_board_interface.cpp
    src/motor.cpp
    src/motor_driver.cpp
)
ADD_LIBRARY(${PROJECT_NAME} SHARED
  ${MASTER_BOARD_SRC}
)
TARGET_LINK_LIBRARIES(${PROJECT_NAME} pthread)
target_include_directories(
  ${PROJECT_NAME} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
                         $<INSTALL_INTERFACE:include>)
install(
  TARGETS ${PROJECT_NAME}
  EXPORT ${TARGETS_EXPORT_NAME}
  DESTINATION lib)
install(DIRECTORY include/ DESTINATION include)

# --- BINDINGS -----------------------------------------------------------------

IF(BUILD_PYTHON_INTERFACE)
  # --- Setup the wrapper name and source files --- #
  SET(PYWRAP ${PROJECT_NAME}_pywrap)
  SET(${PYWRAP}_HEADERS
    srcpy/my_bindings_headr.h
  )
  SET(${PYWRAP}_SOURCES
    srcpy/my_bindings_headr.cpp
  )

  # --- Build the wrapper library --- #
  SET(${PYWRAP}_INSTALL_DIR ${PYTHON_SITELIB})
  ADD_LIBRARY(${PYWRAP} SHARED ${${PYWRAP}_SOURCES} ${${PYWRAP}_HEADERS})
  TARGET_INCLUDE_DIRECTORIES(${PYWRAP} SYSTEM PRIVATE ${PYTHON_INCLUDE_DIR})
  TARGET_LINK_LIBRARIES(${PYWRAP} ${PROJECT_NAME})
  TARGET_LINK_BOOST_PYTHON(${PYWRAP})
  SET_TARGET_PROPERTIES(${PYWRAP} PROPERTIES
    INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
  INSTALL(TARGETS ${PYWRAP} DESTINATION ${${PYWRAP}_INSTALL_DIR})
  
  # --- Allow to do: make python --- #
  ADD_CUSTOM_TARGET(python)
  SET_TARGET_PROPERTIES(python PROPERTIES EXCLUDE_FROM_DEFAULT_BUILD True)
  ADD_DEPENDENCIES(python ${PYWRAP})

  # --- Pre-build the python files for efficiency --- #
  PYTHON_BUILD(example example.py)
  PYTHON_BUILD(example listener.py)
  PYTHON_BUILD(example com_analyser.py)
ENDIF(BUILD_PYTHON_INTERFACE)


# --- CHECK EXAMPLES -----------------------------------------------------------
ADD_EXECUTABLE(master_board_example example/example.cpp)
TARGET_LINK_LIBRARIES(master_board_example ${PROJECT_NAME})
INSTALL(TARGETS master_board_example DESTINATION bin)

ADD_EXECUTABLE(master_board_example_pd example/example_pd.cpp)
TARGET_LINK_LIBRARIES(master_board_example_pd ${PROJECT_NAME})
INSTALL(TARGETS master_board_example_pd DESTINATION bin)
