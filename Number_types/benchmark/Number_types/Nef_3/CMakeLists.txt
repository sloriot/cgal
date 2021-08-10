cmake_minimum_required(VERSION 3.1...3.20)
project("Nef_3 Benchmark")

find_package(CGAL REQUIRED COMPONENTS Core)

add_executable(bench "intersection.cpp")
target_link_libraries(bench CGAL::CGAL)
