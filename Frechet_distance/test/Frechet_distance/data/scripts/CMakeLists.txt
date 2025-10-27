# Created by the script cgal_create_cmake_script
# This is the CMake script for compiling a CGAL application.

cmake_minimum_required(VERSION 3.1...3.14)
project( classical_Frechet_distance )

find_package(CGAL REQUIRED)


find_package(Eigen3 QUIET) # (3.1.0 or greater)
if (EIGEN3_FOUND OR Eigen3_FOUND)
  if ("${Eigen3_VERSION}" VERSION_LESS "3.1.0")
    find_package(Eigen3 3.1 QUIET) # (3.1.0 or greater)
  endif()
endif()

include(CGAL_Eigen3_support)

create_single_source_cgal_program("Compute_classical_Frechet_distance_3.cpp")
create_single_source_cgal_program("txt2wkt_3.cpp")

if(TARGET CGAL::Eigen3_support)
create_single_source_cgal_program( "Compute_classical_Frechet_distance_100.cpp" )
target_link_libraries(Compute_classical_Frechet_distance_100 PUBLIC CGAL::Eigen3_support)
endif()
