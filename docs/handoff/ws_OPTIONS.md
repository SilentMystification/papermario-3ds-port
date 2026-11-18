# ws_OPTIONS (Haiku): Select menu rows that change nothing: hide, grey, label, and the Speed x2 fix (user report; no C1-C12 counter)
Designer: oracle (SendMessage). Worktree build3ds/wt/OPTIONS, branch OPTIONS from main. PM_ROW=OPTIONS PM_LANE=OPTIONS. Waits for nothing.
Edit only: 3ds/src/pm_options.c, 3ds/src/pm_dbg_game.c, new 3ds/src/pm_speedx2.h, new 3ds/tools/pm_speedx2_test.c, new 3ds/tests/host.d/speedx2.sh.
## Row map (FACT, code read of main; writer -> reader). These rows work: Frame skip :312 -> pm_main.c:418, :689. Resolution :303-308 ->
pm_gpu.c:230, :1131. 60 FPS :317 -> pm_interp.c:84, pm_main.c:720. D-PAD :82 -> pm_input.c:249. Circle pad dead zone, Repeat delay, rate :800 ->
pm_input.c:189, pm_options.c:66 (pm_options_ui.c:117, :124). Map A B X Y L R START D-pad :883 -> pm_input.c:252-256. Fog :332 -> nd_set.c:194.
Light :331 -> nd_geom.c:175. Texture filtering :467 -> pm_tex.c:1882. START+SELECT :785 -> pm_main.c:209. Cheats -> pm_dbg_game.c:310-313.
## The rows with no reader at their use (FACT) and the edit for each
- F1 C-Stick dead zone: its reader runs only with pm_new3ds (pm_input.c:218). F2 map rows ZL, ZR and the 4 C-Stick rows: keys of in_key
  (pm_input.c:206-207). An O3DS has none, and 3ds/src has no irrstInit (grep). On the user's O3DS XL these 7 rows change nothing.
1. pm_options.c:426-427 (tab_ids, tab 1). Before: `ids[n++] = RW_DPAD; ids[n++] = RW_DZPAD; ids[n++] = RW_DZCS; ids[n++] = RW_RDELAY; ids[n++] = RW_RRATE;`
   `for (i = 0; i < PM_NIN; i++) ids[n++] = RW_MAP + i;` After: `if (pm_new3ds) ids[n++] = RW_DZCS;` in place of `ids[n++] = RW_DZCS;` and
   `for (i = 0; i < PM_NIN; i++) if (pm_new3ds || !(i == PM_IN_ZL || i == PM_IN_ZR || i >= PM_IN_CSUP)) ids[n++] = RW_MAP + i; /* O3DS: no ZL, ZR, C-Stick */`
- F3 Stats and Position HUD (Debug HUD folder rows 1 and 2): pm_gpu_hud draws them only inside `if (pm_opt.dbg_hud)` (pm_main.c:844-848).
2. pm_options.c:508. After: `int pm_optui_grey(int tab, int row) { return tab == PM_OPT_SUB && ((sub_kind == 14 && row >= 1 && row <= 3 && !pm_opt.log)`
   `|| (sub_kind == 7 && (row == 1 || row == 2) && !pm_opt.dbg_hud)); } /* Stats, Position HUD draw only with HUD on (pm_main.c:844) */`
- F4 Fixed Seed: pm_os.c:55 reads pm_opt.fixedseed once, at the first start. The row changes nothing in this session.
3. pm_options.c: after :24 add `static int seed_boot; /* Fixed Seed of this run (pm_os.c:55 reads it once) */`. After :254 add
   `seed_boot = pm_opt.fixedseed;`. Before :735 (`if (id >= RW_DBG) return pm_options_get(`) add
   `if (id == RW_DBG + 6 && pm_opt.fixedseed != seed_boot) return pm_opt.fixedseed ? "On at restart" : "Off at restart";`
- F5 Speed x2 (Cheats, Position): pm_dbg_game.c:314 doubles walkSpeed and runSpeed at every step. The only other writer is reset_player_status
  (src/7B440.c:98-99, grep `walkSpeed\s*=`). So the speed compounds while the cheat is on. The comment at :314 is false.
4. New 3ds/src/pm_speedx2.h (guard PM_SPEEDX2_H):
   `typedef struct { float walk, run; int on; } PmSpd;`
   `static inline void pm_speedx2(PmSpd* s, int want, float* walk, float* run) {`
   `    if (want && !s->on) { s->walk = *walk; s->run = *run; s->on = 1; }`
   `    if (want) { *walk = s->walk * 2.0f; *run = s->run * 2.0f; } else if (s->on) { *walk = s->walk; *run = s->run; s->on = 0; } }`
   pm_dbg_game.c: after :11 add `#include "pm_speedx2.h"`. Replace :314 with
   `    { static PmSpd spd; pm_speedx2(&spd, (cheat_bits & CHB_SPEED) != 0, &ps->walkSpeed, &ps->runSpeed); } /* SPEEDX2: base x2, no compound */`
5. pm_speedx2_test.c: base 4.0/8.0; 10 calls with want 1: walk 8.0, run 16.0; 1 call with want 0: 4.0, 8.0. Print `speedx2 ok` or
   `speedx2 FAIL on walk got <v>` / `speedx2 FAIL off walk got <v>`. speedx2.sh (form of host.d/startlr.sh): seed
   `sed 's/\*walk = s->walk \* 2.0f/*walk = *walk * 2.0f/'` must print `speedx2 FAIL on walk got 4096.000000`; grep pm_dbg_game.c for
   `pm_speedx2(&spd,`; then the clean build with `-m32 -Wall -Wextra -I 3ds/src` passes.
## Steps (output to build3ds/out/, read the last 20 lines). Budget: 0 Azahar runs, 1 build.
1. Edits 1-5. `sh 3ds/tools/nd_host_test.sh > build3ds/out/options_host.txt 2>&1`: PASS, speedx2.sh passes, its seed seen.
2. `sh 3ds/build.sh > build3ds/out/options_build.txt 2>&1`: rc 0, no new warning in pm_options.c or pm_dbg_game.c.
## Fix rights: compile errors in the listed files. A host failure other than a typo, a tool fault: stop, send the path to the oracle. Wait.
## Stop: commit `OPTIONS 1: O3DS rows hidden, HUD rows grey, Fixed Seed restart label, Speed x2 no compound; host speedx2 ok` deletes this
page. One SubagentHandback: hash, host and build lines. The user checks the menu on the console.
