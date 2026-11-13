#!/bin/sh
# DEADSCAN fixture test: deadscan.py on fix_a.c (exit 1 on FAIL).
cd "$(dirname "$0")/../../.." && python 3ds/tests/deadscan/test_deadscan.py
