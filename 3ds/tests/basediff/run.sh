#!/bin/sh
# BASEKEEP host test: shots_diff_all names every differing shot. Run from anywhere.
cd "$(dirname "$0")/../../.." && python 3ds/tests/basediff/test_basediff.py
