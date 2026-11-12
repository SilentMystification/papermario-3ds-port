#!/bin/sh
# BASEKEY host test: base store key. Run from anywhere.
cd "$(dirname "$0")/../../.." && python 3ds/tests/basekey/test_basekey.py
