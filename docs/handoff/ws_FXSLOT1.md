# ws_FXSLOT1 (Haiku): fxtest slot switch, star shimmer test entry, capture of 5 unseen sites (C1 build -1, C2 gfxpos -1, C10 recdl +1)
Designer: oracle DESIGN3. Worktree build3ds/wt/FXSLOT1, branch FXSLOT1 from main, PM_ROW=FXSLOT1 PM_LANE=FXSLOT1.
Edit only 3ds/src/pm_fxtest.c, 3ds/src/pm_warp.c, src/status_star_shimmer.c; add 3ds/tools/azt.d/FXSLOT.json, 3ds/tools/bench.d/FXSLOT.json,
3ds/tools/fxrest.d/cap3.txt. Row FXSLOT2 (a later page) applies the entries from cap3.txt and moves recdl -5.
## Facts (code read, main debacb03fa)
- chomp_drop.c:163,:166 run only in state 1: spawn + 51..60 frames (chomp_drop.c:79-84). disable_x.c:308,:310 run only when
  unk_38 >= 10: type 0 adds 1 every 9 frames (:167), so from spawn + about 82 (d12, pm_fxtest.c:165). The slot is 44 (:205).
- star_power_shimmer_draw draws D_801097D8 (status_star_shimmer.c:189) only after add_star_power (inventory.c:2018, :925-931).
- entapply needs the plain line `nd_rec_dl(...);` with no comment (entapply.py:32). Lists with a table need entcapt (nd_walk.c:533).
## Edits
1. pm_fxtest.c: after :205 `#define SLOT 44` add `static int slot_len = SLOT; /* FXSLOT: switch fxslot=<n> (48..300) */`.
   Before :278 add `{ char v[8]; if (pm_debug_value("fxslot", v, sizeof v) && atoi(v) >= 48 && atoi(v) <= 300) slot_len = atoi(v); }`.
   :278 log argument `SLOT` -> `slot_len`. :284, :285 `SLOT` -> `slot_len`. :306 `k == 40` -> `k == slot_len - 4`.
2. pm_fxtest.c: after :29 add `static void fxt_star_shimmer(void);`. :185 add ` \` at the end, then the new last entry
   `E(star_shimmer, fxt_star_shimmer())` (index 153). After fxt_lil_oink (:187-193) add:
```c
/* FXSLOT: star power +4 segments: the status bar opens and the shimmer starts (inventory.c:925-931, :2011-2018). */
static void fxt_star_shimmer(void) {
    gPlayerData.maxStarPower = 1;
    gPlayerData.starPower = 0;
    add_star_power(SP_PER_SEG * 4);
}
```
3. status_star_shimmer.c:189 before `                    gSPDisplayList(gMainGfxPos++, NDN(D_801097D8)); // draw particle`
   after `                    nd_rec_dl(NDN(D_801097D8));` (no comment: entapply.py:32).
4. pm_warp.c:846: append `&& !pm_debug_has("warptest")` to the return expression (the switch of 69c08779cd, deleted
   unused in 3d14d6f763; row TST13 uses it). Comment :845: add `Switch warptest allows them.`
5. FXSLOT.json (azt.d and bench.d, copies of the FXDIG.json forms), 2 scenarios and 2 benches. Order: shimmer, chomp, disable_x.
   fxslot_cap: `warp=mac_01:0 pos=0:0:200 det fxtest=153,71,132 fxslot=110 loglevel=debug entcap entcapt psochk`, until
   `fxtest: done`, cap 40, deadline 35, events warp, `entcap l [0-9a-f]{8} s [0-9a-f]{8}`, done.
   fxslot_drec: `warp=mac_01:0 pos=0:0:200 fxtest=153,71,132 fxslot=110 loglevel=debug det drec=412:3:155 psochk`, until
   `fxtest: done`, cap 40, deadline 35, events warp, `drec f722 draws [1-9]\d* bytes \d+`, done. (k 12, 57, 102 of the 3 slots.)
## Loop (each output to build3ds/out/, read the last 20 lines)
1. Edits 1-5. Commit `FXSLOT1 1: fxslot switch, star_shimmer fxtest entry, shimmer nd_rec_dl; fxslot_cap, fxslot_drec`.
2. `python 3ds/tools/bench.py base fxdig_drec --row FXSLOT1 --commit $(git merge-base HEAD main)` (base run, not charged).
3. `PM_HOSTSEL=all sh 3ds/tools/rowloop.sh --row FXSLOT1 --bench fxdig_drec --why "FXSLOT1 gate" > build3ds/out/fxslot1_loop.txt 2>&1`.
4. `python 3ds/tools/bench.py run fxslot_cap --row FXSLOT1 --why "FXSLOT1 capture"`. `python 3ds/tools/entcap.py <its run.log>
   > 3ds/tools/fxrest.d/cap3.txt`. Commit cap3.txt with the `fxtest: ... slot 110 frames` line and the psochk end line in the text.
## Tests that decide (1 build, 3 Azahar starts)
- `LOOP PASS` with fxdig_drec byte-equal (the default slot 44 is unchanged).
- cap3.txt has an `l` line for each of status_star_shimmer.c:189, chomp_drop.c:163, :166, disable_x.c:308, :310.
- listcen on HEAD vs `$(git merge-base HEAD main)`: build -1, gfxpos -1, recdl +1, no other change.
## Fix rights: only the 6 files. A missing site in cap3.txt, a DIFF, a compile error or a timeout: stop, send the path to DESIGN3.
## Stop: last commit `FXSLOT1 2: gate. fxdig_drec byte-equal; cap3 sites <n>/5` deletes this page. One SubagentHandback.
