#!/bin/sh
# CONV2C2 C2a: host test of gen_ntab_entries.py on the shapes of 3ds/tests/gne/cases (expected output a.txt). The seeded fault (the
# designator line removed from a copy of the tool) must give a different output. Prints one line; exit 1 on a fail.
T=/tmp/gne
mkdir -p $T
python3 -I 3ds/tools/gen_ntab_entries.py 3ds/tests/gne/cases > $T/out.txt || { echo "gne FAIL tool exit"; exit 1; }
if ! cmp -s $T/out.txt 3ds/tests/gne/a.txt; then echo "gne FAIL output differs from a.txt"; diff $T/out.txt 3ds/tests/gne/a.txt | head -20; exit 1; fi
sed '/# designators/d' 3ds/tools/gen_ntab_entries.py > $T/fault_tool.py
python3 -I $T/fault_tool.py 3ds/tests/gne/cases > $T/fault.txt || true
if cmp -s $T/fault.txt 3ds/tests/gne/a.txt; then echo "gne FAIL seeded fault not seen"; exit 1; fi
echo "gne ok"
