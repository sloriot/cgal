cmake_minimum_required(VERSION 3.1...3.20)
project("PMP Benchmark")

find_package(CGAL REQUIRED COMPONENTS Core)

add_executable(bench "corefinement_mesh_union_and_intersection.cpp")
target_link_libraries(bench CGAL::CGAL)
