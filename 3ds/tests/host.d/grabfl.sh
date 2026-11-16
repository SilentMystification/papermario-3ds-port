#!/bin/sh
# GRABFL: each command list split in pm_gpu.c has a ring flush in its function. The seed (the grab_ring_flush call removed) must fail at grab_impl.
# Work files: build3ds/out (MENUFL moved the seed path off /tmp, mechanical). The awk strips CR: a checkout with autocrlf has CRLF lines.
mkdir -p build3ds/out/grabfl_seed
cat > build3ds/out/grabfl.awk <<'AWK'
{ sub(/\r$/, "") }
/^[A-Za-z_].*\) *\{ *(\/\*.*)?$/ { fn = $0; gsub(/__attribute__\(\([^)]*\)\)/, "", fn); sub(/\(.*/, "", fn); n = split(fn, a, /[ *]+/); fn = a[n]; fl = 0 }
/gpu_flush_written\(\)|sr_split\(\)|grab_ring_flush\(\)/ { fl = 1 }
/C3D_FrameSplit\(GX_CMDLIST_FLUSH\)/ { s++; if (!fl) { print "grabfl FAIL " fn; bad = 1 } }
END { if (!bad) print "grabfl ok splits " s; exit bad }
AWK
sed '/grab_ring_flush();/d' 3ds/src/pm_gpu.c > build3ds/out/grabfl_seed/pm_gpu.c
got=$(awk -f build3ds/out/grabfl.awk build3ds/out/grabfl_seed/pm_gpu.c || true)
if [ "$got" != "grabfl FAIL grab_impl" ]; then echo "grabfl seed FAIL: got [$got]"; exit 1; fi
awk -f build3ds/out/grabfl.awk 3ds/src/pm_gpu.c || exit 1
