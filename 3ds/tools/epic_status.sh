#!/bin/sh
# epic_status.sh: the totals of the display list epic, straight from the tools. Never sum the listcen lines by hand
# (a hand sum gave wrong counters on 2026-11-16). Run from any folder of the repo.
cd "$(git rev-parse --show-toplevel)" || exit 2
echo "main $(git rev-parse --short HEAD)"
python 3ds/tools/listcen.py 2>&1 | grep '^listcen total' | grep -v '_n64'
if [ -f build3ds/pm_3ds.elf ]; then
    python 3ds/tools/elfgate.py build3ds/pm_3ds.elf 2>&1 | tail -3
else
    echo "elfgate: no build3ds/pm_3ds.elf (build first)"
fi
