#!/bin/sh
# build, dedup stubs, rebuild once, print remaining link problems
cd "$(dirname "$0")/../.."
sh 3ds/build.sh > build3ds/c01.log 2>&1
if grep -q "multiple definition" build3ds/c01.log; then
python 3ds/tools/dedup_stubs.py < build3ds/c01.log >/dev/null
sh 3ds/build.sh > build3ds/c01.log 2>&1
fi
grep "multiple definition\|undefined reference\|error:" build3ds/c01.log | sed "s/.*\(multiple definition of\|undefined reference to\)/\1/" | cut -c1-160 | sort -u
