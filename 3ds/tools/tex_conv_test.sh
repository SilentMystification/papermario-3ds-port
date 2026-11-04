#!/bin/sh
# Texture converter host test (TX1 path). No emulator. Usage (repo root): sh 3ds/tools/tex_conv_test.sh
set -e
cd "$(dirname "$0")/../.."
O=build3ds/texconv; mkdir -p $O
PATH=/c/msys64/mingw64/bin:$PATH; CC=${CC:-gcc}
$CC -O2 -Wall -Wno-unused-function -o $O/tex_conv_drive 3ds/tools/tex_conv_drive.c
PY=python3; "$PY" -c "" >/dev/null 2>&1 || PY=/c/Python314/python
$PY -I 3ds/tools/tex_conv_test.py "$(pwd)/$O/tex_conv_drive"
