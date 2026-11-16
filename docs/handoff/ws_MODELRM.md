# ws_MODELRM (Sonnet, user approved): the 62 Gfx_RM* render mode lists leave src/model.c (C5 C6 C7)
Designer to ask: oracle DL2 (SendMessage; the orchestrator gives the id). Worktree build3ds/wt/MODELRM, branch MODELRM. PM_ROW=MODELRM PM_LANE=MODELRM.
Start after Merge CGFX (both edit 3ds/src/nd_rec.c). Why not Haiku: the coder designs the table form and its generator; every reader must change in one step.
Objective: no `Gfx Gfx_RM*` array and no `Gfx* ModelRenderModes[]` in the device build. FACT (listcen on main): src/model.c data 516, data_arrays 71, gfxtype 168.
GUESS: data -400 or more, data_arrays -63, gfxtype -64. The coder writes the real values.
## Facts (code read, main e91cff3feb)
- Lists: src/model.c:182 `Gfx* ModelRenderModes[]` (61 entries, ND_RM_LISTS 0x3d, nd_rec.c:142), 62 `Gfx Gfx_RM*[]` up to model.c:1249.
- Reader 1: nd_rm_build (nd_rec.c:146-162) copies the typed units of ModelRenderModes[i] without NOPs into nd_rm_tab[i] (nd_rec.c:143), on first use (nd_rec.c:223).
- Reader 2: NdSDef defs `ND_RM_DEF(n) NdSDef n##_def = { n, sizeof n / sizeof n[0] - 1 }` (model.c:1262-1263), declared by ND_RM_EXT (nd_rmdef.h:69-70),
  recorded by ND_RM_REC (nd_rmdef.h:71, nd_scomp_rec). Users: src/animator.c:62 AN_RM, src/entity_model.c:57 EM_RM, src/item_entity.c:950 and :952.
- Other readers: grep `ModelRenderModes`, `Gfx_RM[123]_`, `_def\b`, `nd_rm_tab`, `nd_rm_n` in src, include, 3ds/src, 3ds/include, 3ds/tests, 3ds/tools first.
  List each one in the commit text with its edit. A missed reader is the most common fault.
## Design rules (the coder decides the rest)
- The device data is const typed words, made at build time or by a committed generator from the current lists. No N64 Gfx type in the result.
- The generated words of each list equal what nd_rm_build makes today (NOP-free typed units and NDO_END). The NdSDef inputs stay equal for nd_scomp_analyse.
- No run-time cost on core 0. Follow docs/handoff/hw_guidelines.md. No toggle, no stub. Generated files that build.sh makes are not committed (.gitignore).
## Tests that decide
1. New host test 3ds/tests/host.d/modelrm.sh: it compiles the old lists (git show of model.c at the merge-base, or a frozen copy in 3ds/tests/modelrm/)
   and the new table, and compares each entry word for word. Print `modelrm ok <n>`. Seeded fault: change one word of one entry, the test must print FAIL.
2. `sh 3ds/tools/nd_host_test.sh`: `ND HOST TESTS PASS`, `lkey census ok`, conv2c `bad 0`. Update 3ds/tests/lkey/allow.txt for nd_rec.c only if its count changes.
3. One build: rc 0, 0 compile errors, memcheck margin above 0. listcen into build3ds/out/modelrm_lc.txt.
4. Gate (at most 6 Azahar starts, base runs not charged): spot_mac01_drec, sbk14_drec, fxr_drec, jan09_mesh_drec:
   `bench.py base <b>`, `bench.py run <b> --row MODELRM --why "MODELRM gate"`, `bench.py cmp <b> --row MODELRM --commit $(git merge-base HEAD main)`.
   Pass: 1/1 byte-equal each (jan09_mesh_drec: drecsum.py rule of its bench). A cold first fxr_drec: one warm rerun.
## Loop
Group 1: generator, table, host test (commit `MODELRM 1`). Group 2: readers and model.c (commit `MODELRM 2`). Then the build and the gate.
## Fix scope
model.c render mode lists and their readers, nd_rmdef.h, nd_rec.c (nd_rm_build and its data), the generator, the host test, .gitignore. No other file.
## Stop and hand-back
Stop: the 4 gates pass. Final commit `MODELRM 3: gate` deletes this page. Hand-back: build3ds/out/handback_MODELRM.txt (commit, gate values, listcen totals, memcheck).
Any other failure after 2 tries: write the output to build3ds/out/ and send the path to oracle DL2. Then stop.
