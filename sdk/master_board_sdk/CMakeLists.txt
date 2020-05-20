#
# Copyright (c) 2019 CNRS
#

CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

# ----------------------------------------------------
# --- CXX FLAGS --------------------------------------
# ----------------------------------------------------

ADD_DEFINITIONS(-pthread)
SET(CMAKE_CXX_STANDARD 11)
SET(CXX_DISABLE_WERROR True)
SET(CMAKE_VERBOSE_MAKEFILE True)

# These variables have to be defined before running SETUP_PROJECT
SET(PROJECT_NAME master-board-sdk)
SET(PROJECT_DESCRIPTION "This project contains the sdk for the communication between a computer and the master-board")
SET(PROJECT_URL https://github.com/open-dynamic-robot-initiative/master-board)

INCLUDE(cmake/base.cmake)
INCLUDE(cmake/boost.cmake)
INCLUDE(cmake/python.cmake)
INCLUDE(cmake/ide.cmake)

COMPUTE_PROJECT_ARGS(PROJECT_ARGS LANGUAGES CXX)
PROJECT(${PROJECT_NAME} ${PROJECT_ARGS})

# --- OPTIONS ----------------------------------------
OPTION (BUILD_PYTHON_INTERFACE "Build the python binding" OFF)

MACRO(TAG_LIBRARY_VERSION target)
  SET_TARGET_PROPERTIES(${target} PROPERTIES SOVERSION ${PROJECT_VERSION})
ENDMACRO(TAG_LIBRARY_VERSION)

# ----------------------------------------------------
# --- DEPENDENCIES -----------------------------------
# ----------------------------------------------------

# Set component to fetch from boost
SET(BOOST_REQUIRED_COMPONENTS "")
SET(BOOST_OPTIONAL_COMPONENTS "")
# Get the python interface for the bindings
IF(BUILD_PYTHON_INTERFACE)
  SET(BOOST_REQUIRED_COMPONENTS ${BOOST_REQUIRED_COMPONENTS} python)
  FINDPYTHON()
  INCLUDE_DIRECTORIES(SYSTEM ${PYTHON_INCLUDE_DIRS})
ENDIF(BUILD_PYTHON_INTERFACE)
# Search for the boost libraries
SET(BOOST_COMPONENTS ${BOOST_REQUIRED_COMPONENTS} ${BOOST_OPTIONAL_COMPONENTS} ${BOOST_BUILD_COMPONENTS})
SEARCH_FOR_BOOST()
# Path to boost headers
INCLUDE_DIRECTORIES(SYSTEM ${Boost_INCLUDE_DIRS})

# ----------------------------------------------------
# --- INCLUDE ----------------------------------------
# ----------------------------------------------------
INCLUDE_DIRECTORIES(include)


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
INSTALL(TARGETS ${PROJECT_NAME} DESTINATION lib)

# --- BINDINGS -----------------------------------------------------------------

IF(BUILD_PYTHON_INTERFACE)
  # SET(PYWRAP ${PROJECT_NAME}_pywrap)
  SET(PYWRAP master_board_sdk_pywrap)
  SET(${PYWRAP}_HEADERS
    srcpy/my_bindings_headr.h
  )
  SET(${PYWRAP}_SOURCES
    srcpy/my_bindings_headr.cpp
  )

  ADD_CUSTOM_TARGET(python)
  SET_TARGET_PROPERTIES(python PROPERTIES EXCLUDE_FROM_DEFAULT_BUILD True)

  # --- LIBRARY --- #
  SET(${PYWRAP}_INSTALL_DIR ${PYTHON_SITELIB})

  ADD_LIBRARY(${PYWRAP} SHARED ${${PYWRAP}_SOURCES} ${${PYWRAP}_HEADERS})
  ADD_DEPENDENCIES(python ${PYWRAP})

  IF(BUILD_WITH_COMMIT_VERSION)
    TAG_LIBRARY_VERSION(${PYWRAP})
  ENDIF(BUILD_WITH_COMMIT_VERSION)
  ADD_HEADER_GROUP(${PYWRAP}_HEADERS)
  ADD_SOURCE_GROUP(${PYWRAP}_SOURCES)

  TARGET_LINK_LIBRARIES(${PYWRAP} ${PROJECT_NAME})
  TARGET_LINK_BOOST_PYTHON(${PYWRAP})

  INSTALL(TARGETS ${PYWRAP} DESTINATION ${${PYWRAP}_INSTALL_DIR})
  # --- PACKAGING --- #
  # Format string
  SET(_PKG_CONFIG_PYWRAP_LIBDIR ${${PYWRAP}_INSTALL_DIR})
  SET(_PKG_CONFIG_PYWRAP_BINDIR ${${PYWRAP}_INSTALL_DIR})
  SET(_PKG_CONFIG_PYWRAP_CONFLICTS)
  SET(_PKG_CONFIG_PYWRAP_REQUIRES "${PROJECT_NAME}")
  FOREACH(dep ${PKG_CONFIG_PYWRAP_REQUIRES})
    SET(_PKG_CONFIG_PYWRAP_REQUIRES "${_PKG_CONFIG_PYWRAP_REQUIRES}, ${dep}")
  ENDFOREACH(dep ${PKG_CONFIG_PYWRAP_REQUIRES})
  SET(_PKG_CONFIG_PYWRAP_LIBS "-L\${libdir} -l${PYWRAP}")
  IF(APPLE)
    SET(_PKG_CONFIG_PYWRAP_LIBS "${_PKG_CONFIG_PYWRAP_LIBS} -Wl,-undefined,dynamic_lookup,${Boost_${UPPERCOMPONENT}_LIBRARY}")
  ELSE(APPLE)
    SET(_PKG_CONFIG_PYWRAP_LIBS "${_PKG_CONFIG_PYWRAP_LIBS} ${LIBINCL_KW}boost_python")
  ENDIF(APPLE)
  SET(_PKG_CONFIG_PYWRAP_CFLAGS "-I\${includedir} -I/usr/local/include")
  SET(_PKG_CONFIG_PYWRAP_CFLAGS "${_PKG_CONFIG_PYWRAP_CFLAGS} -I${PYTHON_INCLUDE_DIRS}")
  FOREACH(cflags ${CFLAGS_DEPENDENCIES})
    SET(_PKG_CONFIG_PYWRAP_CFLAGS "${_PKG_CONFIG_PYWRAP_CFLAGS} ${cflags}")
  ENDFOREACH(cflags ${CFLAGS_DEPENDENCIES})
  CONFIGURE_FILE(
     "${CMAKE_CURRENT_SOURCE_DIR}/srcpy/master-board-sdk-py.pc.cmake"
     "${CMAKE_CURRENT_BINARY_DIR}/srcpy/master-board-sdk-py.pc")

  INSTALL(
      FILES "${CMAKE_CURRENT_BINARY_DIR}/srcpy/master-board-sdk-py.pc"
      DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
      PERMISSIONS OWNER_READ GROUP_READ WORLD_READ OWNER_WRITE)

  PYTHON_BUILD(example example.py)
  PYTHON_BUILD(example listener.py)
  PYTHON_BUILD(example latency.py)
ENDIF(BUILD_PYTHON_INTERFACE)


# --- CHECK EXAMPLES -----------------------------------------------------------
ADD_EXECUTABLE(master-board-example example/example.cpp)
TARGET_LINK_LIBRARIES(master-board-example ${PROJECT_NAME})
INSTALL(TARGETS master-board-example DESTINATION bin)

# --- PACKAGING ----------------------------------------------------------------
PKG_CONFIG_APPEND_LIBS (${PROJECT_NAME})
PKG_CONFIG_APPEND_BOOST_LIBS(${BOOST_REQUIRED_COMPONENTS})

FOREACH(cflags ${CFLAGS_DEPENDENCIES})
  PKG_CONFIG_APPEND_CFLAGS(${cflags})
ENDFOREACH(cflags ${CFLAGS_DEPENDENCIES})
