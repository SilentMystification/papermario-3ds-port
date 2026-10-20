#!/bin/sh
# PG2a host test. Usage (repo root): sh 3ds/tools/texbake_test.sh
# 1 extract the CPU decode path from pm_tex.c, 2 build the request corpus, 3 compare tool output to it, 4 bake a pack and print the estimate.
set -e
cd "$(dirname "$0")/../.."
O=build3ds/texbake; mkdir -p $O
python3 -I 3ds/tools/texbake_ref.py 3ds/src/pm_tex.c build3ds/texbake_ref.h
python3 -I 3ds/tools/texbake_corpus.py . $O/corpus.req
gcc -O2 -Wall -Wno-unused-function -o $O/texbake_test 3ds/tools/texbake_test.c
gcc -O2 -Wall -o $O/texbake 3ds/tools/texbake.c
$O/texbake_test $O/corpus.req
$O/texbake bake $O/corpus.req $O/corpus.pak
