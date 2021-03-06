# - Top level CMakeLists.txt for SuperNEMO gamma tracking studies

cmake_minimum_required(VERSION 2.8 FATAL_ERROR)
project(snemo_alpha_delayed_studies)

# Use C++11
set(CMAKE_CXX_FLAGS "-W -Wall -std=c++11")

# - Third party
find_package(Falaise 1.0.0 REQUIRED)

include_directories(${PROJECT_SOURCE_DIR} ${Falaise_INCLUDE_DIRS})

add_library(snemo_alpha_delayed_studies SHARED
  snemo_alpha_delayed_studies_module.h snemo_alpha_delayed_studies_module.cc)

set(Falaise_PID_DIR "${Falaise_INCLUDE_DIR}/../lib64/Falaise/modules")
set(Falaise_PID_LIBRARY "${Falaise_PID_DIR}/libFalaise_ParticleIdentification.so")
target_link_libraries(snemo_alpha_delayed_studies ${Falaise_LIBRARIES} ${Falaise_PID_LIBRARY})

install(FILES
  ${PROJECT_BINARY_DIR}/libsnemo_alpha_delayed_studies${CMAKE_SHARED_LIBRARY_SUFFIX}
  DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
  PERMISSIONS OWNER_READ OWNER_EXECUTE
  GROUP_READ GROUP_EXECUTE
  WORLD_READ WORLD_EXECUTE
  )

# end of CMakeLists.txt