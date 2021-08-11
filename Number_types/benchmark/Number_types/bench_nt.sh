#!/bin/bash
# > sh ./bench_nt

# PARAMETERS:
NEFTYPE="nef"
PMPTYPE="pmp"
N=1 # number of iterations

# USING RELEASE:
cd /Users/monet/Documents/fork/pull-requests/leda-benchmarks/builds/benchmarks-release/gmp-all/

# EXACT RATIONAL TYPES (ET, see Exact_type_selector.h):

# ----- 1 -----
# ET: typedef mpq_class Type;
# CMAKE_CXX_FLAGS: -DCGAL_DO_NOT_USE_BOOST_MP=1
# cd gmp-all # gmp without boost mp

# ----- 2 -----
# ET: typedef Gmpq Type;
# CMAKE_CXX_FLAGS: -DCGAL_DO_NOT_USE_BOOST_MP=1
# CGAL_DISABLE_GMPXX ON
# cd gmp-without-xx # gmp without boost mp and gmpxx

# ----- 3 -----
# ET: typedef leda_rational Type;
# CMAKE_CXX_FLAGS: -DCGAL_DO_NOT_USE_BOOST_MP=1
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
# ET: typedef boost::multiprecision::mpq_rational Type;
# CGAL_DISABLE_GMPXX ON
# cd boost-mp-without-gmpxx # gmp without gmpxx but with boost mp

# ----- 5 -----
# ET: typedef BOOST_cpp_arithmetic_kernel::Rational Type;
# CGAL_DISABLE_GMP ON
# CGAL_DISABLE_GMPXX ON
# cd boost-mp-without-gmp # boost mp without gmp

# ----- 6 -----
# ET: typedef Quotient<MP_Float> Type;
# CMAKE_CXX_FLAGS: -DCGAL_DO_NOT_USE_BOOST_MP=1 -DCGAL_USE_CORE=1
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

echo " "