# Created by the script cgal_create_cmake_script
# This is the CMake script for compiling a CGAL application.

cmake_minimum_required(VERSION 3.12...3.31)
project(Orthtree_Examples)

find_package(CGAL REQUIRED OPTIONAL_COMPONENTS Core)

create_single_source_cgal_program("octree_build_from_point_set.cpp")
create_single_source_cgal_program("octree_build_from_point_vector.cpp")
create_single_source_cgal_program("octree_build_with_custom_split.cpp")
create_single_source_cgal_program("octree_find_nearest_neighbor.cpp")
create_single_source_cgal_program("octree_traversal_custom.cpp")
create_single_source_cgal_program("octree_traversal_manual.cpp")
create_single_source_cgal_program("octree_traversal_preorder.cpp")
create_single_source_cgal_program("octree_grade.cpp")
create_single_source_cgal_program("quadtree_build_from_point_vector.cpp")
create_single_source_cgal_program("octree_surface_mesh.cpp")
create_single_source_cgal_program("quadtree_build_manually.cpp")

find_package(Eigen3 QUIET) # (3.1.91 or greater)
if (EIGEN3_FOUND OR Eigen3_FOUND)
  if ("${Eigen3_VERSION}" VERSION_LESS "3.1.91")
    find_package(Eigen3 3.1.91 QUIET) # (3.1.91 or greater)
  endif()
endif()

include(CGAL_Eigen3_support)
if (TARGET CGAL::Eigen3_support)
  create_single_source_cgal_program("orthtree_build.cpp")
  target_link_libraries(orthtree_build PRIVATE CGAL::Eigen3_support)
endif()
