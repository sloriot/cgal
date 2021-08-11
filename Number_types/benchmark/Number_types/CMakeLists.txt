cmake_minimum_required(VERSION 3.1...3.20)
project("NT Benchmark")

find_package(CGAL REQUIRED COMPONENTS Core)

add_executable(bench "bench_nt.cpp")
target_link_libraries(bench CGAL::CGAL)
