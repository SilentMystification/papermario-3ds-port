# ws_L1Q1 (Haiku): setter unit queue in front of nd_rec_nr: game draw code and the recorder run in separate loops (plan_l1.md 0 item 1)
Designer: oracle L1 (plan_l1.md). Worktree build3ds/wt/L1Q1, branch L1Q1. PM_ROW=L1Q1 PM_LANE=L1Q1. Start: main after the L1PH0 merge.
WAIT: start only when oracle L1 releases this page on rule Q (plan_l1.md 5, from the L1PH0 console capture). No release: no start.
Objective: a setter unit (macro nd_nr, 3ds/include/nd_nr.h:11) goes into a 128-unit queue (1 KB). The recorder runs the queue in one loop when it is full or before any other recorder entry. Unit order and stream bytes stay the same.
Counters: Azahar l1q u, cut and r stay equal to the base (FACT by construction: a full-queue flush runs rec_nr0 with no rec_open, so a run still ends only at a depth-0 rec_open). Console ic1, lgms: GUESS -5000 to -15000 I misses, -1 to -3 ms per step.
Line numbers are main dfb703014f plus L1PH0. Grep each anchor again. Edit with the Edit tool (CRLF sources).
## Edits
1. nd_nr.h:11 before: `#define nd_nr(k, v) nd_rec_nr((k), (v)) /* one native record ... */`. After: `#define nd_nr(k, v) nd_rec_q((k), (v)) /* L1Q1: a setter unit into the unit queue (nd_rec.c) */`.
   nd_cmd.h after :38 add `void nd_rec_q(uint32_t k, uint32_t v); /* L1Q1 */` and `void nd_uq_sync(void); /* L1Q1: run the queued setter units now */`.
2. nd_rec.c, before rec_open (:215), add:
   `#define UQ_N 128u /* L1Q1: setter units held before the recorder runs them (1 KB) */`
   `static uint32_t uq[2u * UQ_N]; unsigned nd_uq_n, nd_uq_holes; unsigned char nd_uq_busy; static void rec_nr0(uint32_t w0, uint32_t w1); unsigned pm_frame_count(void);`
   `void nd_uq_sync(void) { if (!nd_uq_n || nd_uq_busy) return; nd_uq_busy = 1; const unsigned n = nd_uq_n; for (unsigned k = 0; k < n; k++) rec_nr0(uq[2u * k], uq[2u * k + 1u]); nd_uq_n = 0; nd_uq_busy = 0; }`
   `void nd_rec_q(uint32_t w0, uint32_t w1) { if (nd_uq_busy) { rec_nr0(w0, w1); return; } uq[2u * nd_uq_n] = w0; uq[2u * nd_uq_n + 1u] = w1; if (++nd_uq_n == UQ_N) nd_uq_sync(); }`
   `__attribute__((cold, noinline)) void nd_uq_hole(unsigned w) { static unsigned m; nd_uq_holes++; if (!(m >> w & 1u)) { m |= 1u << w; pm_log_evt("uqhole f%u w%u", pm_frame_count(), w); } }`
   rec_open, exact text after: `static void rec_open(void) { nd_uq_sync(); if (uq_run && !rnr_d) uq_cut(); task_open(); }` (the host seed reads this text).
3. Rename, nd_rec.c:611 before: `void nd_rec_nr(uint32_t w0, uint32_t w1) { if (__builtin_expect(nd_ub_on, 0)) nd_ublk_unit(w0, w1); /* GD9A */`.
   After: `static void rec_nr0(uint32_t w0, uint32_t w1) { if (__builtin_expect(nd_ub_on, 0)) nd_ublk_unit(w0, w1); /* GD9A; L1Q1: the body of nd_rec_nr */`.
   After the body end (:638) add `void nd_rec_nr(uint32_t w0, uint32_t w1) { nd_uq_sync(); rec_nr0(w0, w1); } /* L1Q1: the immediate form (recorder loops, buffered sites) */`.
4. Sync: `nd_uq_sync();` is the first statement of each function below. Example nd_rec.c:751 before: `const uint32_t* nd_rec_pos(void) { return wp; }`. After: `const uint32_t* nd_rec_pos(void) { nd_uq_sync(); return wp; } /* L1Q1 */`.
   nd_rec.c: nd_ds_unit :225, nd_rec_cgfx :587, nd_rec_pos :751. nd_snap.c: nd_snap_mesh :134, nd_snap_rect :135.
   nd_direct.c: nd_dir_rec :253, nd_dir_rec_raw :255, nd_dir_rec_alt :257, nd_dir_prim1 :261, nd_trect_rec :320. nd_ublk.c: nd_ublk_begin :98, nd_ublk_end :134.
   Reason per site (FACT code read): each one reads nd_ds or sets a flag that rec_nr0 reads, before its first rec_open.
5. Guard: `if (nd_uq_n && !nd_uq_busy) nd_uq_hole(<w>); /* L1Q1 */` is the first statement of: rec_take nd_rec.c:187 (w 1), snap_build0 nd_snap.c:50 (w 2), nd_snap_dec nd_snap.c:89 (w 3).
   nd_snap.c gets `extern unsigned nd_uq_n; extern unsigned char nd_uq_busy; void nd_uq_hole(unsigned w); /* L1Q1 */` near :19.
6. Host unit (new): 3ds/tools/nd_uq_test.c and 3ds/tests/host.d/uqunit.sh. Copy geounit.sh (FL, FP, RO, LIBS) and the harness of 3ds/tools/nd_geo_test.c (run_one, same, its stubs).
   Shapes, each run once with nd_rec_q and once with nd_rec_nr, compared by same(): q300 (300 units `ND_FS_W0(ND_FS_PRIM, 0u)`, w1 = k * 0x01010101u),
   napi (5 such units, `nd_rec_napi(t_run, r, 1u)` with a local no-op runner and r = {1, 2}, 5 units), pos (5 units, p = nd_rec_pos() kept as p - mk, 5 units).
   Then nd_uq_holes must be 0. Print `uqunit ok 3/3` or `uqunit FAIL <shape>`.
   Seed first: `sed 's/static void rec_open(void) { nd_uq_sync();/static void rec_open(void) {/' 3ds/src/nd_rec.c > /tmp/nd_rec_uqseed.c`, build with it: the output must be `uqunit FAIL q300`.
## Tests (output to build3ds/out/, read the last 20 lines). Gate: 1 build, 2 Azahar runs. Budget 8 Azahar starts.
1. `PM_LANE=L1Q1 sh 3ds/build.sh`: memcheck OK (bss +1 KB). `sh 3ds/tools/nd_host_test.sh` (timeout 600000): PASS, uqunit seed and ok lines. c0gate 0 / 0 / 0.
2. spot_mac01_drec and hudfb_drec: `bench.py base <b> --commit $(git merge-base HEAD main)`, run, cmp: byte-equal. `grep -c uqhole` = 0 in both run logs.
3. capture_check as ws_L1PH0 test 3 (proof `l1q`): the l1q line must equal the base l1q line word for word (a change means the unit order changed: stop, send it to the oracle).
Fix rights: compile errors. A host unit that does not link nd_rec_q or nd_uq_sync: add `__attribute__((weak))` forwarding or no-op definitions in that unit's test double (for example 3ds/tools/nd_direct_test.c).
A uqhole line or a DIFF: stop, send the log path and the cmp output path to the oracle. Do not add a sync site the page does not name.
Stop: the commit `L1Q1: gate` deletes this page. Hand back with ONE SubagentHandback: commit hashes, host lines, c0gate, cmp lines, uqhole counts, l1q line before and after.
