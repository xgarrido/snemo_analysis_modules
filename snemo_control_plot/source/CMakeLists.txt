# - Top level CMakeLists.txt for SuperNEMO control plot program

cmake_minimum_required(VERSION 2.8 FATAL_ERROR)
project(snemo_control_plot)

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
find_package(Falaise 1.0.0 REQUIRED)

include_directories(${PROJECT_SOURCE_DIR} ${Falaise_INCLUDE_DIRS})

add_library(snemo_control_plot SHARED
  snemo_control_plot_module.h
  snemo_control_plot_module.cc
  base_plotter.h
  base_plotter.cc
  simulated_data_plotter.h
  simulated_data_plotter.cc
  calibrated_data_plotter.h
  calibrated_data_plotter.cc
  tracker_clustering_data_plotter.h
  tracker_clustering_data_plotter.cc)

target_link_libraries(snemo_control_plot ${Falaise_LIBRARIES})

install(FILES
  ${PROJECT_BINARY_DIR}/libsnemo_control_plot${CMAKE_SHARED_LIBRARY_SUFFIX}
  DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
  PERMISSIONS OWNER_READ OWNER_EXECUTE
  GROUP_READ GROUP_EXECUTE
  WORLD_READ WORLD_EXECUTE
  )

# end of CMakeLists.txt