#!/bin/sh
# DECDUMP: decode_dump.py --crash names the pc, lr and stack words of the build #5774 block of crash_a.txt through a2l (a stub stands for arm-none-eabi-addr2line). Seeded fault (DECDUMP_SEED=1): the build # regexp is changed, so the block has no build number and the compare must FAIL.
D=/tmp/decdump_t
PY=${PY:-python3}
rm -rf $D; mkdir -p $D/bin
cp 3ds/tests/decdump/a2l_stub.sh $D/bin/arm-none-eabi-addr2line && chmod +x $D/bin/arm-none-eabi-addr2line
tool=3ds/tools/decode_dump.py
if [ "${DECDUMP_SEED:-0}" = 1 ]; then
  $PY -I - "$D" <<'PYEOF'
import sys
s = open("3ds/tools/decode_dump.py").read()
t = s.replace('m = re.search(r"build #(\\d+)", head)', 'm = re.search(r"build #x(\\d+)", head)')
assert t != s, "seed not applied"
open(sys.argv[1] + "/dd.py", "w").write(t)
PYEOF
  tool=$D/dd.py
fi
PATH=$D/bin:$PATH $PY -I $tool --crash 3ds/tests/decdump/crash_a.txt --elf 3ds/tests/decdump/pm_3ds_5774.elf > $D/out.txt 2>&1
if ! diff --strip-trailing-cr 3ds/tests/decdump/expect_a.txt $D/out.txt > $D/diff.txt; then echo "decdump FAIL build"; head -20 $D/diff.txt; exit 1; fi
[ "${DECDUMP_SEED:-0}" = 1 ] && { echo "decdump seed not seen"; exit 1; }
head -1 $D/out.txt | grep -q "build #5774$" || { echo "decdump FAIL last block"; exit 1; }
PATH=$D/bin:$PATH $PY -I $tool --crash 3ds/tests/decdump/crash_a.txt --build 2050 --elf 3ds/tests/decdump/pm_3ds_5774.elf 2>&1 | head -1 | grep -q "build #2050$" || { echo "decdump FAIL --build"; exit 1; }
echo "decdump ok"
