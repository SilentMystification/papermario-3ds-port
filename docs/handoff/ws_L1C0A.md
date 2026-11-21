# ws_L1C0A (Haiku): model hit run census and the cut points of the core 0 hit queue (design: oracle L1C0)
Worktree build3ds/wt/L1C0A, branch L1C0A. PM_ROW=L1C0A PM_LANE=L1C0A. Start: main 475dff963b or later. Line numbers are main 475dff963b: grep each anchor again. Edit with the Edit tool (CRLF sources).
Objective: count the model hits that a queue can batch, and prove that the cut list is complete. No draw changes. Counters: none move (debug lines). Endgame C1-C12: none. The hand-back decides ws_L1C0B (its rule Q0). Terms: a run = the hits with draws between two cuts. A cut = a point where the L1C0B queue must drain (reason 0-6). bad = command words written inside a run (a missed cut).
## Steps (one commit per step, in this order)
1. `L1C0A 1: cut hooks`. 3ds/include/nd.h after the `}` of nd_kp_vec (:296) add 2 lines:
   `extern unsigned pm_mq_run; void pm_mq_cut_do(unsigned r); /* L1C0A: hits with draws in the open run; the cut (pm_gpu.c) */`
   `static inline void pm_mq_cut(unsigned r) { if (__builtin_expect(pm_mq_run != 0u, 0)) pm_mq_cut_do(r); } /* L1C0A: r 0 stream command, 1 range end, 2 miss, 3 post range, 4 texture load, 5 template rebuild, 6 viewport or projection */`
   nd.h after :105 add `int nd_tex_have(const NdTexDesc* d); /* L1C0A: the table lookup of nd_tex_get, no load; -1: not in the table */`. nd.h after :366 add `int nd_tex_unit_fast(int id, int* hs, unsigned* hg, NdUnit* u); int nd_tex_unit_slow(int id, int* hs, unsigned* hg, NdUnit* u); /* L1C0A: the 2 parts of nd_tex_unit_hint */`.
   3ds/src/nd_rec.c after :868 add `unsigned pm_mq_run __attribute__((weak)); __attribute__((weak)) void pm_mq_cut_do(unsigned r) { (void)r; } /* L1C0A: strong in pm_gpu.c */`.
   Edit shape "a cut before a statement" (stream_run). Before :881 `        if (p == stop) return;` After `        if (p == stop) { pm_mq_cut(1u); return; } /* L1C0A */`.
   After :882 `const uint32_t h = p[0];` add `        if ((h >> 24) != NDC_MODEL && (h >> 24) != NDC_JUMP) pm_mq_cut(0u); /* L1C0A: any command but a model record ends the run */`.
   3ds/src/nd_recs.c: before :188 `if (tried || !mce) pm_mdl_miss_n++;` add `    pm_mq_cut(2u); /* L1C0A: a miss */`. Before :262 `if (q[12].words.w0 == 0 && PMDL("waterlog")` add the same line.
   nd_recs.c :298 (rec_run) Before `    if (q[11].words.w1) rec_range((const uint32_t*)(uintptr_t)q[11].words.w1, 2u); /* S3G-1 ...` After
   `    if (q[11].words.w1) { pm_mq_cut(3u); rec_range((const uint32_t*)(uintptr_t)q[11].words.w1, 2u); } /* S3G-1: custom gfx post (D10a: a stream range) NATRS; L1C0A */`.
   3ds/src/nd_mtx_set.c nd_xf_load: after :132 `unsigned flags = (w0 & 0xff) ^ G_MTX_PUSH;` add `    if (flags & G_MTX_PROJECTION) pm_mq_cut(6u); /* L1C0A: eyd_sync reads the projection */`.
2. `L1C0A 2: texture split`. 3ds/src/nd_tex.c. After nd_tex_get (:108) add `int nd_tex_have(const NdTexDesc* in) { ... }`: the body is :83-:91 of nd_tex_get, with `if (s == 0) return -1;` in place of `if (s == 0) break;`.
   Edit shape "split a function, no statement changes". Before: `int nd_tex_unit_hint(int id, int* hs, unsigned* hg, NdUnit* u) { [A :115-:117] [B :118-:124] [C :125-:143] }`. After, 3 functions:
   `int nd_tex_unit_fast(int id, int* hs, unsigned* hg, NdUnit* u) { [A] [B] return 0; } /* L1C0A: the hinted slot, no search, no load */`
   `int nd_tex_unit_slow(int id, int* hs, unsigned* hg, NdUnit* u) { [A] [C] } /* L1C0A: the slot search and the load */`
   `int nd_tex_unit_hint(int id, int* hs, unsigned* hg, NdUnit* u) { return nd_tex_unit_fast(id, hs, hg, u) || nd_tex_unit_slow(id, hs, hg, u); }`
