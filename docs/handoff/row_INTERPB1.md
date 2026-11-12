# Row INTERPB1: design B step 1: twin matrices in the t = 0.5 walk (one walk for both fps60 frames)

(Model: Sonnet (user approved 2026-11-11: cross-file matrix and GPU stream work). Chain INTERPB1 -> INTERPB2 -> INTERPB3, one coder,
one worktree, merged after each step's gates. Design: row_ORACLE_NEXT.md NATIVEAPI "Interpolation: one walk", option B.)

## Facts
INTERP2M (merged): the t = 0.5 frame L and the key frame K differ only in float uniform words (registers 0x2C0-0x2C8: U 355-454 per
300-frame window), never in buffer addresses, render targets or other state (A 0, T 0, O 0) on icmp_mac01 and icmp_btl. So K can be
built from L's command lists with K's matrix uniforms, and the second walk (pm_key_walk, pm_nusys.c:191-207) can go (step 3).

## This step: compute K's matrices during the L walk (no output change)
- load_mtx (nd_mtx_set.c:106-129): in the L pass (pm_interp_repl) nd_mtx_replace gives L; also unpack K = nd_mtx_unpack(src) (the
  cache rule of :116-126 for K). Keep a twin matrix stack in nd_mtx.c (nd_mtx_load applied to both with the same flags; pop pops both;
  gbi_popmtx and every stack op of nd_mtx.c and nd_mtx_set.c follow); upload_mvp (:38-42) makes combined (L) and combined_k.
- gv_matrix_id (nd_mtx_set.c:54-80): an entry is the pair (combined, combined_k) and (nm, nm_k); dedupe on the pair; new arrays
  gm_tab_k, gm_nrm_k (malloc beside gm_tab, gm_nrm). pcomp_calc (pm_gpu.c:814) gets the twin pcomp_k[gm] (post x gm_tab_k[gm]).
- Every other matrix-derived value a draw uploads (model native palette pal0 = post x combined, pm_gpu.c:195 nd_mtx_pcomp; sprite
  palette 0; pm5 normal rows; light directions if they use the view matrix) gets its K twin where it is computed: list each site in
  the commit (file:line) with its twin.
- Outside the L pass the twins equal the main values (copy) or are not computed (a flag), so fps60 off costs nothing.
- Debug check (switch twinchk, cold): the key walk (still running) records gm_tab entries in order; compare entry i of the key walk with
  gm_tab_k[i] of the L walk of the same step (same creation order), and pcomp, pal0, normal rows likewise; one line per 300 frames
  `twinchk f<n> entries <e> mismatch <m> pal0 <p> nrm <q>`.

## Start
`sh 3ds/tools/wt_new.sh INTERPB1`. PM_ROW=INTERPB1 PM_LANE=INTERPB1.

## Touch only
3ds/src/nd_mtx.c, 3ds/src/nd_mtx_set.c, 3ds/include/nd_int.h (twin declarations), 3ds/src/pm_gpu.c (pcomp twin and the matrix sites
of draws: list them), the twinchk check. No emit change.

## Gate (CLAUDE.md 2-3)
- Build, memcheck OK (the twin tables: report their bytes); nd_host PASS (nd_mtx_test, nd_gv_test, nd_model_test build these files).
- One list (3 Azahar starts with base): `bench.py base spot_mac01_fps60_det`; `bench.py run icmp_mac01,icmp_btl,spot_mac01_fps60_det
  --row INTERPB1 --why "twin matrices: K values from the L walk"` with switch twinchk added to the two icmp runs (a scenario copy if the
  switch cannot be added on the command line). Pass: twinchk mismatch 0, pal0 0, nrm 0 in every window after frame 300; spot_mac01_fps60_det
  12/12 byte-equal against `$(git merge-base HEAD main)`.

## Stop / At the end
A failed gate: commit, hand back the values (CLAUDE.md 5). Delete this row file in the completing commit; INTERPB2 follows.
