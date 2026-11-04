#!/bin/sh
# NDS1 phase A host tests (record, template, draw order, emit and eye copy). Usage (repo root): sh 3ds/tools/nat_host_test.sh
set -e
cd "$(dirname "$0")/../.."
O=build3ds/nat; mkdir -p $O
export PATH=/c/msys64/mingw64/bin:$PATH; GCC=gcc
"$GCC" -O2 -Wall -Wno-unused-function -std=gnu11 -ffp-contract=off -I3ds/include -o $O/nat_host_test 3ds/tools/nat_host_test.c 3ds/src/pm_native.c 3ds/src/pm_comb.c -lm
$O/nat_host_test
