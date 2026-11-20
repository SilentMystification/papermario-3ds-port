# ws_MCOPY1 (Sonnet): a model cache miss runs no copy range; it takes the hit state (hit_apply), the record lights and the model snap binds
Designer: oracle NATIVE16 (plan_perf.md 6a). Start after NC0C is merged (binds from snaps). Branch from main. Worktree build3ds/wt/MCOPY1, branch MCOPY1.
PM_ROW=MCOPY1 PM_LANE=MCOPY1. Lines: main 01e0780881; grep the quoted text (NC0B, NC0R edit other lines of nd_recs.c).
Why Sonnet: a census decides one branch of the switch, the miss path order must stay as rec_apply runs it, and a gate tool gets an expected-diff rule with a self-test.
Objective: rec_apply (nd_recs.c:42-50) calls no nd_mdl_alt. Proxy (Azahar): drec end line `altw ... c0 all <a> m <m>` (pm_gpu.c:632): m 0 in the tip log.
Gate: byte-equal, or expected-diff (user decision 20 Nov): a DIFF passes only when the tip log has `mmiss f<m>` with m at or before the first DIFF frame.
Console number: core 0 runs no model copy (FACT console 7569: c0 m 0 in steady windows): no steady change. MCOPY2 moves the core 1 numbers. c0gate 0 / 0 / 1.
## Step 1: census (no behavior change). nd_recs.c rec_apply, cold helpers, the log only
- Before :44 keep the pre values: nd_c0 omh (texture filter bits 12-13 masked out), prim, prim_lod, envc, comb_w0, comb_w1, tex_on, fog_fm, fog_fo; lt_n, lt[0..8];
  tile_x0, tile_y0, tile_fx, tile_fy; `pm_tex_cur_idx()`. After :44 build `NdHitD d; hit_delta(q, &d);` (declare hit_delta above rec_apply) and set bits:
  1 a scalar field differs from pre plus d (the hd_op rules :71-80, the HD_* flags of d); 2 lt_n or a light slot differs from the record lights
  (n = q[2].words.w0 ? q[2].words.w0 : 1 when q[3].words.w1 != 0 and n <= 7; slot i - 1 holds col and dir of `l[i - 1]`, slot n holds `a`, as nd_ds.c:472-479).
- After :48 set bit 4 when `nd_snap_bdiff(nd_msnap_b(), walker tiles)` is not 0, bit 8 when the 4 uv origin words moved, bit 16 when pm_tex_cur_idx() moved.
- Log `mcopy f<n> rec %08x d <mask>` for a mask not 0 (32 lines at most) and at drec end `mcopyc f<n> miss <k> b1 <> b2 <> b4 <> b8 <> b16 <>` (pm_gpu.c:632 area).
- Gate: spot_mac01_drec, entcape_kmr03_drec byte-equal. Read the counts. b1 or b2 not 0: stop, send the lines to oracle NATIVE16. Commit `MCOPY1 1: miss census`.
## Step 2: switch. Delete the census of step 1
1. nd_recs.c, above rec_apply: `static void hit_apply(const NdW2* q, const NdMEnt* e);` and `static void rec_lights(const NdW2* q)`: for the n of step 1,
   `nd_light_count(NUML(n));` then for i = 1 .. n + 1 the MOVEMEM words of nd_ds.c:477-478 (w0 the a field, w1 the light address), as nd_mvm_run
   (nd_walk.c:246): `if (lt_mem_same(w0, w1)) continue; gv_epoch++; nd_mv_mem(w0, w1);`.
2. rec_apply gets `const NdMEnt* e` (caller :188 passes mce). :44 before `gv_epoch++; nd_mdl_alt(nd_msnap_ab((const uint32_t*)q));`
   after `gv_epoch++; if (__builtin_expect(pm_drec_now(), 0)) mmiss_note(); hit_apply(q, e); rec_lights(q); nd_sr_forget();`. Delete :48.
   mmiss_note (cold): `pm_log_evt("mmiss f%u", pm_frame_count())` once per frame.
3. Only when step 1 counted b8 or b16 not 0: after nd_msnap_use (:196), for a record with texture units (`q[13].words.w0 || q->words.w1 || q[9].words.w0 ||
   q[10].words.w0`): `nd_bsrc = nd_msnap_b(); nd_tile_bind(0); nd_bsrc = NULL;`.
4. 3ds/tools/bench.py cmd_cmp (:2419): option `--expect WORD`. When drec.txt differs: the first DIFF draw seq from drec_cmp; its frame F from the row log line
   `drec frame <F> seq <S>` (pm_gpu.c:609, the last S <= seq); PASS when the row log has `WORD f<m>` with m <= F. Print
   `expected-diff PASS <bench> draw <seq> frame <F> marker f<m>`, return 2, no cmppass_put. Other shot diffs pass only with that drec PASS. Else FAIL, return 1.
   Self-test `python 3ds/tools/bench.py selftest-expect`: temp drec pair differing at draw 2, log `drec frame 200 seq 0`, `drec frame 201 seq 2`;
   marker f201 PASS, marker f202 FAIL, no marker FAIL (seeded faults). Prints 3 lines and `selftest-expect PASS`.
## Checklist (hw_guidelines.md 1, 6): the miss path does less (no copy walk of 2 ranges); rec_lights reads the record words in order; cold census and marker.
   PICA: hit_apply makes no GPU call (as the hit path); the native miss draw sets its own state; nd_sr_forget makes the next walker draw send its render state.
## Tests (build3ds/out/, last 20 lines). Step 2: 1 build, 2 Azahar runs. Budget 8 starts.
1. Host `sh 3ds/tools/nd_host_test.sh` (timeout 600000) PASS; selftest-expect PASS; c0gate 0 / 0 / 1.
2. `bench.py cmp spot_mac01_drec --row MCOPY1 --expect mmiss`, the same for entcape_kmr03_drec: return 0 or expected-diff PASS; altw `m 0` (core 0) in both tip logs.
Fix rights: compile errors; bench.py --expect code. A FAIL, a crash, census b1 or b2: stop, send the output path to oracle NATIVE16.
Stop: last commit `MCOPY1 2: gate` deletes this page. Hand-back: one SubagentHandback: hashes, host, selftest, the census counts, the 2 cmp results, altw lines.
