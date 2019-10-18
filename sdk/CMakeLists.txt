#
# Copyright (c) 2019 CNRS
#

CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

# These variables have to be defined before running SETUP_PROJECT
SET(PROJECT_NAME master-board)
SET(PROJECT_DESCRIPTION "This project contains the sdk for the communication between a computer and the master-board")
SET(PROJECT_URL https://github.com/open-dynamic-robot-initiative/master-board)

PROJECT(${PROJECT_NAME} CXX)

INCLUDE(cmake/base.cmake)
INCLUDE(cmake/boost.cmake)
INCLUDE(cmake/python.cmake)

# --- OPTIONS ----------------------------------------
OPTION (BUILD_PYTHON_INTERFACE "Build the python binding" ON)

MACRO(TAG_LIBRARY_VERSION target)
  SET_TARGET_PROPERTIES(${target} PROPERTIES SOVERSION ${PROJECT_VERSION})
ENDMACRO(TAG_LIBRARY_VERSION)

# ----------------------------------------------------
# --- DEPENDENCIES -----------------------------------
# ----------------------------------------------------

# Set component to fetch from boost
SET(BOOST_REQUIERED_COMPONENTS "")
SET(BOOST_OPTIONAL_COMPONENTS "")
# Get the python interface for the bindings
IF(BUILD_PYTHON_INTERFACE)
  SET(BOOST_OPTIONAL_COMPONENTS ${BOOST_OPTIONAL_COMPONENTS} python)
  FINDPYTHON()
  INCLUDE_DIRECTORIES(SYSTEM ${PYTHON_INCLUDE_DIRS}) 
ENDIF(BUILD_PYTHON_INTERFACE)
# Search for the boost libraries
SET(BOOST_COMPONENTS ${BOOST_REQUIERED_COMPONENTS} ${BOOST_OPTIONAL_COMPONENTS} ${BOOST_BUILD_COMPONENTS})
SEARCH_FOR_BOOST()
# Path to boost headers
INCLUDE_DIRECTORIES(SYSTEM ${Boost_INCLUDE_DIRS})

# ----------------------------------------------------
# --- INCLUDE ----------------------------------------
# ----------------------------------------------------
INCLUDE_DIRECTORIES(include)

# ----------------------------------------------------
# --- CXX FLAGS --------------------------------------
# ----------------------------------------------------

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
SET(CXX_DISABLE_WERROR True)
SET(CMAKE_VERBOSE_MAKEFILE True)

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
add_library(master-board SHARED
  ${MASTER_BOARD_SRC}
)

# --- BINDINGS -----------------------------------------------------------------


# --- CHECK EXAMPLES -----------------------------------------------------------


# --- PACKAGING ----------------------------------------------------------------
PKG_CONFIG_APPEND_LIBS (${PROJECT_NAME})
PKG_CONFIG_APPEND_BOOST_LIBS(${BOOST_REQUIERED_COMPONENTS})

FOREACH(cflags ${CFLAGS_DEPENDENCIES})
  PKG_CONFIG_APPEND_CFLAGS(${cflags})
ENDFOREACH(cflags ${CFLAGS_DEPENDENCIES})

SETUP_PROJECT_FINALIZE()
