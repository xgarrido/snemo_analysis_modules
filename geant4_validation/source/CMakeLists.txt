# - Top level CMakeLists.txt for Geant4 validation program

cmake_minimum_required(VERSION 2.8 FATAL_ERROR)
project(geant4_validation)

if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
  message(STATUS "${PROJECT_NAME} requires an out-of-source build.")
  message(STATUS "Please remove these files from ${CMAKE_BINARY_DIR} first:")
  message(STATUS "  CMakeCache.txt")
  message(STATUS "  CMakeFiles")
  message(STATUS "Once these files are removed, create a separate directory")
  message(STATUS "and run CMake from there, pointing it to:")
  message(STATUS "  ${CMAKE_SOURCE_DIR}")
  message(FATAL_ERROR "in-source build detected")
endif()

# - Third party
find_package(mctools 1.0.0 REQUIRED)

include_directories(${PROJECT_SOURCE_DIR} ${mctools_INCLUDE_DIRS})

add_library(geant4_validation SHARED)

target_link_libraries(geant4_validation ${mctools_LIBRARY_DIR})

install(FILES
  ${PROJECT_BINARY_DIR}/libgeant4_validation${CMAKE_SHARED_LIBRARY_SUFFIX}
  DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
  PERMISSIONS OWNER_READ OWNER_EXECUTE
  GROUP_READ GROUP_EXECUTE
  WORLD_READ WORLD_EXECUTE
  )

# end of CMakeLists.txt