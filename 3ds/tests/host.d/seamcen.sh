#!/bin/sh
# CRK2: seam census of drec.txt (3ds/tools/seamcen.py): the built-in selftest (two cases, exit 0 only when both are right).
python3 3ds/tools/seamcen.py --selftest || exit 1
