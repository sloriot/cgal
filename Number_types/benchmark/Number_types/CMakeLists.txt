cmake_minimum_required(VERSION 3.1...3.20)
project("NT Benchmark")

find_package(CGAL REQUIRED COMPONENTS Core)

# If we want to use the develop branch of boost mp:
# include_directories("path_to/boost/multiprecision/");

add_executable(bench "bench_nt.cpp")
target_link_libraries(bench CGAL::CGAL)
