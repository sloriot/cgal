#!/bin/bash
# > sh ./bench_nt

# PARAMETERS:

NEFTYPE="nef" # nef benches
ARRTYPE="arr" # arrangment benches

N=10 # number of iterations

# USING RELEASE:
# You should run this bench from the directory that contains all builds
# with different configurations (see below) called as below:
# gmp-all, gmp-without-xx, boost-mp-without-gmpxx, boost-mp-without-gmp, boost-with-interval, cppint, core, leda.
cd /Users/monet/Documents/fork/pull-requests/leda-benchmarks/builds/benchmarks-release/gmp-all/

# EXACT RATIONAL TYPES (ET, see Exact_type_selector.h):

# ----- 1 -----
# gmp-all
# ET: typedef mpq_class Type;
# CMAKE_CXX_FLAGS: -DCGAL_DO_NOT_USE_BOOST_MP=1 -DCGAL_DO_NOT_RUN_TESTME=1
# cd gmp-all # gmp without boost mp

# ----- 2 -----
# gmp-without-xx
# ET: typedef Gmpq Type;
# CMAKE_CXX_FLAGS: -DCGAL_DO_NOT_USE_BOOST_MP=1 -DCGAL_DO_NOT_RUN_TESTME=1
# CGAL_DISABLE_GMPXX ON
# cd gmp-without-xx # gmp without boost mp and gmpxx

# ----- 3 -----
# boost-mp-without-gmpxx
# ET: typedef boost::multiprecision::mpq_rational Type;
# CMAKE_CXX_FLAGS: -DCGAL_DO_NOT_RUN_TESTME=1
# CGAL_DISABLE_GMPXX ON
# cd boost-mp-without-gmpxx # gmp without gmpxx but with boost mp

# ----- 4 -----
# boost-mp-without-gmp
# ET: typedef boost::multiprecision::cpp_rational Type;
# CMAKE_CXX_FLAGS: -DCGAL_DO_NOT_RUN_TESTME=1
# CGAL_DISABLE_GMP ON
# CGAL_DISABLE_GMPXX ON
# cd boost-mp-without-gmp # boost mp without gmp

# or if we want to use our to_interval():
# CMAKE_CXX_FLAGS: -DCGAL_DO_NOT_RUN_TESTME=1 -DCGAL_USE_TO_INTERVAL_WITH_BOOST=1

# ----- 5 -----
# cppint
# ET: typedef Quotient<boost::multiprecision::cpp_int> Type;
# CMAKE_CXX_FLAGS: -DCGAL_USE_CPP_INT=1 -DCGAL_DO_NOT_RUN_TESTME=1
# CGAL_DISABLE_GMP ON
# CGAL_DISABLE_GMPXX ON
# cd core # cgal core only

# ----- 6 -----
# core
# ET: typedef Quotient<MP_Float> Type;
# CMAKE_CXX_FLAGS: -DCGAL_DO_NOT_USE_BOOST_MP=1 -DCGAL_DO_NOT_RUN_TESTME=1
# CGAL_DISABLE_GMP ON
# CGAL_DISABLE_GMPXX ON
# cd core # cgal core only

# ----- 7 -----
# leda
# ET: typedef leda_rational Type;
# CMAKE_CXX_FLAGS: -DCGAL_DO_NOT_USE_BOOST_MP=1 -DCGAL_DO_NOT_RUN_TESTME=1
# CGAL_DISABLE_GMP ON
# CGAL_DISABLE_GMPXX ON
# WITH_LEDA ON
# CMAKE_EXE_LINKER_FLAGS: -L/opt/X11/lib -lX11 -lpthread
# CMAKE_MODULE_LINKER_FLAGS: -L/opt/X11/lib -lX11 -lpthread
# CMAKE_SHARED_LINKER_FLAGS: -L/opt/X11/lib -lX11 -lpthread
# LEDA_INCLUDE_DIR: /Users/monet/Documents/third-party/leda/src/incl
# LEDA_LIBRARIES: /Users/monet/Documents/third-party/leda/leda-release/libleda_numbers.dylib
# LEDA_LIBRARY_RELEASE: /Users/monet/Documents/third-party/leda/leda-release/libleda_numbers.dylib
# LEDA_LINKER_FLAGS: -L/opt/X11/lib -lX11 -lpthread
# Might also require on the arm machine:
# CMAKE_THREAD_LIBS_INIT: -lpthread
# cd leda # leda

echo " "
echo "MAKE ALL"

cd ../gmp-all
make
cd ../gmp-without-xx
make
cd ../boost-mp-without-gmpxx
make
cd ../boost-mp-without-gmp
make
cd ../boost-with-interval
make
cd ../cppint
make
cd ../core
make
cd ../leda
make

echo " "
echo "NEF BENCHMARKS"

cd ../gmp-all
./bench $NEFTYPE $N
cd ../gmp-without-xx
./bench $NEFTYPE $N

cd ../boost-mp-without-gmpxx
./bench $NEFTYPE $N
cd ../boost-mp-without-gmp
./bench $NEFTYPE $N
cd ../boost-with-interval
./bench $NEFTYPE $N

cd ../cppint
./bench $NEFTYPE $N

# cd ../core # very slow
# ./bench $NEFTYPE $N
# cd ../leda # very slow
# ./bench $NEFTYPE $N

echo " "
echo "ARR BENCHMARKS"

cd ../gmp-all
./bench $ARRTYPE $N
cd ../gmp-without-xx

./bench $ARRTYPE $N
cd ../boost-mp-without-gmpxx
./bench $ARRTYPE $N
cd ../boost-mp-without-gmp
./bench $ARRTYPE $N
cd ../boost-with-interval
./bench $ARRTYPE $N

cd ../cppint
./bench $ARRTYPE $N

cd ../core
./bench $ARRTYPE $N
cd ../leda
./bench $ARRTYPE $N

echo " "
