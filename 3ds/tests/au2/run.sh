#!/bin/sh
# Host test of the AU2 ring. Run from the repo root.
set -e
cd "$(dirname "$0")/../../.."
OUT=${TMPDIR:-/tmp}/au2_test
gcc -O2 -g -fsanitize=thread -pthread -I3ds/tests/au2 -I3ds/include -Wall -o "$OUT" 3ds/tests/au2/test_queue.c 3ds/src/pm_au2.c
"$OUT"
