# ws_FXDIG (Haiku): damage_indicator two-digit quads get their captured entry (C10 recdl -2)
Designer: oracle NEXT2 (SendMessage). Worktree build3ds/wt/FXDIG, branch FXDIG from main, PM_ROW=FXDIG PM_LANE=FXDIG.
Main at release: 70dd84d8c0. Edit only src/effects/damage_indicator.c, new 3ds/tools/azt.d/FXDIG.json, new 3ds/tools/bench.d/FXDIG.json.
Shares no file with ENTLST, EFXGT, RECBUF, EFFCEN, DRECTEX, FLTGT, BLANKET, GTSMALL. Model: Haiku (one desc pair, one scenario).
## Objective
- FACT (fxrest.d/classes.txt:12-13): damage_indicator.c:286 and :288 are class `skip`. Cause (entapply.py:32 NDREC, :14): the trailing `// Gfx...`
  comment breaks the `\);\s*$` match. FACT (fxrest.d/cap.txt:144-145): each site has ONE captured tuple, the same for both:
  `fc123224 ff67ffff 1e082cf0 00504340 00200005 00000000 00000000 0000007f 00010001 00010000 00000002`.
- The edit applies the entapply rule by hand (entapply.py:15, :20, :37-41). gen_ntab_entries.py then seeds the 2 lists (src/effects scan).
- FACT (pm_fxtest.c:58, :162; index 129): `damage_indicator_12` draws the two-digit path. FACT (pm_fxtest.c:205-206, :292-306): slot 44, start 400,
  spawn at frame 402, shots at 412 and 432, `fxtest: done` at 444. No current drec bench runs fxtest 129 (fxr_drec stops at 127).
- Name note: pe_damage_indicator_288 exists (:6, used at :290). New names: pe_damage_indicator_ones, pe_damage_indicator_tens.
## Edits
1. src/effects/damage_indicator.c. After :6 add the 2 desc lines (bytes of w8, w9, w10 little endian):
   `ND_PSO_DESC(pe_damage_indicator_ones, 0xfc123224, 0xff67ffff, 0x1e082cf0, 0x00504340, 0x00200005, 0x00000000, 0x00000000, 0x0000007f, 1, 0, 1, 0, 0, 0, 1, 0, 2, 0, 0, 0);`
   and the same line with `pe_damage_indicator_tens`.
   Before (:286) `                nd_rec_dl(NDN(D_09002200_352420)); // GfxDrawOnesQuad`
   after `                nd_draw_list(NDN(D_09002200_352420), pe_damage_indicator_ones); // GfxDrawOnesQuad`.
   :288 the same with `NDN(D_090021E0_352400)`, `pe_damage_indicator_tens`, `// GfxDrawTensQuad`.
2. New 3ds/tools/azt.d/FXDIG.json: one scenario `fxdig_drec`, a copy of `fxpic_89_drec` (azt.d/CONV2D2.json) with these fields changed:
   debug `warp=mac_01:0 pos=0:0:200 fxtest=129 loglevel=debug det drec=412:3:10 psochk`, until `fxtest: done`, expected_events warp,
   `drec f432 draws [1-9]\\d* bytes \\d+` and `fxtest: done`; subject, proves and reason name FXDIG and damage_indicator_12.
3. New 3ds/tools/bench.d/FXDIG.json: one bench `fxdig_drec`, a copy of bench `fxpic_89_drec` (bench.d/CONV2D2.json), scenario `fxdig_drec`,
   until `fxtest: done`, purpose names FXDIG. Commit edits 2-3 together (CLAUDE.md 3).
## Loop (each output to build3ds/out/, read the last 20 lines)
1. Edits 1-3. `grep -c "nd_rec_dl(" src/effects/damage_indicator.c` prints 0. Commit `FXDIG 1: damage_indicator ones and tens quads entry; fxdig_drec`.
2. `python 3ds/tools/bench.py base fxdig_drec --row FXDIG` (base run, not charged).
3. `PM_HOSTSEL=all sh 3ds/tools/rowloop.sh --row FXDIG --bench fxdig_drec --why "FXDIG gate" > build3ds/out/fxdig_loop.txt 2>&1`.
## Tests that decide
- `LOOP PASS`: host PASS, build rc 0, the fxdig_drec cmp line byte-equal (base = merge-base of HEAD and main). Budget: 2 builds, 3 Azahar starts.
- `grep "psochk end" <base run.log> <row run.log>` (base run.log in build3ds/basecache/<merge-base>/fxdig_drec/): the `bad` values are equal.
- `python 3ds/tools/listcen.py | grep "^listcen total recdl"`: 2 lower than on the merge-base.
## Fix rights: only the 3 files. A base run without `fxtest: done`, a cmp DIFF, a higher `bad` or a host FAIL: stop, send the path to the oracle, wait.
## Stop and hand-back
Last commit `FXDIG 2: gate. fxdig_drec byte-equal, psochk bad equal; recdl <n>` deletes this page. One SubagentHandback: hashes, LOOP lines, recdl.
