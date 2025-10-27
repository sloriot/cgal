# Created by the script cgal_create_cmake_script
# This is the CMake script for compiling a CGAL application.

cmake_minimum_required(VERSION 3.12...3.31)
project(Surface_mesh_deformation_Demo)

set_property(DIRECTORY PROPERTY CGAL_NO_TESTING TRUE)

find_package(CGAL REQUIRED)

find_package(Eigen3 QUIET) # (3.1.91 or greater)
if (EIGEN3_FOUND OR Eigen3_FOUND)
  if ("${Eigen3_VERSION}" VERSION_LESS "3.1.91")
    find_package(Eigen3 3.1.91 QUIET) # (3.1.91 or greater)
  endif()
endif()

include(CGAL_Eigen3_support)
if(TARGET CGAL::Eigen3_support)
  create_single_source_cgal_program("deform_mesh_for_botsch08_format.cpp")
  target_link_libraries(deform_mesh_for_botsch08_format PRIVATE CGAL::Eigen3_support)
else()
  message("NOTICE: This program requires requires Eigen 3.1.91 (or greater) or later and will not be compiled.")
endif()
