#!/bin/sh
# TX1 host test. Usage (repo root): sh 3ds/tools/tx1_test.sh
set -e
cd "$(dirname "$0")/../.."
O=build3ds/texbake; mkdir -p $O
python3 -I 3ds/tools/texbake_ref.py 3ds/src/pm_tex.c build3ds/texbake_ref.h
[ -f $O/corpus.req ] || python3 -I 3ds/tools/texbake_corpus.py . $O/corpus.req
gcc -O2 -Wall -Wno-unused-function -o $O/tx1_test 3ds/tools/tx1_test.c
$O/tx1_test $O/corpus.req