3. `L1C0A 3: census`. 3ds/src/pm_gpu.c. Before :657 `static __attribute__((cold, noinline)) void drec_begin(void) {` add:
   `unsigned pm_mq_run; static unsigned mq_b[5], mq_why[8], mq_hits, mq_draws, mq_bad; static u32* mq_cb; static u32 mq_off; /* L1C0A: ws_L1C0A; mq_b hits by run length 1, 2-3, 4-7, 8-15, 16+ */`
   `static int mq_moved(void) { u32 *cb, sz, off; GPUCMD_GetBuffer(&cb, &sz, &off); return cb != mq_cb || off != mq_off; } /* L1C0A: words since the last hit */ static void mq_mark(void) { u32 sz; GPUCMD_GetBuffer(&mq_cb, &sz, &mq_off); }`
   `void pm_mq_cut_do(unsigned r) { if (mq_moved()) mq_bad++; const unsigned b = 31u - (unsigned)__builtin_clz(pm_mq_run); mq_b[b < 4u ? b : 4u] += pm_mq_run; mq_why[r & 7u]++; pm_mq_run = 0; }`
   :678 after `drec_seq = drec_over = 0;` add `memset(mq_b, 0, sizeof mq_b); memset(mq_why, 0, sizeof mq_why); mq_hits = mq_draws = mq_bad = 0; /* L1C0A */`.
   After :690 (the c0src line) add `pm_log_evt("mqr f%u hits %u draws %u r %u %u %u %u %u", pm_frame_count(), mq_hits, mq_draws, mq_b[0], mq_b[1], mq_b[2], mq_b[3], mq_b[4]); /* L1C0A */` and
   `pm_log_evt("mqw f%u bad %u why %u %u %u %u %u %u %u", pm_frame_count(), mq_bad, mq_why[0], mq_why[1], mq_why[2], mq_why[3], mq_why[4], mq_why[5], mq_why[6]); /* L1C0A */`.
   pm_gpu_set_mvp: before :3464 `if (nb.draw_3d && nb.gni > 0) flush();` add `    pm_mq_cut(6u); /* L1C0A: a viewport or ortho change */`.
   pm_gpu_model_hit :4370 Before `int id = ds ? nd_tex_get(ds) : -1;` After `int id = ds ? nd_tex_have(ds) : -1;` and the new line `if (ds && id < 0) { pm_mq_cut(4u); id = nd_tex_get(ds); } /* L1C0A: a table miss loads */`.
   :4372 Before `if (id < 0 || !nd_tex_unit_hint(id, &e->hs, &e->hg, &u)) { ndn_texf++; return 0; }` After 3 lines:
   `int tok = id >= 0 && nd_tex_unit_fast(id, &e->hs, &e->hg, &u);` / `if (id >= 0 && !tok) { pm_mq_cut(4u); tok = nd_tex_unit_slow(id, &e->hs, &e->hg, &u); } /* L1C0A: the search may load */` / `if (!tok) { ndn_texf++; return 0; }`.
   After :4382 `if (e->tgen != nd_tmpl_gen()) {` add `        pm_mq_cut(5u); /* L1C0A: an eviction */`. After :4397 (`const uint32_t shc = ...`) add `    if (pm_mq_run && mq_moved()) mq_bad++; /* L1C0A */`.
   Before :4408 `ndn_rec++; ndn_draws += e->n; ndn_hit++;` add `    mq_mark(); pm_mq_run++; mq_hits++; mq_draws += e->n; /* L1C0A */`.
## Tests (output to build3ds/out/, read the last 20 lines). Budget 8 Azahar starts (this page uses 5).
1. Every commit: `PM_LANE=L1C0A sh 3ds/build.sh` (memcheck OK), `sh 3ds/tools/nd_host_test.sh` (timeout 600000) PASS, `python 3ds/tools/c0gate.py <elf>` 0 / 0 / 0.
2. Gate: `bench.py base spot_mac01_drec --commit $(git merge-base HEAD main)`, `bench.py run spot_mac01_drec --row L1C0A --why "L1C0A gate"`, `bench.py cmp spot_mac01_drec --row L1C0A`: byte-equal. The log has `mqr f201` and `mqw f201 bad 0`.
3. Data: `bench.py run entcape_kmr03_drec --row L1C0A --why "L1C0A census kmr_03"` (no cmp). Pass: `mqw f201 bad 0`.
4. Seeded fault (self-test of bad): delete the step 1 `pm_mq_cut(0u)` line, `PM_ROW=L1C0A_seed PM_LANE=L1C0A_seed sh 3ds/build.sh`, `bench.py run spot_mac01_drec --row L1C0A_seed --why "L1C0A seeded fault"` (same env). Pass: mqw bad > 0. Then `git checkout 3ds/src/nd_rec.c`. No commit of the seed.
Fix rights: compile errors, -Werror warnings, a log line over 16 words (split it), a mechanical tool fault. A DIFF in test 2, bad > 0 in test 2 or 3, or bad 0 in test 4: stop, send the log path to the oracle (SendMessage).
Stop: the commit `L1C0A: gate` deletes this page. Hand-back with ONE SubagentHandback: commit hashes, host, c0gate, cmp line, the mqr and mqw lines of tests 2, 3 and 4.
