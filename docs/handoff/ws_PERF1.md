# ws_PERF1 (Haiku: closed edits, byte-equal): snap intern. One stored snap per unique input per context; records and core 0 hold a pointer
Designer: oracle NATIVE15 (plan_perf.md F1, F8). Branch from main. Worktree build3ds/wt/PERF1, branch PERF1. PM_ROW=PERF1 PM_LANE=PERF1.
Objective: core 1 decides and stores a snap once per unique input (FACT now 123 to 171 new 760-byte snaps per step). c0gate stays 0 / 0 / 1. New log line `snapi f<n> new <a> hit <b> full <c> snn <d>` (FACT by construction new + hit = snn).
Edit only: 3ds/src/nd_snap.c, 3ds/include/nd_snap.h, 3ds/include/nd_api.h:117, 3ds/src/nd_rec.c:166-175 and :682-691, 3ds/src/pm_nusys.c:89-97, 3ds/tools/nd_direct_test.c:21. Sources are CRLF.
## Edits (line numbers main 1472130686; do step 1 first, on the unchanged file)
1. `sed -b -i '118,238s/sc_cur\./nd_ap->/g; 245,246s/sc_cur\./nd_ap->/g' 3ds/src/nd_snap.c`. Then by hand in nd_snap.c:
   :117 `const NdSnapChk* nd_ap = &sc_cur; /* PERF1: the applied snap, in the stream or the hold copy */`. :118 `= &sc_cur;` becomes `= nd_ap;`. :181 `return &sc_cur;` becomes `return nd_ap;`.
   :148 `nd_ap = (const NdSnapChk*)(uintptr_t)rec[0]; /* PERF1 */`. :154 memcmp becomes `memcmp(nd_ap, (const void*)(uintptr_t)rec[0], sizeof *nd_ap)`. :213 `nd_ap = (const NdSnapChk*)(uintptr_t)lr_b[2u + n + ND_LS_SNAP(lr_b[2u + r])];`.
   After :181 `void nd_snap_hold(void) { if (nd_ap != &sc_cur) { sc_cur = *nd_ap; nd_ap = &sc_cur; } } /* PERF1, core 0: the walked chain may be rebuilt next */`.
   nd_snap.h:97 `extern const NdSnapChk* nd_ap; void nd_snap_hold(void); void nd_snap_ctx(void); /* NWS4B; PERF1 */`. nd_direct_test.c:21 drops the second `const` (host -Werror).
2. nd_snap.c core 1. :23 `static NdSnapChk sn_last;` becomes these lines:
   `static const NdSnapChk sn_zero; static const NdSnapChk* sn_lp = &sn_zero; /* PERF1: the last snap */`
   `typedef struct { uint32_t h; int32_t m5; uint32_t vp; const NdSnapChk* p; } SiEnt; static SiEnt si_t[256]; static unsigned si_n, si_next; unsigned nd_si_new, nd_si_hit, nd_si_full;`
   `static uint32_t si_hw(uint32_t h, const void* p, unsigned n) { const uint32_t* w = (const uint32_t*)p; for (unsigned i = 0; i < n / 4u; i++) h = (h ^ w[i]) * 16777619u; return h; }`
   `static int si_eq(const NdSnapChk* a, const NdSnapChk* b) { return !memcmp(&a->in, &b->in, sizeof a->in) && !memcmp(&a->role, &b->role, 6u * 4u) && !memcmp(&a->plod, &b->plod, sizeof *a - offsetof(NdSnapChk, plod)); }`
   `static __attribute__((cold, noinline)) void si_log(void) { si_next = pm_frame_n + 100u; pm_log_evt("snapi f%u new %u hit %u full %u snn %u", pm_frame_n, nd_si_new, nd_si_hit, nd_si_full, nd_snn); }`
   `void nd_snap_ctx(void) { memset(si_t, 0, sizeof si_t); si_n = 0; sn_lp = &sn_zero; if (pm_frame_n >= si_next) si_log(); } /* PERF1: a context chain restarts */`
