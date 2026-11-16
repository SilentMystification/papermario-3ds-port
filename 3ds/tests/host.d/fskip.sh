#!/bin/sh
# FSKIP: host test of pm_fpsmode.h (60 FPS modes Off, On, Dynamic and the Dynamic rule). Each seeded fault must print its exact line.
mkdir -p /tmp/fskip_seed
seed() {
    sed "$1" 3ds/include/pm_fpsmode.h > /tmp/fskip_seed/pm_fpsmode.h
    gcc -m32 -Wall -I /tmp/fskip_seed -o /tmp/fskip_seed/t 3ds/tools/pm_fpsmode_test.c || exit 1
    got=$(/tmp/fskip_seed/t)
    if [ "$got" != "$2" ]; then echo "fskip seed FAIL: [$1] got [$got]"; exit 1; fi
}
seed 's/mode == PM_FPS_ON ? 0 : (late || heavy)/(late || heavy)/' 'fskip FAIL iskip on'
seed 's/#define PM_DYN_W_LO 27000u/#define PM_DYN_W_LO 20000u/' 'fskip FAIL calm o3ds'
seed 's/else d->calm = 0;/else (void)0;/' 'fskip FAIL hysteresis'
grep -q 'pm_fps_iskip(pm_opt.fps60 == PM_FPS_ON' 3ds/src/pm_main.c || { echo "fskip FAIL: pm_main.c does not call pm_fps_iskip"; exit 1; }
grep -q 'pm_fps_dyn_step(&dyn, w, g)' 3ds/src/pm_main.c || { echo "fskip FAIL: interp_heavy does not call pm_fps_dyn_step"; exit 1; }
grep -q 'case RW_FPS: return pm_fps_name(pm_opt.fps60);' 3ds/src/pm_options.c || { echo "fskip FAIL: the 60 FPS row does not show the mode"; exit 1; }
gcc -m32 -Wall -Wextra -I 3ds/include -o /tmp/fskip_t 3ds/tools/pm_fpsmode_test.c && /tmp/fskip_t || exit 1
