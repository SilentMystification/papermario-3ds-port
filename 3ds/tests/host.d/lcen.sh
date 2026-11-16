#!/bin/sh
# LKEY: the list reader census (lkey_cen.py). The tree must match 3ds/tests/lkey/allow.txt. The seed (case_new: one reader line, not listed) must fail.
PY=${PY:-python3}
$PY 3ds/tools/lkey_cen.py 3ds/tests/lkey/allow.txt . || exit 1
got=$($PY 3ds/tools/lkey_cen.py 3ds/tests/lkey/case_new/allow.txt 3ds/tests/lkey/case_new | tail -1)
if [ "$got" != "lkey census FAIL" ]; then echo "lkey census seed not seen: [$got]"; exit 1; fi
echo "lkey census test ok"
