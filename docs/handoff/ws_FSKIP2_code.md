# ws_FSKIP2 code: the exact edits (verified against main a0115f743c, file:line)
Make each edit with the exact text. Change no other line. "Delete" means delete the whole line.
All line numbers are of main before any edit. Make the edits of a file from the bottom up, so the numbers stay true.
## 3ds/include/pm_fpsmode.h
E1 :1-6 (the comment block). Replace the 6 lines with:
```
/* FSKIP2 (user 2026-11-16): the Frame skip option (options.ini fps60, Options row "Frame skip", General tab row 0).
 *  0 Off: 30 FPS, no interpolation. FS1 (pm_main.c) drops the draws of a late step, so the game logic keeps 30 steps a second.
 *  1 On: every drawn game step shows the key frame and the interpolated frame. No draw is dropped: a slow step makes the game slower.
 *  2 Dynamic: 60 FPS while the walk and the GPU fit, 30 FPS (no interpolated frame) under load, with hysteresis. FS1 drops late draws.
 * The fps60 launch switch with fps60=0 acts as Dynamic. The fs1off switch turns FS1 off, fs1force=<n> forces it on. Integers only, no division. */
```
E2 after :24 (the pm_fps_iskip line) add 4 lines:
```
/* FSKIP2: the mode in force. interp_on is pm_interp_on (> 0: the interpolation runs). */
static inline int pm_fps_mode(int opt, int interp_on) { return interp_on <= 0 ? PM_FPS_OFF : opt == PM_FPS_ON ? PM_FPS_ON : PM_FPS_DYN; }
/* FSKIP2: FS1 (drop the draws of a late step) runs in Off and Dynamic, never in On. */
static inline int pm_fps_fs1(int mode) { return mode != PM_FPS_ON; }
```
## 3ds/src/pm_main.c
E3 :418 delete (`void pm_fskip_set(int on) { ... }`).
E4 :568 before `        fs1_off = pm_debug_has("fs1off") || !pm_opt.fskip;`
after `        fs1_off = pm_debug_has("fs1off"); /* FSKIP2: the mode decides FS1 at each step (pm_fps_fs1) */`
E5 :689 in the line replace the text `!fs1_off && !PMDL("fast")` with
`!fs1_off && (fs1_force || pm_fps_fs1(pm_fps_mode(pm_opt.fps60, pm_interp_on))) && !PMDL("fast")`. Keep the rest of the line.
E6 :720 in the line replace the text `pm_fps_iskip(pm_opt.fps60 == PM_FPS_ON ? PM_FPS_ON : PM_FPS_DYN, interp_heavy()` with
`pm_fps_iskip(pm_fps_mode(pm_opt.fps60, pm_interp_on), interp_heavy()`. Keep the rest of the line (it is inside `pm_interp_on > 0 ?`: same values).
## 3ds/src/pm_options.c
E7 :14 in the comment delete the text `fskip on, `.   E8 :18 delete the text `.fskip = 1, `.   E9 :97 delete (`if (!strcmp(key, "fskip")) ...`).
E10 :110 in the format string delete the text `fskip=%d\n`. :111 delete the text `pm_opt.fskip, ` (between `pm_opt.rep_rate, ` and `pm_opt.hud_ms`).
E11 before :207 (`            fld = opt_field(line);`) add:
`            if (!strcmp(line, "fskip")) continue; /* FSKIP2: the old Frame skip boolean (fskip=0|1) is ignored; the Frame skip row is fps60 */`
E12 :289 delete (`if (row == 9) return pm_opt.fskip; ...`).   E13 :312 delete (`if (row == 9) { pm_opt.fskip = ...`).
E14 :390 in dbg_lbl replace `"Fixed Seed", "Frame skip", "Position HUD"` with `"Fixed Seed", "", "Position HUD"`.
E15 :423 before `        ids[n++] = RW_DBG + 7; ids[n++] = RW_WIDE; ids[n++] = RW_FPS; /* Frame skip (option row 9), Resolution Mode, 60 FPS (MENUGEN order) */`
after `        ids[n++] = RW_FPS; ids[n++] = RW_WIDE; /* FSKIP2: Frame skip (option row 0: Off, On, Dynamic), Resolution Mode (MENUGEN order) */`
E16 :677 before `        case RW_FPS: return "60 FPS";` after `        case RW_FPS: return "Frame skip";`
## 3ds/include/pm_port.h
E17 :292 delete (`int fskip; /* FS1: ...`).   E18 :300 delete (`void pm_fskip_set(int on); ...`).
E19 :309 in the comment replace `Rows 0 to 5 are fps60 wide` with `Rows 0 to 5 are fps60 (the Frame skip row: 0 Off, 1 On, 2 Dynamic) wide`,
and delete the text `row 9 is Frame skip, `.
## 3ds/tools/pm_fpsmode_test.c
E20 after :12 (the `fskip FAIL name` line) add:
```
    if (pm_fps_mode(PM_FPS_ON, 0) != PM_FPS_OFF || pm_fps_mode(PM_FPS_OFF, 1) != PM_FPS_DYN || pm_fps_mode(PM_FPS_ON, 1) != PM_FPS_ON || pm_fps_mode(PM_FPS_DYN, 1) != PM_FPS_DYN) { printf("fskip FAIL mode\n"); return 1; }
    if (pm_fps_fs1(PM_FPS_OFF) != 1 || pm_fps_fs1(PM_FPS_ON) != 0 || pm_fps_fs1(PM_FPS_DYN) != 1) { printf("fskip FAIL fs1\n"); return 1; }
```
E21 :26 before `    printf("fskip ok 12/12\n");` after `    printf("fskip ok 14/14\n");`
## 3ds/tests/host.d/fskip.sh
E22 after :12 (the last seed line) add 2 seed lines:
```
seed 's/return mode != PM_FPS_ON;/return 1;/' 'fskip FAIL fs1'
seed 's/interp_on <= 0 ? PM_FPS_OFF/interp_on < 0 ? PM_FPS_OFF/' 'fskip FAIL mode'
```
E23 :13 before `grep -q 'pm_fps_iskip(pm_opt.fps60 == PM_FPS_ON' 3ds/src/pm_main.c || { echo "fskip FAIL: pm_main.c does not call pm_fps_iskip"; exit 1; }`
after `grep -q 'pm_fps_iskip(pm_fps_mode(pm_opt.fps60, pm_interp_on)' 3ds/src/pm_main.c || { echo "fskip FAIL: pm_main.c does not call pm_fps_iskip"; exit 1; }`
E24 after :15 add:
```
grep -q 'pm_fps_fs1(pm_fps_mode(pm_opt.fps60, pm_interp_on))' 3ds/src/pm_main.c || { echo "fskip FAIL: FS1 does not read the mode"; exit 1; }
grep -q 'case RW_FPS: return "Frame skip";' 3ds/src/pm_options.c || { echo "fskip FAIL: the row label is not Frame skip"; exit 1; }
if grep -q 'pm_opt.fskip\|pm_fskip_set\|RW_DBG + 7' 3ds/src/pm_main.c 3ds/src/pm_options.c 3ds/include/pm_port.h; then echo "fskip FAIL: the fskip boolean is still wired"; exit 1; fi
```
## Facts for the edits (grep on main)
- Readers and writers of pm_opt.fskip and pm_fskip_set: only pm_main.c:418, :568, pm_options.c:18, :97, :111, :289, :312, pm_port.h:292, :300.
- RW_DBG + 7 is used only at pm_options.c:423. pm_options_get and pm_options_flip have no caller outside pm_options.c.
- pm_interp_on is visible in pm_main.c (used at :498). The 2 new seeds change only the new lines, so each prints its own FAIL line.
