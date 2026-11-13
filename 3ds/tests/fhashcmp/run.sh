#!/bin/sh
# FRAMEHASH4 host test: fhash.txt extract, shot names, frame compare. Run from anywhere.
cd "$(dirname "$0")/../../.." && python 3ds/tests/fhashcmp/test_fhashcmp.py
