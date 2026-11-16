#!/bin/sh
# BASEBIN: self-test of the base build store 3ds/tools/basebin.py (cache safety rule 4). Prints "basebin ok", exit 1 on a failure.
python3 3ds/tests/basebin/test_basebin.py
