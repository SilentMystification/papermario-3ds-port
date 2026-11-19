# ws_NAT1D (Haiku): count the drec draws whose walker decision differs from the last snap (c0diff), the predictor of the W2 deletions. Counter: c0gate unchanged, expected `c0gate calls 126 interp 37 state 23`. Gate: drec byte-equal.
Designer: oracle NATIVE. Branch from main (b485faae44 or later). Worktree build3ds/wt/NAT1D, branch NAT1D. PM_ROW=NAT1D PM_LANE=NAT1D. Wave W2a with NAT2D.
Edit only: 3ds/src/nd_comb.c (lines 190 and 339 area), 3ds/src/nd_set.c (line 248 area), 3ds/src/pm_gpu.c (line 598 area, c0_count line 1331, drec_end), 3ds/src/nd_snap.c (new lines after line 230 only. NAT2D edits nd_snap.c lines 67-77 and 195-205: no overlap).
Overlaps with other rows: none in this wave. NAT3A (next wave) edits nd_comb.c lines 342-361.
## Facts (main b485faae44)
- FACT c0src counts the last walker writer, not a wrong value. A W2 deletion keeps the GPU state of the last applied snap (sc_cur). So a deletion is byte-equal when every walker decision that reaches a draw equals sc_cur.
- FACT the walker decisions: sync_combine_work (nd_comb.c:331-340, its NdCombOut o), the memo replay scm_replay (:181-191, ScMemo m), nd_rnd_sync (nd_set.c:233-249, rs mixk mixc).
- FACT NdRender is 8 uint8_t and one int16_t (nd.h:337), no padding: memcmp is exact.
## Edits (new lines end with `/* NAT1D */`)
1. nd_snap.c, after line 230 (the end):
```c
/* NAT1D: 1 when a walker decision differs from the last snap core 0 took (sc_cur) */
int nd_snap_cdiff(int plan_on, int leg, const PmPlan* plan) { const NdCombOut* s = &sc_cur.o; return s->plan_on != plan_on || (plan_on ? memcmp(&s->plan, plan, sizeof *plan) != 0 : s->leg != leg); }
int nd_snap_rdiff(const NdRender* rs, int mixk, unsigned mixc) { return memcmp(&sc_cur.rs, rs, sizeof *rs) != 0 || sc_cur.mixk != mixk || sc_cur.mixc != mixc; }
```
2. nd_comb.c: a new line above `void scm_replay(const ScMemo* m) {` (line 181): `int nd_snap_cdiff(int plan_on, int leg, const PmPlan* plan); extern unsigned char nd_c0d; /* NAT1D */`.
   In scm_replay, a new line before line 190 (`    nd_c0w |= 1u; nd_c0_src(); ...`): `    nd_c0d = (unsigned char)((nd_c0d & ~1u) | (unsigned)nd_snap_cdiff(m->plan_on, m->leg, &m->plan)); /* NAT1D */`.
   In sync_combine_work, a new line before line 339 (`    nd_c0w |= 1u; nd_c0_src(); ...`): `    nd_c0d = (unsigned char)((nd_c0d & ~1u) | (unsigned)nd_snap_cdiff(o.plan_on, o.leg, &o.plan)); /* NAT1D */`.
3. nd_set.c: a new line above `void nd_rnd_sync(void) {` (line 233): `int nd_snap_rdiff(const NdRender* rs, int mixk, unsigned mixc); extern unsigned char nd_c0d; /* NAT1D */`.
   A new line before line 248 (`    nd_c0w |= 2u; nd_c0_src(); /* NAT1B */`): `    nd_c0d = (unsigned char)((nd_c0d & ~2u) | (nd_snap_rdiff(&rs, mixk, mixc) ? 2u : 0u)); /* NAT1D */`.
4. pm_gpu.c, a new line after line 602 (the second and last line of `static void c0_who(void)`, which starts at line 601):
   `unsigned char nd_c0d; static uint32_t c0_dk[8]; static unsigned c0_dkc[8], c0_dkn, c0_wd; static void c0_dwho(void) { c0_wd++; for (unsigned i = 0; i < c0_dkn; i++) if (c0_dk[i] == c0_wk) { c0_dkc[i]++; return; } if (c0_dkn < 8u) { c0_dk[c0_dkn] = c0_wk; c0_dkc[c0_dkn++] = 1u; } } /* NAT1D */`
   c0_count (line 1331): after `if (nd_c0w & 3u) c0_who();` insert ` if (nd_c0w & nd_c0d & 3u) c0_dwho(); /* NAT1D */`.
   drec_end: a new line after the c0who lines: `    pm_log_evt("c0diff f%u wdiff %u", pm_frame_count(), c0_wd); for (unsigned i = 0; i < c0_dkn; i++) pm_log_evt("c0dwho k %08x n %u", (unsigned)c0_dk[i], c0_dkc[i]); /* NAT1D */`
## Steps (output to build3ds/out/, read the last 20 lines, Bash `timeout: 600000`, 1 build, 6 Azahar starts of 8)
1. Edits 1-4. `sh 3ds/tools/nd_host_test.sh > build3ds/out/nat1d_host.txt 2>&1`: PASS (the 3 CACHECHECK lines are known). Commit `NAT1D 1: c0diff counter`.
2. For B in spot_mac01_drec, entcape_kmr03_drec, fxparts_drec: `python 3ds/tools/bench.py base <B> > build3ds/out/nat1d_base_<B>.txt 2>&1`.
3. `sh 3ds/tools/rowloop.sh --row NAT1D --bench spot_mac01_drec,entcape_kmr03_drec,fxparts_drec --no-host --why "NAT1D: c0diff counter" > build3ds/out/nat1d_loop.txt 2>&1`.
4. `grep -H "c0src f\|c0who\|c0diff\|c0dwho" build3ds/azt/logs/spot_mac01_drec.txt build3ds/azt/logs/entcape_kmr03_drec.txt build3ds/azt/logs/fxparts_drec.txt > build3ds/out/nat1d_c0src.txt`.
5. `python 3ds/tools/c0gate.py build3ds/lane-NAT1D/pm_3ds.elf > build3ds/out/nat1d_c0gate.txt 2>&1`.
## Gate (exact)
Host PASS. Rowloop exit 0 (3 drec equal). c0gate `calls 126 interp 37 state 23`. nat1d_c0src.txt has 3 c0diff lines.
Fix rights: compile errors (a missing PmPlan or NdRender type: include the header the file already uses for it). A DIFF or a tool fault: stop, send the output path to oracle NATIVE with SendMessage, and wait.
## Hand-back
Last commit `NAT1D 2: gate. 3 drec equal; c0diff lines` with `git add -f build3ds/out/nat1d_c0src.txt build3ds/out/nat1d_loop.txt`. It deletes this page. One SubagentHandback: hash, the 3 c0diff lines, the c0dwho lines.
