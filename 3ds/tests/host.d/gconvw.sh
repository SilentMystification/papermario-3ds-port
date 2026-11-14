#!/bin/sh
# GCONVW: the own-TU split of gen_gfx_syms.py. The fixture (3ds/tests/gconvw/w.c, namespace tw) gives gconv_tu0.c; it must equal
# exp_tu0.c (written by hand). The seeded fault (DEF without the N(x) form) must differ. The TU object needs the gconv_sym section.
F="-DF3DEX_GBI_2 -D_LANGUAGE_C -DTARGET_3DS -DVERSION_US -DVERSION=us -D_MIPS_SZLONG=32 -Dnullptr=NULL"
INC="-I 3ds/tools/gconv_inc -I 3ds/tools -I 3ds/include -I 3ds/stub_include -I include -I src -I assets/us -I ver/us/build/include -I ."
rm -rf /tmp/gcw && mkdir -p /tmp/gcw/f
python3 3ds/tools/gen_gfx_syms.py /tmp/gcw/gconv_in.c 3ds/tests/gconvw/w.c > /tmp/gcw/tus.txt || { echo "gconvw FAIL gen"; exit 1; }
[ "$(cat /tmp/gcw/tus.txt)" = "/tmp/gcw/gconv_tu0.c" ] || { echo "gconvw FAIL tu list"; exit 1; }
cmp /tmp/gcw/gconv_tu0.c 3ds/tests/gconvw/exp_tu0.c || { echo "gconvw FAIL tu0 differs"; exit 1; }
gcc -m32 -w -ffunction-sections -fdata-sections $F $INC -c -o /tmp/gcw/gconv_tu0.o /tmp/gcw/gconv_tu0.c || { echo "gconvw FAIL compile"; exit 1; }
readelf -S /tmp/gcw/gconv_tu0.o | grep -q "gconv_sym" || { echo "gconvw FAIL no gconv_sym section"; exit 1; }
# seeded fault: a copy of the tool whose DEF lacks the N(x) form (the empty alternative keeps the group count) must differ
python3 - <<'PY' || { echo "gconvw FAIL seeded fault not applied"; exit 1; }
t = open("3ds/tools/gen_gfx_syms.py").read()
old = r"(?:N\((\w+)\)|(\w+))\s*\[[^\]]*\]\s*="
assert t.count(old) == 1
open("/tmp/gcw/gen_fault.py", "w").write(t.replace(old, r"(?:()|(\w+))\s*\[[^\]]*\]\s*="))
PY
python3 /tmp/gcw/gen_fault.py /tmp/gcw/f/gconv_in.c 3ds/tests/gconvw/w.c > /dev/null 2>&1
if [ ! -f /tmp/gcw/f/gconv_tu0.c ] || cmp -s /tmp/gcw/f/gconv_tu0.c 3ds/tests/gconvw/exp_tu0.c; then echo "gconvw FAIL seeded fault not seen"; exit 1; fi
echo "gconvw ok"
