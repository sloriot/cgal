#!/bin/bash
# > sh ./bench_nt

# PARAMETERS:
NEFTYPE="nef"
PMPTYPE="pmp"
N=1 # number of iterations

cd /Users/monet/Documents/fork/pull-requests/leda-benchmarks/builds/benchmarks-release/gmp-all/

# EXACT TYPES:

# ----- 1 -----
# CMAKE_CXX_FLAGS: -DCGAL_DO_NOT_USE_BOOST_MP
# cd gmp-all # gmp without boost mp

# ----- 2 -----
# CMAKE_CXX_FLAGS: -DCGAL_DO_NOT_USE_BOOST_MP
# CGAL_DISABLE_GMPXX ON
# cd gmp-without-xx # gmp without boost mp and gmpxx

# ----- 3 -----
# CMAKE_CXX_FLAGS: -DCGAL_DO_NOT_USE_BOOST_MP
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
# cd leda # leda

# ----- 4 -----
# CGAL_DISABLE_GMPXX ON
# cd boost-mp-without-gmpxx # gmp without gmpxx but with boost mp

# ----- 5 -----
# CGAL_DISABLE_GMP ON
# CGAL_DISABLE_GMPXX ON
# cd boost-mp-without-gmp # boost mp without gmp

# ----- 6 -----
# CMAKE_CXX_FLAGS: -DCGAL_DO_NOT_USE_BOOST_MP
# CGAL_DISABLE_GMP ON
# CGAL_DISABLE_GMPXX ON
# cd core # cgal core only

echo " "
echo "NEF 3 BENCHMARKS"

cd ../gmp-all
./bench $NEFTYPE $N
cd ../gmp-without-xx
./bench $NEFTYPE $N
# cd ../leda # very slow
# ./bench $NEFTYPE $N
cd ../boost-mp-without-gmpxx
./bench $NEFTYPE $N
cd ../boost-mp-without-gmp
./bench $NEFTYPE $N
cd ../core
./bench $NEFTYPE $N

echo " "
echo "PMP BENCHMARKS"

cd ../gmp-all
./bench $PMPTYPE $N
cd ../gmp-without-xx
./bench $PMPTYPE $N
cd ../leda
./bench $PMPTYPE $N
cd ../boost-mp-without-gmpxx
./bench $PMPTYPE $N
cd ../boost-mp-without-gmp
./bench $PMPTYPE $N
cd ../core
./bench $PMPTYPE $N
