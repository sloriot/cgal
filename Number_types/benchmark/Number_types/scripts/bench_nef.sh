#!/bin/bash
# > sh ./bench_nef

cd /Users/monet/Documents/fork/pull-requests/leda-benchmarks/builds/benchmarks-release/nef-3/

# CMAKE_CXX_FLAGS: -DCGAL_DO_NOT_USE_BOOST_MP
cd gmp-all # gmp without boost mp
./bench

# CMAKE_CXX_FLAGS: -DCGAL_DO_NOT_USE_BOOST_MP
# CGAL_DISABLE_GMPXX ON
cd ../gmp-without-xx # gmp without boost mp and gmpxx
./bench

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
cd ../leda # leda
./bench

# CGAL_DISABLE_GMPXX ON
cd ../boost-mp-latest # gmp without gmpxx but with boost mp
./bench

# CGAL_DISABLE_GMPXX ON
# LINK AGAINST BOOST MASTER
# cd ../boost-mp-master # gmp without gmpxx but with boost mp
# ./bench