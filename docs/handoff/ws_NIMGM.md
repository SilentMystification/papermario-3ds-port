# ws_NIMGM (Sonnet, user approved 2026-11-16): the imgfx heap lists and the last imgfx gbi calls become native records (C1 C2)
Designer: the oracle (SendMessage). Worktree build3ds/wt/NIMGM, branch NIMGM, from main 7eb679a916 or later. PM_ROW=NIMGM PM_LANE=NIMGM.
Start now. Only src/imgfx.c and the new bench files are in scope (no overlap with GPOSREST or NTGT).
Why not Haiku: the heap anim list changes from a walker list call to a device entry, and the runner path must be proven equal to the walker path.
## Objective and counters (FACT listcen main 7eb679a916: src/imgfx.c build 8, gfxpos 8, gfxtype 4)
- Pass values: imgfx.c build 0, gfxpos 0. C10 recdl +1 (rule 3). gfxtype: write the real value.
## Design rules (line numbers: main 7eb679a916)
1. Texture calls (imgfx.c:1502, :1512, :1556, :1673, :1683, :1702). FACT: codemod keeps 4 of them with reason `comment` (a comment inside the call).
   Move each comment above its call. Then run `python 3ds/tools/codemod.py --tex src/imgfx.c` and `--state`. Each call becomes its nd_tx.h form (ns3unit proves the words).
2. The anim list (imgfx.c:1767 `gSPDisplayList(gMainGfxPos++, state->gfxBufs[gCurrentDisplayContextIndex]);`). gfxBufs[i] are heap lists of typed units
   (nd_imgfx_type, imgfx.c:1259-1261), vertex loads into vtxBufs[i]. Use the MESHST shape (src/animator.c:876-886, 3ds/include/nd_mesh.h):
   one NdNDev per buffer in ImgFXState (imgfx.c:95 area), filled where the buffers are made (imgfx.c:1249-1261): list = gfxBufs[i], u = its units, nu = units before NDO_END.
   No copy: the units already point at vtxBufs[i]. Clear the entries where gfxBufs are freed (imgfx.c:471-486, :1237-1245). The draw: `nd_rec_ndev(&state-><field>[ctx]);`.
3. The overlay strip (imgfx.c:1803 `gSPDisplayList(gMainGfxPos++, ufs->displayList);`). FACT: displayList is a moved list key (NDN): monstar.c:158,
   sam_05/npc.c:60; both lists are inherit tables (build3ds/out/ntab_inherit.txt:199, :241). Edit: `nd_rec_dl(ufs->displayList);` (nd_gtab.c:108, the table path).
4. Path equality (the core of this row). Today the walker runs a heap list through walk_call (3ds/src/nd_walk.c:225-250): ent_gdl when
   `!(nt && !nt->ntri)` (:237), then dlc_try (:243), then nd_ops_run. nd_rec_ndev runs gt_run (nd_walk.c:609-640). Set ntri and fl of the anim entry so that gt_run
   takes the same branch as walk_call took for that list. Read ent_gdl and dlc_try for a heap list and write the reason in the commit text.
   For rule 3, do the same check for walk_call with a found entry against nd_ntab_rec. If a path cannot be made equal, stop and send the facts to the oracle.
5. No division, no double, no toggle, no stub. One pm_log_evt line per run at the first anim draw (`imgfx anim f<frame>`) and the first strip draw (`imgfx strip f<frame>`) may stay (debug tool).
## Bench (new, one commit with its scenario: 3ds/tools/azt.d/NIMGM.json, 3ds/tools/bench.d/NIMGM.json; copy the EVAR files, drec=200:2)
- nimgm_strip_drec: sam_05 (Monstar overlay, GUESS). nimgm_anim_drec: a scene with an imgfx anim at frame 200 (GUESS candidates: gmerlee_drec scene dro_02, hos_05, isk_07).
- Proof of a scene: `python 3ds/tools/capture_check.py --row NIMGM --proof "imgfx strip f" ...` (and `imgfx anim f`). Take the first candidate that prints CAPTURE CHECK OK.
  No candidate in 3 tries for a bench: drop that bench, note it in the commit, and send the fact to the oracle.
## Tests that decide
1. `sh 3ds/tools/nd_host_test.sh` after the change (one time): `ND HOST TESTS PASS`, `ns3unit ok`, `lkey census ok`.
2. One build: rc 0, memcheck margin above 0. `python 3ds/tools/listcen.py > build3ds/out/nimgm_lc.txt`.
3. Gate (at most 8 Azahar starts with the scene checks, base runs not charged): spot_mac01_drec and the new benches. `bench.py base <b>`,
   `bench.py run <b> --row NIMGM --why "NIMGM gate"`, `bench.py cmp <b> --row NIMGM --commit $(git merge-base HEAD main)`. Pass: 1/1 byte-equal each.
## Loop and stop
Group 1: rule 1 (commit `NIMGM 1`). Group 2: rules 2-5 and the bench files (commit `NIMGM 2`). Then tests 1-3 (commit `NIMGM 3: gate`, deletes this page).
Fix scope: src/imgfx.c and the 2 bench files. Any other failure after 2 tries: write the output to build3ds/out/ and send the path to the oracle.
Hand-back: commit, imgfx.c listcen lines, the path reasons of rule 4, gate values, memcheck margin.
