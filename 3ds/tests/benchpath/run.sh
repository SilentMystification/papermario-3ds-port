#!/bin/sh
# BENCHPATH host test: bench.append_runs survives a worktree removed during the run. Run from anywhere.
cd "$(dirname "$0")/../../.." && python 3ds/tests/benchpath/test_benchpath.py
