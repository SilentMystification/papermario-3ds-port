# ws_FXOPS (Haiku): RARE, FILL, SCISSOR, RECT become units; 7 effect lists move (C5 data -81, C6 data_arrays -7, gfxtype -7)
Designer: oracle EFFGFX_A (agent a35af1a838a1cc25c, SendMessage). Worktree build3ds/wt/FXOPS, branch FXOPS. PM_ROW=FXOPS PM_LANE=FXOPS.
Start now. Shared line: 3ds/tools/nd_gfx_conv.c:509. A page that edits that line and merges first makes the patch reject: then stop and send it to the oracle.
## Objective and counters
- FACT (code read): v2_walk refuses RARE, FILL, SCISSOR, RECT (3ds/tools/nd_gfx_conv.c:509-511). The device runs only the unit copy of a
  moved list (nd_ntab_run, 3ds/src/nd_walk.c:600). nd_walk_state runs these units (nd_set.c:169-174, nd_walk.c:337-340).
- FACT (code read): flame is not in 3ds/tools/gconv_src.d, so its 2 asset lists have no table. Its render target D_800B32E0 is in src/main.c:15.
- FACT (oracle dry run, main 023db9ca81 + patch): listcen data 1207 -> 1126 (-81), data_arrays 92 -> 85 (-7), gfxtype 580 -> 573 (-7).
  build, gfxpos, recdl, reader, model: 0. Host: `ND HOST TESTS PASS`, `ntab moved 1898` (main 1891), conv2c `bad 0`, `lkey census ok`.
- Lists that move: embers D_09000280, radial_shimmer D_090038B8, star_spirits_energy D_09005440, underwater D_09000528 and D_09000570,
  flame D_090008F8 and D_09000918. New tables that do not move: kmr_20_records_screen_gfx, mgm_00_Gfx_RecordDisplay_Init, NoControllerGfx, hos_05_gfx_setup_story_viewport.
## Files (all in docs/handoff/ws_FXOPS.patch, 21 files). Edit only these.
3ds/tools/nd_gfx_conv.c, 3ds/tools/nd_gtab_test.c, new 3ds/tools/gconv_src.d/FXOPS.txt, new 3ds/tools/gset.d/FXOPS.txt,
3ds/tools/gfxin/src/effects/gfx/{embers,radial_shimmer,star_spirits_energy,underwater}.c, new 3ds/tools/gfxin/src/effects/gfx/flame.c,
src/effects/{embers,flame,radial_shimmer,star_spirits_energy,underwater}.c, src/effects/gfx/ (the same 5 names), new 3ds/tools/azt.d/FXOPS.json, new 3ds/tools/bench.d/FXOPS.json.
## Edit shape (the patch does it; use it to check a reject)
Before (3ds/tools/nd_gfx_conv.c:509): `case NDO_FILL: case NDO_PRIMZ: case NDO_SCISSOR: case NDO_RARE: case NDO_RECT: case NDO_MISC: v2_unsup(...)`.
After: RARE writes s->conv_k4, s->key_c, s->key_s (the set_rare rule, nd_set.c:148-156). FILL, SCISSOR, RECT: `break;` (unit only). PRIMZ, MISC: v2_unsup.
nd_gtab_test.c m_unit gets the same RARE case on ms. Before (src/effects/embers.c:13): `{ NDN(D_090003B0_3DC2F0), D_09000280_3DC1C0 }`.
After: `{ NDN(D_090003B0_3DC2F0), NDN(D_09000280_3DC1C0) }`.
## Loop (1 build, 2 Azahar starts; base runs not charged)
1. `python 3ds/tools/listcen.py > build3ds/out/fxops_lc0.txt`.
2. `tr -d '\r' < docs/handoff/ws_FXOPS.patch | git apply -`. A reject: stop, send the output to the oracle.
3. `sh 3ds/tools/nd_host_test.sh > build3ds/out/fxops_host.txt 2>&1`. Read the last 20 lines.
   Pass: `ND HOST TESTS PASS`, `ntab moved 1898`, conv2c `ntab tables .. bad 0`, `lkey census ok`, no line `gconv: moved`.
4. `python 3ds/tools/listcen.py > build3ds/out/fxops_lc1.txt`. Pass vs fxops_lc0.txt: total data -81, data_arrays -7, gfxtype -7,
   no other total line changes. Commit the 21 files: `FXOPS 1: RARE, FILL, SCISSOR, RECT units in v2 tables; 7 effect lists move; fxops_drec bench`.
5. One build. Pass: rc 0, 0 compile errors, memcheck margin above 0.
6. `python 3ds/tools/bench.py base fxops_drec` and `python 3ds/tools/bench.py base curt_title_drec`. Pass: each base log has its drec line.
7. Gate, for b in fxops_drec curt_title_drec: `python 3ds/tools/bench.py run $b --row FXOPS --why "FXOPS gate"`, then
   `python 3ds/tools/bench.py cmp $b --row FXOPS --commit $(git merge-base HEAD main)`. Pass: 1/1 byte-equal each.
   Coverage: FACT fxtest 0-based index (3ds/src/pm_fxtest.c FXLIST): 27 flame, 45 radial_shimmer, 87 underwater, 105 embers, 123 star_spirits_energy.
   FACT embers.c:213 and underwater.c:220 draw their lists. FACT radial_shimmer type 2 draws D_E0066C8C[2], not D_090038B8 (radial_shimmer.c:314).
   GUESS: star_spirits_energy type 2 draws D_09005440 (star_spirits_energy.c:400, needs unk_70 1, 5 or 100). GUESS: curt_title_drec draws NoControllerGfx.
## Fail
- A failing test or a compile error: edit only the 21 files. Compare each with the patch. 2 tries for one cause. Then write the output to build3ds/out/ and send the path to the oracle.
- A base run that fails or hangs, or a byte diff in a gate: stop. Send the output path to the oracle.
## Stop: both gates pass. The last commit `FXOPS 2: gate` deletes ws_FXOPS.md and ws_FXOPS.patch.
## Hand-back (one SubagentHandback message)
Commits. listcen data, data_arrays, gfxtype before and after. `ntab moved` line. memcheck margin. The 2 cmp lines.
