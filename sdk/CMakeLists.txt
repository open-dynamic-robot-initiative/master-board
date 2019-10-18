#
# Copyright (c) 2015-2019 CNRS INRIA
# Copyright (c) 2015 Wandercraft, 86 rue de Paris 91400 Orsay, France.
#

CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

# Check if the submodule cmake have been initialized
IF(NOT EXISTS "${PROJECT_SOURCE_DIR}/cmake/base.cmake")
  MESSAGE(FATAL_ERROR "\nPlease run the following command first:\ngit submodule update --init\n")
ENDIF()

INCLUDE(cmake/base.cmake)
INCLUDE(cmake/boost.cmake)
INCLUDE(cmake/python.cmake)
INCLUDE(cmake/ide.cmake)

SET(PROJECT_NAME pinocchio)
SET(PROJECT_DESCRIPTION "A fast and flexible implementation of Rigid Body Dynamics algorithms and their analytical derivatives")
SET(PROJECT_URL "http://github.com/stack-of-tasks/pinocchio")
SET(PROJECT_CUSTOM_HEADER_EXTENSION "hpp")

SET(DOXYGEN_USE_MATHJAX YES)

# Disable -Werror on Unix for now.
SET(CXX_DISABLE_WERROR True)
SET(CMAKE_VERBOSE_MAKEFILE True)

SETUP_PROJECT()

IF(WIN32)
  SET(LINK copy_if_different)
ELSE(WIN32)
  SET(LINK create_symlink)
ENDIF(WIN32)

# --- OPTIONS ----------------------------------------
OPTION (BUILD_PYTHON_INTERFACE "Build the python binding" ON)

MACRO(TAG_LIBRARY_VERSION target)
  SET_TARGET_PROPERTIES(${target} PROPERTIES SOVERSION ${PROJECT_VERSION})
ENDMACRO(TAG_LIBRARY_VERSION)

# ----------------------------------------------------
# --- DEPENDENCIES -----------------------------------
# ----------------------------------------------------

SET(BOOST_REQUIERED_COMPONENTS filesystem serialization system)
# SET(BOOST_BUILD_COMPONENTS unit_test_framework)
SET(BOOST_OPTIONAL_COMPONENTS "")

IF(BUILD_PYTHON_INTERFACE)
  SET(BOOST_OPTIONAL_COMPONENTS ${BOOST_OPTIONAL_COMPONENTS} python)
  FINDPYTHON()
  INCLUDE_DIRECTORIES(SYSTEM ${PYTHON_INCLUDE_DIRS}) 
ENDIF(BUILD_PYTHON_INTERFACE)

SET(BOOST_COMPONENTS ${BOOST_REQUIERED_COMPONENTS} ${BOOST_OPTIONAL_COMPONENTS} ${BOOST_BUILD_COMPONENTS})
SEARCH_FOR_BOOST()

# Variable containing all the cflags definition relative to optional dependencies
SET(CFLAGS_DEPENDENCIES)
LIST(APPEND CFLAGS_DEPENDENCIES "-DBOOST_MPL_LIMIT_LIST_SIZE=30")

# Path to boost headers
INCLUDE_DIRECTORIES(SYSTEM ${Boost_INCLUDE_DIRS})

# ----------------------------------------------------
# --- INCLUDE ----------------------------------------
# ----------------------------------------------------
INCLUDE_DIRECTORIES(include)

# --- MAIN LIBRARY -------------------------------------------------------------


# --- BINDINGS -----------------------------------------------------------------


# --- CHECK EXAMPLES -----------------------------------------------------------


# --- PACKAGING ----------------------------------------------------------------
PKG_CONFIG_APPEND_LIBS (${PROJECT_NAME})
PKG_CONFIG_APPEND_BOOST_LIBS(${BOOST_REQUIERED_COMPONENTS})

FOREACH(cflags ${CFLAGS_DEPENDENCIES})
  PKG_CONFIG_APPEND_CFLAGS(${cflags})
ENDFOREACH(cflags ${CFLAGS_DEPENDENCIES})

SETUP_PROJECT_FINALIZE()
