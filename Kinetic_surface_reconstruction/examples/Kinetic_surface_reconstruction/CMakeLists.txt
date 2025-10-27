# Created by the script cgal_create_CMakeLists.
# This is the CMake script for compiling a set of CGAL applications.

cmake_minimum_required(VERSION 3.12...3.31)

project(Kinetic_surface_reconstruction_Examples)

find_package(CGAL REQUIRED)
include(CGAL_CreateSingleSourceCGALProgram)

find_package(Eigen3 QUIET) # (3.1.0 or greater)
if (EIGEN3_FOUND OR Eigen3_FOUND)
  if ("${Eigen3_VERSION}" VERSION_LESS "3.1.0")
    find_package(Eigen3 3.1 QUIET) # (3.1.0 or greater)
  endif()
endif()

include(CGAL_Eigen3_support)

if(TARGET CGAL::Eigen3_support)
  message(STATUS "Found Eigen")

  set(targets ksr_basic ksr_building ksr_parameters)

  foreach(target ${targets})
    create_single_source_cgal_program("${target}.cpp")
    target_link_libraries(${target} PRIVATE CGAL::Eigen3_support)
  endforeach()
else()
  message(ERROR "This program requires the Eigen library, and will not be compiled.")
endif()
