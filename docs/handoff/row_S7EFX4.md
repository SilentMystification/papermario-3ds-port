# Row S7EFX4: effect group 4 through the S7 tables (14 gfx sources; call sites use nd_rec_dl)

(Model: Haiku. One pattern over the group's files. Blocker: S7RUN merged (nd_rec_dl, nd_gtab_run, the `# S7EFX4` line in
3ds/tools/gen_gfx_tab.sh). Groups are alphabetical and disjoint, so S7EFX1 to S7EFX8 run together.)

## Group (gfx sources src/effects/gfx/<g>.c)
fright_jar gather_energy_pink gather_magic got_item_outline green_impact hieroglyphs huff_puff_breath ice_pillar ice_shard landing_dust lens_flare light_rays lightning lightning_bolt

## Code facts (oracle 5, main 3cef2c284)
- plan_nativeapi.md section 6: a source in GCONV_SRC gives one NdGTab per `Gfx` list of it; nd_host's gtab compare checks every
  table against the list words. Dynamic producers (section 6 item 7) keep their built strips; only their static lists change.
- nd_rec_dl(list) (S7RUN) records the table of list, or the old gSPDisplayList unit when the list has no table (counted `dl`).

## Start
`sh 3ds/tools/wt_new.sh S7EFX4`. PM_ROW=S7EFX4 PM_LANE=S7EFX4 on every call.

## Touch only
3ds/tools/gen_gfx_tab.sh (the `# S7EFX4` line only); the src/effects/*.c files whose `gSPDisplayList(gMainGfxPos++, X)` calls name a list of the group (for each g:
src/effects/<g>.c, plus any file that grep shows naming a `Gfx` symbol of src/effects/gfx/<g>.c); azt_scenarios.json and
bench_defs.json (one entry `fxpic_21`, right after `fxpic_84`).

## Exact edits
1. gen_gfx_tab.sh: `GCONV_SRC="$GCONV_SRC" # S7EFX4` becomes `GCONV_SRC="$GCONV_SRC fright_jar gather_energy_pink gather_magic got_item_outline green_impact hieroglyphs huff_puff_breath ice_pillar ice_shard landing_dust lens_flare light_rays lightning lightning_bolt" # S7EFX4`.
2. Build once and run nd_host (step 1 of the gate). A source the tool cannot convert (nd_host or the gen step fails on it): drop
   that one name from the line, write it in the commit, go on.
   Known drop: flame (src/effects/gfx/flame.c references D_800B32E0 and D_800B3AE0, defined in src/main.c, so the host
   link of gconv_in.o fails); no other effect gfx source references outside symbols (ORACLE6 grep).
3. In each effect file of the group: every `gSPDisplayList(gMainGfxPos++, X);` becomes, under `#ifdef TARGET_3DS`,
   `nd_rec_dl(X);` with the old line in `#else` (X unchanged: a symbol, a table element or a pointer variable). Lines inside a
   local packet (`gSPDisplayList(&g[...], ...)` for nd_parts_add) and gSPBranchList stay. Add `#include "nd_gtab.h"` at the top of
   each edited file (after its includes). Count the swapped lines per file into the commit.
   ORACLE6 rule for the special forms (code read of firework.c:255-330, nd_rec.c:566-581): a `gSPDisplayList(gMainGfxPos++, X)`
   that is already inside a `#ifdef TARGET_3DS` branch becomes `nd_rec_dl(X);` in place (no new #ifdef). This holds inside an
   nd_sub_open/nd_sub_close range too: stream_run replays the NDC_NAPI record of nd_rec_dl on each nd_sub_call
   (nd_rec.c:578, NDC_NAPI arm), as it replays the call unit today. Lines in an `#else` (N64) branch, nd_sub_call lines,
   `savedGfxPos + 1` forms and local packets stay.
4. Scenario and bench `fxpic_21`: a copy of fxpic_50 with fxtest=21 (lens_flare), same window. Same commit as step 3.

## Gate (expected values)
- `sh 3ds/build.sh > build3ds/out/build.txt 2>&1`: memcheck OK. `bench.py run nd_host --row S7EFX4 --why "S7EFX4 gtab"`:
  PASS; hand back the `gtab lists <n> parts <p> defs <d> ops <o> bad 0` line (n above main's).
- `bench.py base fxpic_21` (new bench: 2 runs) and `bench.py base spot_mac01_det`; one list `bench.py run
  fxpic_21,spot_mac01_det --row S7EFX4 --why "S7EFX4: group 4 lists from tables"` (2 starts). cmp: every shot byte-equal.
- Census (this row's edits only): the `gtabrun` line of the fxpic_21 log in the effect frames: `dl 0` (every swapped call of
  lens_flare has a table) and `tab` above 0. dl above 0: name the list (the gen symbol list) and stop.
- Budget 8 Azahar starts.

## Stop
A byte difference, a host compare `bad` above 0 or dl above 0: stop, commit, hand back (2 tries per named cause). Delete this
row file in the completing commit.
