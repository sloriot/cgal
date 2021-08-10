#!/bin/bash
# > sh ./bench_nef

cd /Users/monet/Documents/fork/pull-requests/leda-benchmarks/builds/benchmarks-release/nef-3/

cd gmp-all # default
./bench
cd ../gmp-without-xx # gmp without gmpxx
./bench
cd ../leda # leda
./bench
