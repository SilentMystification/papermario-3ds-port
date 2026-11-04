#!/bin/sh
# nd_size_test.sh [source.c]: hot loop size test (plan_native_system 9.5, gate G5). Default source: the fixture 3ds/tools/nd_size_stub.c
# (nd_core.c comes in S1-2). Compiles with the docker image pm3ds-build:1 arm-none-eabi-gcc: the flags of 3ds/CMakeLists.txt, plus the
# 3DS arch flags of devkitPro (-march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft, not in this repo). Then nm -S and objdump -dr, and
# nd_size_check.py. Exit 0 pass, 1 over the limit, 2 build or tool error.
cd "$(dirname "$0")/../.." || exit 2
SRC=${1:-3ds/tools/nd_size_stub.c}
OUT=3ds/logs/perf/nd_size; mkdir -p "$OUT"
ROOT=$(pwd -W 2>/dev/null || pwd)
MSYS_NO_PATHCONV=1 docker run --rm -v "$ROOT:/w" -w /w pm3ds-build:1 sh -c "
  set -e
  export PATH=/opt/devkitpro/devkitARM/bin:\$PATH
  arm-none-eabi-gcc -O2 -fsigned-char -fno-strict-aliasing -fwrapv -ffunction-sections -fdata-sections -std=gnu11 \
    -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft -Dnullptr=NULL -Iinclude -I3ds/include -c $SRC -o /tmp/nd.o
  arm-none-eabi-nm -S /tmp/nd.o > $OUT/nm.txt
  arm-none-eabi-objdump -dr /tmp/nd.o > $OUT/objdump.txt
" || { echo "NDSIZE build failed"; exit 2; }
python 3ds/tools/nd_size_check.py "$OUT/nm.txt" "$OUT/objdump.txt"
