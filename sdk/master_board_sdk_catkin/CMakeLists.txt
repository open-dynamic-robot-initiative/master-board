cmake_minimum_required(VERSION 2.8.3)
project(master_board_sdk_catkin)

# required to use std::shared_ptr in code and to link the python bindings
if(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wl,--no-as-needed")
endif()
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Wextra -pthread")

###########
# OPTIONS #
###########
OPTION (BUILD_PYTHON_INTERFACE "Build the python binding" OFF)

###################
# Path to the sdk #
###################
set(source_sdk ${CMAKE_CURRENT_SOURCE_DIR}/../master_board_sdk)

################
# Dependencies #
################
find_package(catkin REQUIRED COMPONENTS mpi_cmake_modules)

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

# PATH to the sdk headers
include_directories(${source_sdk}/include)

###################################
# Declare the libraries to export #
###################################
set(export_lib ${PROJECT_NAME})
IF(BUILD_PYTHON_INTERFACE)
  set(export_lib ${PROJECT_NAME}_pywrap ${export_lib})
ENDIF(BUILD_PYTHON_INTERFACE)

catkin_package(
    INCLUDE_DIRS ${source_sdk}/include
    CATKIN_DEPENDS
    LIBRARIES ${export_lib}
)

###############
# add library #
###############

SET(MASTER_BOARD_SRC
    ${source_sdk}/src/ESPNOW_manager.cpp
    ${source_sdk}/src/ESPNOW_types.cpp
    ${source_sdk}/src/ETHERNET_types.cpp
    ${source_sdk}/src/Link_manager.cpp
    ${source_sdk}/src/master_board_interface.cpp
    ${source_sdk}/src/motor.cpp
    ${source_sdk}/src/motor_driver.cpp
)
ADD_LIBRARY(${PROJECT_NAME} SHARED
  ${MASTER_BOARD_SRC}
)

###################
# Python bindings #
###################

IF(BUILD_PYTHON_INTERFACE)
  SET(PYWRAP ${PROJECT_NAME}_pywrap)  
  SET(${PYWRAP}_HEADERS
    # ${source_sdk}/srcpy/my_bindings.hpp
  )
  SET(${PYWRAP}_SOURCES
    # ${source_sdk}/srcpy/my_bindings.cpp
  )
    
  ADD_CUSTOM_TARGET(python)
  SET_TARGET_PROPERTIES(python PROPERTIES EXCLUDE_FROM_DEFAULT_BUILD True)

  # --- LIBRARY --- #
  SET(${PYWRAP}_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/${PYTHON_SITELIB}/${PROJECT_NAME})

  ADD_LIBRARY(${PYWRAP} SHARED ${${PYWRAP}_SOURCES} ${${PYWRAP}_HEADERS})
  ADD_DEPENDENCIES(python ${PYWRAP})

  IF(BUILD_WITH_COMMIT_VERSION)
    TAG_LIBRARY_VERSION(${PYWRAP})
  ENDIF(BUILD_WITH_COMMIT_VERSION)
  ADD_HEADER_GROUP(${PYWRAP}_HEADERS)
  ADD_SOURCE_GROUP(${PYWRAP}_SOURCES)

  TARGET_LINK_LIBRARIES(${PYWRAP} ${PROJECT_NAME})
  TARGET_LINK_BOOST_PYTHON(${PYWRAP})

  SET_TARGET_PROPERTIES(${PYWRAP} PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bindings/python/${PROJECT_NAME}")

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
      FILES "${CMAKE_CURRENT_BINARY_DIR}/master-board-sdk-py.pc"
      DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
      PERMISSIONS OWNER_READ GROUP_READ WORLD_READ OWNER_WRITE)
ENDIF(BUILD_PYTHON_INTERFACE)

#########
# demos #
#########

ADD_EXECUTABLE(demos_${PROJECT_NAME} ${source_sdk}/example/example.cpp)
TARGET_LINK_LIBRARIES(demos_${PROJECT_NAME} ${PROJECT_NAME})

##############
# Unit tests #
##############

# catkin_add_gtest(${PROJECT_NAME}_unit_tests
#   tests/main.cpp
# )

##########################
# building documentation #
##########################
build_doxygen_documentation()
