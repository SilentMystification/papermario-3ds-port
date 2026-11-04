#!/bin/sh
# NDT2 host tests (combiner, matrix, fp24 dp4, record). Usage (repo root): sh 3ds/tools/ndt_host_test.sh
set -e
cd "$(dirname "$0")/../.."
O=build3ds/ndt; mkdir -p $O
export PATH=/c/msys64/mingw64/bin:$PATH; GCC=gcc
"$GCC" -O2 -Wall -Wno-unused-function -std=gnu11 -ffp-contract=off -I3ds/include -o $O/ndt_host_test 3ds/tools/ndt_host_test.c 3ds/src/pm_comb.c -lm
$O/ndt_host_test
