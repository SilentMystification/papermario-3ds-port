# ws_NST2D (Haiku): stream ranges feed nd_ds (unit log, sub save and restore, model R11 ranges) (stage S3; counter: dschk fbad lower)
Designer: oracle NSTO. WAIT: branch from main after the NST2C merge. Worktree build3ds/wt/NST2D, branch NST2D. PM_ROW=NST2D PM_LANE=NST2D.
Edit only: 3ds/src/nd_ds.c, 3ds/include/nd_ds.h, 3ds/src/nd_rec.c, 3ds/src/nd_walk.c (2 lines), 3ds/src/nd_dschk.c, src/model.c (2 lines),
3ds/tools/nd_ds_test.c, 3ds/tests/host.d/nst1.sh, 3ds/tools/fxrest.d/dschk_base.txt.
## Facts (code read, main 35db24d0de)
- A sub range (nd_rec.c:501-511) is skipped in place (NDC_JUMP) and runs at nd_rec_call (:479). Core 1 applies its units at record time.
  Custom gfx ranges (model.c:3535-3554) sit in one sub. The model record runs them (R11: q[22] pre, q[23] post; nd_recs.c:99, :299).
  A floor range (model.c:4158-4166) draws in place, then nd_rec_call replays it. nd_rec_list, map shape lists: no edit (NST2A).
## Edits
1. nd_ds.c above nd_ds_apply (host-built; malloc at first use, kept). First line of nd_ds_apply: `if (lg_dep && s == &nd_ds) lg_put(k, v);`
   ```
   static uint32_t* lg_ch[8]; static unsigned lg_n, lg_dep, lg_nmk, sv_n; static NdDs sv[4]; unsigned nd_ds_lg_over, nd_ds_lg_hw;
   static struct { const uint32_t* pos; unsigned i; } lg_mk[1024]; /* a chunk = 4096 units (32 KB) */ static void lg_put(uint32_t k, uint32_t v) { const unsigned c = lg_n >> 12;
       if (c >= 8u || (!lg_ch[c] && !(lg_ch[c] = (uint32_t*)malloc(4096u * 8u)))) { nd_ds_lg_over++; return; }
       lg_ch[c][2u * (lg_n & 4095u)] = k; lg_ch[c][2u * (lg_n & 4095u) + 1u] = v; if (++lg_n > nd_ds_lg_hw) nd_ds_lg_hw = lg_n; }
   void nd_ds_lg_reset(void) { lg_n = lg_dep = lg_nmk = sv_n = 0; }   void nd_ds_lg_open(void) { lg_dep++; }   void nd_ds_lg_close(void) { if (lg_dep) lg_dep--; }
   void nd_ds_mark(const uint32_t* pos) { if (lg_nmk < 1024u) { lg_mk[lg_nmk].pos = pos; lg_mk[lg_nmk++].i = lg_n; } else nd_ds_lg_over++; }
   void nd_ds_sub_save(void) { if (sv_n < 4u) sv[sv_n] = nd_ds; sv_n++; }   void nd_ds_sub_restore(void) { if (sv_n && --sv_n < 4u) nd_ds = sv[sv_n]; }
   void nd_ds_range(NdDs* s, const uint32_t* a, const uint32_t* b) { /* replay from mark a to mark b (b NULL: the next mark) */
       int ia = -1, ib = -1; if (a == b) return;
       for (int j = (int)lg_nmk - 1; j >= 0; j--) if (lg_mk[j].pos == a) { ia = j; break; }
       if (ia >= 0 && !b) ib = ia + 1 < (int)lg_nmk ? ia + 1 : -1;
       else if (ia >= 0) for (int j = (int)lg_nmk - 1; j > ia; j--) if (lg_mk[j].pos == b) { ib = j; break; }
       if (ib < 0) { nd_ds_lg_over++; return; }
       for (unsigned i = lg_mk[ia].i, e = lg_mk[ib].i; i < e; i++) nd_ds_apply(s, lg_ch[i >> 12][2u * (i & 4095u)], lg_ch[i >> 12][2u * (i & 4095u) + 1u]); }
   ```
   nd_ds_model: after the shroud/fog lines add `if (q[22]) nd_ds_range(s, (const uint32_t*)(uintptr_t)q[22], NULL);`. Last line: the same for q[23]. nd_ds.h: declare the 8 functions and the 2 counters.
2. nd_rec.c: rec_ctx_check after `sh_all();` (:171) `nd_ds_lg_reset();`. nd_rec_mark before `return wp;` and nd_rec_end at its end `nd_ds_mark(wp);`.
   nd_sub_open after `s->a = nd_rec_mark();` `nd_ds_sub_save(); nd_ds_lg_open();`. nd_sub_close at its end `nd_ds_lg_close(); nd_ds_sub_restore();`.
   nd_rec_call at its end `nd_ds_range(&nd_ds, a, b);`. stream_run (:602): `case NDC_CALL: nd_ds_rpl++; stream_run(...); nd_ds_rpl--; break;`.
3. nd_walk.c nd_range_run: `nd_ds_rpl++;` before `nd_stream_run(a);` (:690), `nd_ds_rpl--;` after it. 4. src/model.c: nd_range_begin `const uint32_t* a = nd_rec_mark(); nd_ds_lg_open(); return a;`. nd_range_repeat after `end = nd_rec_mark();`
   add `nd_ds_lg_close();`. Declare both there (`/* nd_ds.c */`).
5. nd_dschk.c: `unsigned nd_ds_rpl;`. First line of nd_dschk_run: `if (nd_ds_rpl) return;` (core 1 took that copy inside the range). nd_dschk_end adds a line `dschk lg hw <nd_ds_lg_hw> over <nd_ds_lg_over>`.
6. nd_ds_test.c (on &nd_ds): reset; mark A; open; PRIM X; mark B; close; PRIM Y; nd_ds_range(A, B): prim X. save; ENV E; restore: env back. Print `nst2d cases 2 bad <b>`. Fault: nd_ds_range(B, A).
## Steps and tests (output to build3ds/out/, read the last 20 lines; 1 build, 4 Azahar starts)
1. Edits 1-6. Host PASS with `nst2d cases 2 bad 0` and the fault seen. Commit `NST2D 1: ranges feed nd_ds`.
2. bench.py base spot_mac01_drec,wscmb_pause_drec (merge-base, not charged). rowloop --row NST2D --bench the same 2: LOOP PASS, byte-equal.
3. bench.py run the 2 dschk scenes spot_mac01_dschk and fxchk_a_dschk; grep their dschk lines > build3ds/out/nst2d_dschk.txt. Pass: each last fbad not higher than dschk_base.txt,
   the sum lower (GUESS), `over 0` in each `dschk lg` line. Write the grep into dschk_base.txt.
## Fix rights: the listed files; compile errors. A higher fbad, over above 0, a crash, a DIFF: stop, send the output path to oracle NSTO. Wait. Stop: last commit `NST2D 2: gate. 2 benches byte-equal; dschk fbad <2> (base <2>); lg hw <2>` deletes this page. One SubagentHandback.
