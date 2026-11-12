# Row S4COL: the prim colour rides in the vertices (NATIVEAPI S4 colour step: pink_sparkles, shimmer_burst)

(Model: Haiku. A new named cause after PBATCH2's two tries (CLAUDE.md 5).)
## Cause (oracle code read, main 6e171b39a; data FXFNCOV and PBFLUSH)
pink_sparkles per part: D (submit) +5306 calls, E (batch close) +1373 per frame (FXFNCOV), pm_gpu_set_plan flushes 53.8 per frame
(PBFLUSH 504363): every prim change makes a new plan. PBATCH2 (branch PBATCH2, f514533db) routes only the plan's vertex slot colour
(cur_plan.vtxname) into the ring vertices, but pm_comb.c build() never gives PRIM that slot: src_of (pm_comb.c:287-295) takes the
stage constant first (:289, :292), then the buffer, and the vertex slot only when both are taken (:294). So for (1 - PRIM) * TEXEL0 +
PRIM (pink_sparkles list D_09000F20) PRIM is a stage constant, vtxname is NONE, gv_vc_vtx stays 0 and every part flushes.

## Fix
PRIM takes the plan's vertex slot (pm_comb.c build) inside particle records only (step 2'), with PBATCH2's pieces (vertex colour,
no flush on a prim change in combine 8).

## Start
`sh 3ds/tools/wt_new.sh S4COL`. PM_ROW=S4COL PM_LANE=S4COL on every build and bench call.

## Touch only
3ds/src/pm_comb.c (build), 3ds/src/pm_gpu.c (pm_gpu_gvtx_colour new, pm_gpu_set_plan, pm_gpu_set_prim), 3ds/src/nd_geom.c (gv_tri,
nd_part_draw).

## Exact edits
1. Apply the PBATCH2 source diff: `git diff $(git merge-base main PBATCH2) PBATCH2 -- 3ds/src/nd_geom.c 3ds/src/pm_gpu.c | git apply -3`.
   A conflict: stop and hand back (do not resolve by guessing).
2. pm_comb.c build(): a static helper `reads_prim(const Seq* r, const Seq* a)` like uses_shade (:302) for TN_PR; replace line 365 by
   `if (uses_shade(&R, &A)) al.vtx = PM_NM_SHADE; else if (reads_prim(&R, &A)) al.vtx = PM_NM_PRIM;`
3. nd_geom.c nd_part_draw: in the per-triangle block, after its params (gv_params_calc) lines, add the PBATCH2 lines of gv_tri: the
   gv_vc_vtx computation with its cm/ca set, and `if (gv_vc_vtx)` the three pm_gpu_gvtx_colour calls with rs[i0], rs[i1], rs[i2] and
   cur_plan.vtx (the vertices of an instance are loaded after its prim op, so no vbase check is needed there).

## Next step (after try 1: nd_host FAIL, HEAD f4f554455; resume on branch S4COL)
Fact: the global step 2 changes the TEV template of every combine that reads PRIM and no SHADE (pause, battle and splash templates of
the ndref files: 39 FAIL lines in nd_tmpl_test). The ndref oracle words are recorded walker states of the old plan; regenerating them
to the new plan would hide any later regression of those draws, and the change is only needed for the particle instances. Decision:
limit PRIM-in-the-vertex-slot to the particle path; keep the ndref files.
2'. Replace step 2 by: pm_comb.c: `int pm_comb_vprim; /* S4COL: 1 inside a particle record (nd_parts_run): PRIM takes the vertex slot
    when the combine reads no SHADE */` (zero in the host test: the ndref templates stay); build() line 365:
    `if (uses_shade(&R, &A)) al.vtx = PM_NM_SHADE; else if (pm_comb_vprim && reads_prim(&R, &A)) al.vtx = PM_NM_PRIM;`
    nd_comb.c: add `(unsigned)pm_comb_vprim` to the parts[] of decide (:424, the plan memo key), sc_sig_now (:468) and scm_sig (:437),
    so the memos keep the two plans apart (`extern int pm_comb_vprim;`).
    nd_walk.c nd_parts_run (after S4P's lines): at its start `pm_comb_vprim = 1; sc_ok = 0;` (the first pre op resyncs with the vertex
    plan); at its end `pm_comb_vprim = 0; sc_ok = 0; sync_combine();` (the normal plan is back in gs before the next walker draw).
    Steps 1 and 3 stay (the PBATCH2 pieces in gv_tri apply only while the plan routes the vertex slot).
Touch only adds 3ds/src/nd_comb.c (decide, sc_sig_now, scm_sig) and 3ds/src/nd_walk.c (nd_parts_run).
Gates unchanged; add fxpic_50 to the byte list (shimmer_burst also takes the vertex plan): `bench.py run fxpic_124,fxpic_50 ...`
(msgbox_talk_nowide no longer needed: text and HUD keep their plan). nd_host must PASS with the ndref files unchanged.

## Gate (CLAUDE.md 2-3)
- Build, memcheck OK; nd_host PASS.
- `bench.py base fxpic_124` and `bench.py base msgbox_talk_nowide`; one list `bench.py run fxpic_124,msgbox_talk_nowide --row S4COL
  --why "S4COL: prim as the vertex colour"`; `bench.py cmp` of both against `$(git merge-base HEAD main)`. Byte-equal: pass. Not
  byte-equal: stop, commit, hand back the cmp lines (diff pixels, boxes): the user decides on a non-byte-equal colour (as SPARKLE).
- Speed gate (census): `PM_FNCOV=1 PM_ROW=S4COL PM_LANE=S4COL sh 3ds/build.sh`; `bench.py run fxfncov_base,fxfncov_124,fxfncov_50
  --row S4COL --lane S4COL_fncov --why "S4COL: per-function calls"`; `hotset.py --fncov <log> --fncovfn --lane S4COL` per log. Pass
  (pink_sparkles minus base): pm_gpu_set_plan < 20 and flush_impl_body < 20 (one plan and one flush per part before: 47 parts);
  shimmer_burst: report the same two. Budget: 6 Azahar starts.
- Console: none in this row; one XL run `fxtest=all` after the batch merge (orchestrator), compared with xl4a.

## Stop / At the end
A failed gate or an edit that does not fit: commit, hand back the output (CLAUDE.md 4-5). Delete this row file in the completing commit.
