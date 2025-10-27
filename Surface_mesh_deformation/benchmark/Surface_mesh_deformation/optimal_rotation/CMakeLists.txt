cmake_minimum_required(VERSION 3.12...3.31)
project(benchmark_for_closest_rotation)

find_package(CGAL REQUIRED COMPONENTS Core)

find_package(Eigen3 QUIET) # (3.1.0 or greater)
if (EIGEN3_FOUND OR Eigen3_FOUND)
  if ("${Eigen3_VERSION}" VERSION_LESS "3.1.0")
    find_package(Eigen3 3.1 QUIET) # (3.1.0 or greater)
  endif()
endif()

include(CGAL_Eigen3_support)
if(TARGET CGAL::Eigen3_support)
  create_single_source_cgal_program("benchmark_for_concept_models.cpp")
  target_link_libraries(benchmark_for_concept_models PRIVATE CGAL::Eigen3_support)
else()
  message("NOTICE: This program requires requires Eigen 3.1 (or greater) or later and will not be compiled.")
endif()
