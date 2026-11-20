# ws_ALT2C (Haiku): census of the entity copy words core 0 runs, by reason (no behaviour change)
Designer: oracle NATIVE17 (plan_perf.md section 6b, ALT2). Start from main after the NC0F merge. Parallel with NC0T2 (other lines).
Worktree build3ds/wt/ALT2C, branch ALT2C. PM_ROW=ALT2C PM_LANE=ALT2C. Lines: main 0c20b5ebff; grep the quoted text.
Why Haiku: counters and one log line at fixed sites. No draw changes. The gate is byte-equal.
Objective: a drec end line `entw f<n> r<code> n<runs> w<words> ...` that splits altw `c0 e` (core 0 entity copy words) by reason. The ALT2 design uses it.
## Facts (FACT code read, main 0c20b5ebff)
- Core 0 runs entity copy words (nd_alt_cls 2) at 2 sites: ent_dl_walk (nd_walk.c:295-303, the whole G_DL of a refused entity record, after nd_ent_case
  returns 0 at nd_walk.c:186-190) and nd_ent_slice (nd_walk.c:311-319, one cut range: a segment that ent_seg_native refuses, or a state gap).
- nd_alt_run (nd_rec.c:758) adds the words of a top-level run to nd_altw[4 + nd_alt_cls]. That is the place to split them.
- Refusal returns: ent_draw nd_ent.c:401 (not a host list or no CALL), :437 (no key), :443 (k->st not ENT_OK: ENT_DYN, or ENT_REF with k->why),
  :444 (entchk), :445 (no cut table or other count); nd_ent_case :471 (LKEY end), :477-478 (no G_PM_ENT_DRAW, or ent_draw failed).
- Slice reasons: ent_seg_native nd_ent.c:312 (lighting, ent_sw 1), :313 (fog, 2), :314 (index room, 3), :317 (no gm index, 4), :324 (dlc_seg failed, 5 + dbg_seg).
  State gaps: nd_ent.c:451 `ent_gap(e->sd, ...)` and the tail `ent_gap(k->tail, ...)` (about :457).
## Edits
1. nd_rec.c next to `unsigned char nd_alt_cls;` (:757): `unsigned char nd_ent_rsn; unsigned nd_entn[64], nd_entw[64]; /* ALT2C: entity copy runs and words by reason */`.
   In nd_alt_run inside `if (!altx_d) {`: `if (nd_alt_cls == 2u) { nd_entn[nd_ent_rsn & 63u]++; nd_entw[nd_ent_rsn & 63u] += w; }`.
2. nd_ent.c (`extern unsigned char nd_ent_rsn;`), set the code just before each return or run:
   - nd_ent_case first line: `nd_ent_rsn = 19u;` (other). :471 LKEY end: 18. ent_draw :401: 1. :437: 2. :443: `k->st == ENT_DYN ? 3u : 4u + (k->why < 12u ? k->why : 0u)`.
     :444: 16. :445: 17.
   - ent_seg_native: at each `ent_sw[i]++` set `nd_ent_rsn = 32u + i` (for :324 use the same index as its ent_sw line).
   - Before each ent_gap call in the segment loop: 20 (gap before a segment), 21 (tail).
3. pm_gpu.c after the altw line (:633): one cold line built with snprintf into a 512-byte buffer: `entw f<n>` then ` r<code> n<runs> w<words>` for each code with runs > 0.
## Checklist (hw_guidelines.md 6): one byte store per entity decision; 2 adds per top-level entity run. The drec end line runs once per run.
## Tests (build3ds/out/, last 20 lines). Gate: 1 build, 2 Azahar runs. Base: `bench.py base <b> --commit $(git merge-base HEAD main)`.
1. Host `sh 3ds/tools/nd_host_test.sh` (timeout 600000) PASS. `PM_LANE=ALT2C sh 3ds/build.sh`; c0gate unchanged.
2. `bench.py run spot_mac01_drec,entcape_kmr03_drec --row ALT2C --why "ALT2C census"`; `bench.py cmp <b> --row ALT2C` byte-equal for both.
3. Check: the sum of the w values in `entw` equals `c0 ... e` of the altw line in the same log. Copy both lines into the hand-back.
Fix rights: compile errors; a host link error for the new globals (they live in nd_rec.c, which the host units link). A DIFF, a sum that differs, a crash:
stop, send the path to oracle NATIVE17.
Stop: last commit `ALT2C 2: gate` deletes this page. Hand-back: one SubagentHandback: commit hash, host, the 2 cmp lines, the entw and altw lines of both logs.
