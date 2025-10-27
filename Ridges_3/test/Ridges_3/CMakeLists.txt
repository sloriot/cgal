# Created by the script cgal_create_cmake_script
# This is the CMake script for compiling a CGAL application.

cmake_minimum_required(VERSION 3.12...3.31)
project(Ridges_3_Tests)

find_package(CGAL REQUIRED)

find_package(Eigen3 QUIET) # (3.1.0 or greater)
if (EIGEN3_FOUND OR Eigen3_FOUND)
  if ("${Eigen3_VERSION}" VERSION_LESS "3.1.0")
    find_package(Eigen3 3.1 QUIET) # (3.1.0 or greater)
  endif()
endif()

include(CGAL_Eigen3_support)
if(TARGET CGAL::Eigen3_support)
  create_single_source_cgal_program("ridge_test.cpp")
  target_link_libraries(ridge_test PRIVATE CGAL::Eigen3_support)
else()
  message("NOTICE: This project requires Eigen 3.1 (or greater), and will not be compiled.")
endif()
