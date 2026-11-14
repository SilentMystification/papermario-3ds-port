#!/bin/sh
# FXSELF 1: host test of 3ds/tools/selfapply.py on 3ds/tests/selfapply (a.c, tab.txt; expected a.out.c and a.log). A seeded fault
# (all -> any in a copy of the tool) must change a.c or the log; the real tool must give a.out.c and a.log byte for byte.
# Prints "selfapply ok" or "selfapply FAIL ..."; exit 1 on a fail.
T=/tmp/selfapply
PY=python3
python3 -c 1 >/dev/null 2>&1 || PY=python
F=3ds/tests/selfapply
rm -rf $T; mkdir -p $T/fault $T/w
cp 3ds/tools/gen_ntab_entries.py $T/fault/
sed 's/all(n in tab/any(n in tab/' 3ds/tools/selfapply.py > $T/fault/selfapply.py
cmp -s 3ds/tools/selfapply.py $T/fault/selfapply.py && { echo "selfapply FAIL seeded fault did not change the tool"; exit 1; }

# run <tool dir>: runs <tool dir>/selfapply.py on a copy of a.c in $T/w (a.c, a.log left there)
run() {
    cp $F/a.c $F/tab.txt $T/w/ || return 1
    (cd $T/w && $PY -I $1/selfapply.py tab.txt a.c > a.log) || return 1
}

run $T/fault || { echo "selfapply FAIL seeded fault tool exit"; exit 1; }
if cmp -s $T/w/a.c $F/a.out.c && cmp -s $T/w/a.log $F/a.log; then echo "selfapply FAIL seeded fault not caught"; exit 1; fi

rm -f $T/w/a.c $T/w/a.log
run $(pwd)/3ds/tools || { echo "selfapply FAIL tool exit"; exit 1; }
cmp -s $T/w/a.c $F/a.out.c || { echo "selfapply FAIL a.c differs from a.out.c"; diff $T/w/a.c $F/a.out.c | head -20; exit 1; }
cmp -s $T/w/a.log $F/a.log || { echo "selfapply FAIL a.log differs"; diff $T/w/a.log $F/a.log | head -20; exit 1; }
echo "selfapply ok"
