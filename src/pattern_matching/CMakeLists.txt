
cmake_minimum_required(VERSION 3.12)
project(pattern_matching)

# Set STEPCODE_ROOT_PATH
set(STEPCODE_ROOT_DIR "${CMAKE_SOURCE_DIR}/../../stepcode" CACHE STRING "${CMAKE_SOURCE_DIR}/../../stepcode")

# Set STEPCODE_ROOT_DIR to point to the root of the STEPcode source tree.
if(NOT DEFINED STEPCODE_ROOT_DIR)
  message(FATAL_ERROR "STEPCODE_ROOT_DIR is not defined! Set via cmake-gui or on command line:\n    cmake .. -DSTEPCODE_ROOT_DIR=/path/to/sc")
endif(NOT DEFINED STEPCODE_ROOT_DIR)

# STEPCODE_ROOT_DIR is relative or absolute path?
if(EXISTS "${CMAKE_BINARY_DIR}/${STEPCODE_ROOT_DIR}/src/express/express.c")
  set(STEPCODE_ROOT_DIR "${CMAKE_BINARY_DIR}/${STEPCODE_ROOT_DIR}")
  message("** STEPCODE_ROOT_DIR is a relative path; converted to absolute path: ${STEPCODE_ROOT_DIR}.")
else()
  if(NOT EXISTS "${STEPCODE_ROOT_DIR}/src/express/express.c")
    message(FATAL_ERROR "****  Cannot locate STEPCODE_ROOT_DIR - try an absolute path.")
  endif(NOT EXISTS "${STEPCODE_ROOT_DIR}/src/express/express.c")
endif(EXISTS "${CMAKE_BINARY_DIR}/${STEPCODE_ROOT_DIR}/src/express/express.c")


# Use STEPcode as library, but build from this build process.
set(SC_IS_SUBBUILD TRUE)

# Build type, DEBUG or RELEASE
if(NOT DEFINED CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif(NOT DEFINED CMAKE_BUILD_TYPE)
set(SC_BUILD_TYPE ${CMAKE_BUILD_TYPE})

# Path to STEPcode
set(STEPCODE_BUILD_DIR ${CMAKE_BINARY_DIR}/sc CACHE PATH "STEPcode build subdir")

# STEP schema to build and link against.
set(SCHEMA ap203)
set(SC_BUILD_SCHEMAS ${SCHEMA} CACHE INTERNAL "Schema, internal, immutable" FORCE)

# Parse out schema name and library.
set(SCHEMA_PATH "${STEPCODE_ROOT_DIR}/data/${SCHEMA}/*.exp")
file(GLOB SCHEMA_FILE "${SCHEMA_PATH}")

if(NOT EXISTS ${SCHEMA_FILE})
  message(FATAL_ERROR "Cannot find the schema; cannot continue. Looked for ${SCHEMA_PATH} from ${CMAKE_CURRENT_LIST_DIR}")
endif(NOT EXISTS ${SCHEMA_FILE})


get_filename_component(SCHEMA_SN ${SCHEMA_FILE} NAME)
set(SCHEMA_LINK_NAME "sdai_${SCHEMA}")
set(STEPCODE_LIBRARIES stepcore stepeditor stepdai steputils ${SCHEMA_LINK_NAME})

# Add STEPCode project to CMake build.
add_subdirectory(${STEPCODE_ROOT_DIR} "${CMAKE_CURRENT_BINARY_DIR}/sc" EXCLUDE_FROM_ALL)

# Set up STEPcode include directories.
set(STEPCODE_INCLUDE_DIR
  ${STEPCODE_ROOT_DIR}/src/clstepcore
  ${STEPCODE_ROOT_DIR}/src/cldai
  ${STEPCODE_ROOT_DIR}/src/clutils
  ${STEPCODE_ROOT_DIR}/src/cleditor
  ${STEPCODE_BUILD_DIR}/include
  ${STEPCODE_ROOT_DIR}/include
  ${CMAKE_BINARY_DIR}
  ${STEPCODE_BUILD_DIR}/schemas/${SCHEMA_LINK_NAME}
 )

set(YAML_CPP_LIBRARY_DIR ${CMAKE_SOURCE_DIR}/../yaml-cpp/)

set(YAML_CPP_INCLUDE_DIR
   ${YAML_CPP_LIBRARY_DIR}
   ${YAML_CPP_LIBRARY_DIR}/include
)

set(YAML_CPP_LIBRARIES ${YAML_CPP_INCLUDE_DIR})

add_subdirectory(${YAML_CPP_LIBRARY_DIR} "${CMAKE_CURRENT_BINARY_DIR}/yaml-cpp" EXCLUDE_FROM_ALL)


# Reset sane values for build destination.  Otherwise, everything ends up in ${CMAKE_BINARY_DIR}/sc
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include_directories(
  ${STEPCODE_INCLUDE_DIR}
  ${YAML_CPP_INCLUDE_DIR}
 )

# Application sources and headers
set(SRCS 
  main.cpp
)

set(HDRS 
)

add_executable(${PROJECT_NAME} ${SRCS} ${HDRS})

target_link_libraries(${PROJECT_NAME} ${STEPCODE_LIBRARIES})
target_link_libraries(${PROJECT_NAME} yaml-cpp)

# Visual Studio Working Folder
if (CMAKE_GENERATOR MATCHES "Visual Studio")
  set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
endif()

# Check .gitignore
if(NOT EXISTS ${CMAKE_BINARY_DIR}/.gitignore)
  file(WRITE ${CMAKE_BINARY_DIR}/.gitignore "*")
endif()


