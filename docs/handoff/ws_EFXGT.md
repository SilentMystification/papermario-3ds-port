# ws_EFXGT (Haiku): the last Gfx tokens of src/effects (not flame.c) leave (C7 gfxtype -36)
Designer: oracle NEXT1 (SendMessage). Worktree build3ds/wt/EFXGT, branch EFXGT from main, PM_ROW=EFXGT PM_LANE=EFXGT.
Main at release: bf73af2c9a. Shares no file with FLTGT (flame.c stays), ENTLST, RECBUF. Model: Haiku (local edits, same code).
## Objective
- FACT (listcen on 66cd65f3cd): src/effects gfxtype 39, of it flame.c 3 (FLTGT). The 36 others are dead locals, byte counts written as
  `n * sizeof(Gfx)`, and handle types. FACT (include/PR/gbi.h:1062-1092): sizeof(Vtx) is 16, sizeof(Gfx) is 8.
- Every edit gives the same code: a dead local, or the same constant, or a pointer type change. Move: gfxtype -36 (416 -> 380 without FLTGT).
## Edits (match the text, not the line number: a deleted line moves the lines below it)
A. Delete the dead local line (FACT grep: no other use in the file). 18 lines:
   `Gfx* savedGfxPos;` in chomp_drop.c:148, confetti.c:267, firework.c:178, hieroglyphs.c:124, shimmer_burst.c:213, shimmer_wave.c:197, stop_watch.c:190.
   `Gfx* savedGfxPos;` and `Gfx* savedGfxPos2;` in green_impact.c:153-154, red_impact.c:167-168, shockwave.c:282-283, thunderbolt_ring.c:108-109.
   whirlwind.c:159 `Gfx* triangleDisplayList;`, :164 `Gfx* whirlwindMainDisplayList;`, :165 `Gfx* savedPos;`.
B. Byte counts. Before (stop_watch.c:207) `nd_rec_data(64 * sizeof(Gfx))` after `nd_rec_data(32 * sizeof(Vtx))`. The others:
   lightning_bolt.c:257 `0x30 * sizeof(Gfx)` -> `24 * sizeof(Vtx)`; water_block.c:244 `80 * sizeof(Gfx)` -> `40 * sizeof(Vtx)`;
   whirlwind.c:189 `64 * sizeof(Gfx)` -> `32 * sizeof(Vtx)`.
   effect_65.c:10 `#define VTX_BUF_SIZE    2 * MAX_POINTS * sizeof(Vtx) / sizeof(Gfx)` -> `#define VTX_BUF_SIZE    (2 * MAX_POINTS * sizeof(Vtx))`;
   effect_65.c:269 `nd_rec_data(VTX_BUF_SIZE * sizeof(Gfx))` -> `nd_rec_data(VTX_BUF_SIZE)` (old value 2*30*16/8*8 = 960, new 960).
C. squirt.c:167 and waterfall.c:131 `Gfx* savedGfxPos;` -> `Vtx* savedGfxPos;`.
   squirt.c:190 and waterfall.c:147 `savedGfxPos = (Gfx*) nd_rec_data(48 * sizeof(Gfx));` -> `savedGfxPos = (Vtx*) nd_rec_data(24 * sizeof(Vtx));`.
D. Handle types (the file includes nd_gtab.h, FACT grep): `Gfx*` -> `NdLst` on firework.c:15 `Gfx* D_E0086AA0[] = {`,
   sparkles.c:19 `Gfx* D_E0022CD0[] = {`, firework_rocket.c:20 `Gfx* D_E010ACC0[] = {`, chapter_change.c:9 `/* 0x00 */ Gfx* displayList;`,
   shape_spell.c:13 `/* 0x04 */ Gfx* unk_04;`, shape_spell.c:14 `/* 0x08 */ Gfx* unk_08;`.
   Before `/* 0x00 */ Gfx* displayList;` after `/* 0x00 */ NdLst displayList;`.
Edit no other line. src/effects/flame.c and src/effects/gfx/ stay.
## Loop (each output to build3ds/out/, read the last 20 lines)
1. Edit A-D. `grep -rnw Gfx src/effects --include=*.c | grep -v "src/effects/gfx/\|flame.c\|energy_orb_wave.c:244"` prints nothing.
2. Commit `EFXGT 1: dead Gfx locals, Vtx byte counts, NdLst handles in src/effects`.
3. `python 3ds/tools/bench.py base fxr_drec --row EFXGT`, then the same for fxlt_drec (base runs, not charged).
4. `sh 3ds/tools/rowloop.sh --row EFXGT --bench fxr_drec,fxlt_drec --why "EFXGT gate" > build3ds/out/efxgt_loop.txt 2>&1`.
## Tests that decide
- `LOOP PASS`: host PASS, build rc 0, both cmp lines byte-equal (base = merge-base of HEAD and main). Budget: 2 builds, 4 Azahar starts.
- `python 3ds/tools/listcen.py | grep "^listcen total gfxtype"`: 380 (FLTGT not merged) or 376 (FLTGT merged). Other ENTLST/RECBUF merges
  lower it by their own moves.
## What you may edit to fix a fail
Only the 21 files above. A compile error or a cmp DIFF: stop. Send build3ds/out/loop_EFXGT_build.txt or loop_EFXGT_cmp_<bench>.txt to the oracle.
## Stop and hand-back
Last commit `EFXGT 2: gate. fxr_drec, fxlt_drec byte-equal; gfxtype <n>` deletes this page.
One SubagentHandback: commit hashes, LOOP lines, listcen gfxtype.
