#!/bin/sh
# MENUFL: a menu redraw after the grey does not send the colour target to the top screen (pm_gpu.c pm_gpu_menu_present_w).
chk() { awk '/^static void pm_gpu_menu_present_w\(void\) \{/{f=1} f&&/C3D_FrameBegin/{print (s?"ok":"late"); exit} f&&/if \(menu_grey_top\) \{ gfxFlushBuffers\(\); return; \}/{s=1}' "$1"; }
mkdir -p build3ds/out/menufl_seed; grep -v "if (menu_grey_top) { gfxFlushBuffers(); return; }" 3ds/src/pm_gpu.c > build3ds/out/menufl_seed/pm_gpu.c
[ "$(chk build3ds/out/menufl_seed/pm_gpu.c)" = "late" ] || { echo "menufl seed FAIL"; exit 1; }
[ "$(chk 3ds/src/pm_gpu.c)" = "ok" ] || { echo "menufl FAIL: no skip before C3D_FrameBegin"; exit 1; }
grep -q "menu_dirty = 1; menu_grey_top = 0;" 3ds/src/pm_gpu.c || { echo "menufl FAIL: no reset at the freeze open"; exit 1; }
[ "$(grep -c "menu_grey_top = 1;" 3ds/src/pm_gpu.c)" = "1" ] || { echo "menufl FAIL: no set after the grey swap"; exit 1; }
echo "menufl ok"