3. snap_build0 (:40-67) returns `const NdSnapChk*` (NULL: the memo holds) and loses rp. Keep :41-52. :53 becomes `if (sn_cmds == nd_rec_cmds() && si_eq(&r, sn_lp)) return NULL;`. Keep :54. Then:
   `const int m5 = sn_mode5; const uint32_t h = si_hw(si_hw(si_hw(2166136261u ^ (uint32_t)m5 ^ sn_vprim << 8, &r.in, sizeof r.in), &r.role, 24u), &r.plod, sizeof r - offsetof(NdSnapChk, plod));`
   `unsigned i = h & 255u, k; for (k = 0; k < 8u && si_t[i].p; k++, i = (i + 1u) & 255u) if (si_t[i].h == h && si_t[i].m5 == m5 && si_t[i].vp == sn_vprim && si_eq(&r, si_t[i].p)) {`
   `    const NdSnapChk* const q = si_t[i].p; nd_si_hit++; sn_leg = q->leg; sn_mode5 = q->mode5; sn_pat = q->pat; sn_lp = q; return q; }`
   Keep :55-63 (the decision). :64-66 become (2 lines):
   `NdSnapChk* const p = (NdSnapChk*)nd_rec_data_st(sizeof r); *p = r; nd_si_new++; sn_lp = p;` `if (k < 8u && si_n < 192u) { si_t[i].h = h; si_t[i].m5 = m5; si_t[i].vp = sn_vprim; si_t[i].p = p; si_n++; } else nd_si_full++; return p;`
   :68 `static const NdSnapChk* snap_build(const NdDs* s, int zc) { NDT_B(nt8); const NdSnapChk* const r = snap_build0(s, zc); NDT_E(nt8, NDT_SNAP); return r; }`.
   :70 snap_take_r body `const NdSnapChk* const p = snap_build(s, zc); if (!p) return; const uint32_t u[2] = { (uint32_t)(uintptr_t)p, 0u }; nd_rec_st(run, u, 1u); sn_cmds = nd_rec_cmds();`.
   :72 nd_snap_now body `(void)snap_build(&nd_ds, 0); sn_cmds = ~0u; return &sn_lp->o;`.
4. List tables. :75 drop `NdSnapChk ls_t` and `ls_h[LS_SMAX]`, add `static const NdSnapChk* ls_np;`. :90 `snap_build(&nd_ds, 0, &ls_t)` becomes `(ls_np = snap_build(&nd_ds, 0)) != NULL`. :77-83 become
   `static unsigned ls_find0(void) { for (unsigned i = 0; i < ls_sn; i++) if (ls_p[i] == ls_np) return i; if (ls_sn >= LS_SMAX) return ~0u; nd_lsb_n++; ls_p[ls_sn] = ls_np; return ls_sn++; } /* PERF1 */`
   nd_snap.h:95 ND_LS_DSB becomes `32u /* PERF1: 8 end words */`. nd_snap.c:95 `memcpy((char*)b + h, &nd_ds, sizeof nd_ds);` becomes
   `{ uint32_t* const e = (uint32_t*)((char*)b + h); e[0] = nd_ds.w.comb_w0; e[1] = nd_ds.w.comb_w1; e[2] = (uint32_t)nd_ds.comb_set; e[3] = nd_ds.w.prim; e[4] = nd_ds.w.prim_lod; e[5] = nd_ds.w.envc; e[6] = nd_ds.conv_k4; e[7] = nd_ds.w.om_kl; }`
   nd_lsr_end :199-201: `const uint32_t* const c = (const uint32_t*)(const void*)((const char*)lr_b + ND_LS_HDR(lr_b[0] + lr_b[1]));` `ls_gpu(c[0], c[1], (int)c[2], c[3], c[4], c[5], c[6]); nd_c0.om_kl = c[7]; nd_c0.prim = c[3]; nd_c0.envc = c[5]; nd_c0.comb_w0 = c[0]; nd_c0.comb_w1 = c[1]; nd_c0.conv_k4 = c[6];`
   Delete nd_lsc_end (:97) and `void nd_lsc_end(void);` in nd_api.h:117. Leave nd_snap.c:239-244 unchanged (ws_PERF3 deletes their last callers).
5. nd_rec.c: above :167 `void nd_snap_ctx(void); /* PERF1 */`. :175 `nd_ds_lg_reset();` becomes `nd_ds_lg_reset(); nd_snap_ctx(); /* PERF1 */`. After :683 `__attribute__((weak)) void nd_snap_ctx(void) {} /* PERF1 */`.
   pm_nusys.c: a prototype `void nd_snap_hold(void);` above stream_walk, and `nd_snap_hold(); /* PERF1 */` after `nd_stream_run(s);` (:93).
## Tests (output to build3ds/out/, read the last 20 lines; 1 build, at most 2 Azahar starts)
1. `sh 3ds/tools/nd_host_test.sh > build3ds/out/perf1_host.txt` (timeout 600000): PASS. Commit `PERF1 1: snap intern`.
2. `PM_LANE=PERF1 sh 3ds/build.sh`; `python 3ds/tools/c0gate.py build3ds/pm_3ds.elf`: 0 / 0 / 1. Base, run, cmp as ws_PERF0 step 3, benches spot_mac01_drec,fxparts_drec. Pass: every shot byte-equal; the last `snapi` line of each log has hit > 0 and new + hit = snn.
Fix rights: the edited lines, compile errors. A DIFF, new + hit not equal to snn, or a crash: stop, send the output path to oracle NATIVE15.
Stop: last commit `PERF1 2: gate` deletes this page. One SubagentHandback: commit hash, host, c0gate, cmp results, the last snapi line of each run.
