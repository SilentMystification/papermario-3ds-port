# ws_PRA31 (Haiku): the symbol map takes Matrix4s objects; the 34 pra_31 stairs skeleton includes move to the twin (C3 data, C4 data_arrays)
Designer to ask: oracle DL3 (SendMessage; the orchestrator gives the id). Worktree build3ds/wt/PRA31, branch PRA31. PM_ROW=PRA31 PM_LANE=PRA31.
Start now. Parallel with GSETR, CGFX, DLSITE1, MODELRM, GBIHDR, HOLD1 (no shared file). The A(x) row edits gen_gfx_syms.py later: it starts after Merge PRA31.
Cause (FACT, code read): 18 pra_31 lists are unsupported (build3ds/out/ntab_unsup.txt on main: 14 `unit address outside the symbol map`,
unk_02..30; 4 `matrix address outside the symbol map`, unk_31..34; nd_gfx_conv.c:439 and :493). Their matrices are
`Matrix4s N(unk_NN_mtx)` (src/world/area_pra/pra_31/stairs_vtx.c:120, 14 objects), and the symbol scan reads only `Mtx` (3ds/tools/gen_gfx_syms.py:16).
FACT: no other source defines a Matrix4s object (grep `^Matrix4s` in src: 14 lines, all in stairs_vtx.c). Matrix4s is 0x40 bytes (common_structs.h:136-139).
Objective (FACT, oracle gset dry run on a copy of main a0115f743c with Edit 2 and the 36 names added to gset_ok.txt): `gset moved 36 files 2 keep 0`;
listcen data -311, data_arrays -36, gfxtype -2. GUESS: the converter marks all 36 ok after Edit 1 (34 unk lists, texture_2_gfx, texture_3_gfx).
Draw path (FACT): a static animator node draws node->displayList by nd_rec_ent_draw (src/animator.c:877); an NDN key there is the precedent of
src/entity/model/BellbellPlant_anim.c:12. Files: 3ds/tools/gen_gfx_syms.py, src/world/area_pra/pra_31/stairs_skele.c, new 3ds/tools/gset.d/PRA31.txt,
new 3ds/tools/azt.d/PRA31.json and 3ds/tools/bench.d/PRA31.json, and the files gset.py writes (pra_31 sources, pra_31.h, twins). No other file.
## Edit 1: 3ds/tools/gen_gfx_syms.py:16
Before: `MTX = re.compile(r"^Mtx\s+(?:N\((\w+)\)|(\w+))\s*(\[[^\]]*\])?\s*=", re.M)`
After: `MTX = re.compile(r"^(?:Mtx|Matrix4s)\s+(?:N\((\w+)\)|(\w+))\s*(\[[^\]]*\])?\s*=", re.M)  # PRA31: a Matrix4s object has the 0x40 bytes of an Mtx`
In the docstring (line 7) change `and \`Mtx NAME =\`` to `and \`Mtx NAME =\` or \`Matrix4s NAME =\``.
## Edit 2: src/world/area_pra/pra_31/stairs_skele.c, 16 lines (43 53 64 81 93 105 117 129 141 153 165 177 189 201 213 225)
Before (:43): `    .displayList = &N(unk_29_gfx),`. After: `    .displayList = N(unk_29_gfx),`. Check: `grep -c "displayList = N(" <file>` prints 16.
After gset (dry run FACT): `    .displayList = NDN(N(unk_29_gfx)),` and each `#include "world/area_pra/pra_31/unk_NN.gfx.inc.c"` becomes NDN_DECL lines.
## Edit 3: new 3ds/tools/gset.d/PRA31.txt (3 lines, LF)
`src/world/area_pra/pra_31/stairs_skele.c` `src/world/area_pra/pra_31/texture_2.c` `src/world/area_pra/pra_31/texture_3.c` (one path per line).
## Edit 4: bench files (copy the shape of azt.d/MESH1.json and bench.d/MESH1.json)
Scenario and bench pra31_drec: `warp=pra_31:0 loglevel=debug det drec=200:2`, first_marker `warp: pra_31 entry 0`,
until `drec f201 draws [1-9]\d* bytes \d+`, cap 40, deadline 35. GUESS: the stairs draw in frames 200-201 (puzzle.c:548 loads the animator at setup).
## Loop (1 build, at most 4 Azahar starts; base runs not charged)
1. Edit 4. Commit `PRA31 1: pra31_drec bench`. `bench.py base pra31_drec` and `bench.py base spot_mac01_drec`. Pass: the base log has the until line.
2. Edit 1. `sh 3ds/tools/nd_host_test.sh > build3ds/out/pra31_host1.txt 2>&1`. Pass: `ND HOST TESTS PASS`, conv2c `bad 0`,
   `grep -c "^pra_31_" build3ds/out/ntab_unsup.txt` prints 0 (main: 18), `grep -c "^pra_31_" build3ds/out/gset_ok.txt` prints 35 to 37 (main: 1).
   Commit `PRA31 2: the symbol map takes Matrix4s objects` with both counts.
3. `python 3ds/tools/listcen.py > build3ds/out/pra31_lc0.txt`. Edit 2 and Edit 3. Then
   `python 3ds/tools/gset.py 3ds/tools/gset.d/PRA31.txt build3ds/out/gset_ok.txt > build3ds/out/pra31_gset.txt 2>&1`.
   Pass: `keep 0`, `gset ok`, moved 34 to 36 (36 when the gset_ok count of step 2 is 37). Another result: stop and send the path to oracle DL3.
4. Host test again (> pra31_host2.txt). Pass: `ND HOST TESTS PASS`, `lkey census ok`, `gconv gset ok` with no `not moved` line, conv2c `bad 0`.
   Commit `PRA31 3: gset moves the stairs lists`. One build: rc 0, 0 compile errors, memcheck margin above 0. listcen > pra31_lc1.txt.
   Pass vs pra31_lc0.txt (moved 36): data -311, data_arrays -36, gfxtype -2. Moved 34: data -290, data_arrays -34, gfxtype 0.
5. Gate: pra31_drec and spot_mac01_drec: `bench.py run <b> --row PRA31 --why "PRA31 gate"`, `bench.py cmp <b> --row PRA31 --commit $(git merge-base HEAD main)`.
   Pass: 1/1 byte-equal each.
Fix scope: Edits 1-4 only. A compile error in stairs_skele.c: check the `NDN(N(x))` form of that line. A `not moved` line or a byte diff: stop, send the path.
Stop: both gates pass. Final commit `PRA31 4: gate` deletes this page. Hand-back: build3ds/out/handback_PRA31.txt (commit, gate values,
unsup and ok counts, gset moved, listcen data data_arrays gfxtype, memcheck). Other failure after 2 tries: send the output path to oracle DL3. Then stop.
