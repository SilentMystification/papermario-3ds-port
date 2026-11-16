#!/bin/sh
# CAPTOOL: pm_logrecv diagnosis, retry and #PMLIVE (test_pm_logrecv.py), capture_check judge (test_capture_check.py),
# and the push echo prints the real 3dslink argv. Seed: the old echo line fails the argv check.
python3 3ds/tools/test_pm_logrecv.py || exit 1
python3 3ds/tools/test_capture_check.py || exit 1
chk() { grep -qF 'set -- --verbose livelog "$@"' "$1" && ! grep -qF -- '-- --verbose $*"' "$1"; }
mkdir -p build3ds/out # CAPTOOL fix: the seed file needs its directory (a missing file passed the seed check)
printf '%s\n' 'set -- --verbose livelog "$@"' 'echo "+ $LINK -a $ip $SRC3DSX -- --verbose $*"' > build3ds/out/captool_seed.sh
test -s build3ds/out/captool_seed.sh || { echo "captool FAIL: seed file not written"; exit 1; }
if chk build3ds/out/captool_seed.sh; then echo "captool seed FAIL: the old echo passes"; exit 1; fi
chk 3ds/push_3ds.sh || { echo "captool FAIL: push echo argv"; exit 1; }
grep -qF 'if line.startswith(b"#PMLIVE "):' 3ds/tools/livelog.py || { echo "captool FAIL: livelog #PMLIVE"; exit 1; }
echo "captool ok"
