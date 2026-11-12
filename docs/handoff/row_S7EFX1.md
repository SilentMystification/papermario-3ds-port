# Row S7EFX1: effect group 1 through the S7 tables (14 gfx sources; call sites use nd_rec_dl)

(Model: Haiku. One pattern over the group's files. Blocker: S7RUN merged (nd_rec_dl, nd_gtab_run, the `# S7EFX1` line in
3ds/tools/gen_gfx_tab.sh). Groups are alphabetical and disjoint, so S7EFX1 to S7EFX8 run together.)

## Group (gfx sources src/effects/gfx/<g>.c)
aura balloon big_smoke_puff big_snowflakes blast breaking_junk bulb_glow butterflies chapter_change chomp_drop cloud_puff_trail cold_breath confetti damage_indicator

## Code facts (oracle 5, main 3cef2c284)
- plan_nativeapi.md section 6: a source in GCONV_SRC gives one NdGTab per `Gfx` list of it; nd_host's gtab compare checks every
  table against the list words. Dynamic producers (section 6 item 7) keep their built strips; only their static lists change.
- nd_rec_dl(list) (S7RUN) records the table of list, or the old gSPDisplayList unit when the list has no table (counted `dl`).

## Start
`sh 3ds/tools/wt_new.sh S7EFX1`. PM_ROW=S7EFX1 PM_LANE=S7EFX1 on every call.

## Touch only
3ds/tools/gen_gfx_tab.sh (the `# S7EFX1` line only); the src/effects/*.c files that call lists of the group (find them: for each
g, `grep -ho "^Gfx [A-Za-z0-9_]*" src/effects/gfx/<g>.c assets/us/effects/gfx/*.inc.c` is not needed: grep the effect file
src/effects/<g>.c and any file that includes the group's gfx headers for `gSPDisplayList(gMainGfxPos++,`); azt_scenarios.json and
bench_defs.json (one entry `fxpic_0`, right after `fxpic_55`).

## Exact edits
1. gen_gfx_tab.sh: `GCONV_SRC="$GCONV_SRC" # S7EFX1` becomes `GCONV_SRC="$GCONV_SRC aura balloon big_smoke_puff big_snowflakes blast breaking_junk bulb_glow butterflies chapter_change chomp_drop cloud_puff_trail cold_breath confetti damage_indicator" # S7EFX1`.
2. Build once and run nd_host (step 1 of the gate). A source the tool cannot convert (nd_host or the gen step fails on it): drop
   that one name from the line, write it in the commit, go on.
3. In each effect file of the group: every `gSPDisplayList(gMainGfxPos++, X);` becomes, under `#ifdef TARGET_3DS`,
   `nd_rec_dl(X);` with the old line in `#else` (X unchanged: a symbol, a table element or a pointer variable). Lines inside a
   local packet (`gSPDisplayList(&g[...], ...)` for nd_parts_add) and gSPBranchList stay. Add `#include "nd_gtab.h"` at the top of
   each edited file (after its includes). Count the swapped lines per file into the commit.
4. Scenario and bench `fxpic_0`: a copy of fxpic_50 with fxtest=0 (big_smoke_puff), same window. Same commit as step 3.

## Gate (expected values)
- `sh 3ds/build.sh > build3ds/out/build.txt 2>&1`: memcheck OK. `bench.py run nd_host --row S7EFX1 --why "S7EFX1 gtab"`:
  PASS; hand back the `gtab lists <n> parts <p> defs <d> ops <o> bad 0` line (n above main's).
- `bench.py base fxpic_0` (new bench: 2 runs) and `bench.py base spot_mac01_det`; one list `bench.py run
  fxpic_0,spot_mac01_det --row S7EFX1 --why "S7EFX1: group 1 lists from tables"` (2 starts). cmp: every shot byte-equal.
- Census (this row's edits only): the `gtabrun` line of the fxpic_0 log in the effect frames: `dl 0` (every swapped call of
  big_smoke_puff has a table) and `tab` above 0. dl above 0: name the list (the gen symbol list) and stop.
- Budget 8 Azahar starts.

## Stop
A byte difference, a host compare `bad` above 0 or dl above 0: stop, commit, hand back (2 tries per named cause). Delete this
row file in the completing commit.
