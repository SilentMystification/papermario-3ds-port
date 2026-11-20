# ws_PERF0 (Haiku: closed edits, byte-equal): counters for the per-core split, the alt copy volume and the GPU draw words
Designer: oracle NATIVE15 (plan_perf.md). Branch from main. Worktree build3ds/wt/PERF0, branch PERF0. PM_ROW=PERF0 PM_LANE=PERF0.
Objective: measure, change no draw. Counters: no C1..C14 move (c0gate stays 0 / 0 / 1). New prof lines `c1s`, `c1a` and drec event line `altw`.
Edit only these regions (line numbers main 1472130686). Sources are CRLF: use the Edit tool or `sed -b -i`.
## Edits
1. nd_ds.c: after line 17 add `unsigned nd_altw[8]; /* PERF0: alt words. Core 1 written: 0 model, 1 entity, 2 stream call, 3 fallback. Core 0 run (top level): 4 all, 5 model, 6 entity */`.
   nd_ds.h after line 30 (`extern NdDs nd_ds;`): `extern unsigned nd_altw[8]; /* PERF0 */`.
2. nd_ds.c:485 `nd_alt_close(br, ab); }` becomes `nd_alt_close(br, ab); nd_altw[0] += (ab[1] - ab[0]) >> 2; }`.
   nd_ds.c:490 `nd_alt_close(br, ab + 2); }` becomes `nd_alt_close(br, ab + 2); nd_altw[0] += (ab[3] - ab[2]) >> 2; }`.
3. nd_rec.c (the words of a range are its end address minus its start address, divided by 4):
   :248 after `nd_alt_close(br, ab);` add `nd_altw[1] += (ab[1] - ab[0]) >> 2;`. :264 after `nd_alt_close(br, ab);` add the same. :467 after `nd_alt_call_end(blk);` add `nd_altw[2] += (blk[2] - blk[1]) >> 2;`.
   :485 and :503: append at the line end `nd_altw[3] += (ab[1] - ab[0]) >> 2; /* PERF0 */`.
   :726 before: `void nd_alt_run(const uint32_t* ab) { nd_ds_rpl++; stream_run(...); nd_ds_rpl--; } /* NAT5R */` after:
   `unsigned char nd_alt_cls; static unsigned char altx_d; /* PERF0: core 0, the caller class (1 model, 2 entity) and the nesting depth */`
   `void nd_alt_run(const uint32_t* ab) { if (!altx_d) { const unsigned w = (ab[1] - ab[0]) >> 2; nd_altw[4] += w; if (nd_alt_cls) nd_altw[4u + nd_alt_cls] += w; } altx_d++; nd_ds_rpl++; stream_run((const uint32_t*)(uintptr_t)ab[0], (const uint32_t*)(uintptr_t)ab[1]); nd_ds_rpl--; altx_d--; } /* NAT5R; PERF0 */`
4. nd_walk.c: add near line 30 `extern unsigned char nd_alt_cls; /* PERF0: nd_rec.c */`. :417 nd_mdl_alt: `nd_alt_run(ab);` becomes `nd_alt_cls = 1u; nd_alt_run(ab); nd_alt_cls = 0u;`.
   :299 nd_ent_slice and :291 ent_dl_walk: put `nd_alt_cls = 2u;` before `nd_alt_run(ab)` and before `nr_call(`, and `nd_alt_cls = 0u;` after each call.
5. pm_gpu.c:293 after `static unsigned sc_prg, sc_shr, sc_upd, sc_spl;` add `static unsigned sc_gd[2], sc_gw[2]; /* PERF0: draws and command words, 0 emitted, 1 cached block */`.
   :1361-1362 before: `GPUCMD_SetBufferOffset(coff + (ndn_bq && ... ? blk_put(cb + coff, ndn_bq, d) : nd_emit_draw(cb + coff, &ndn_sh, d))); }` after:
   `const int bq_ = ndn_bq && !d->use5 && !d->no_uni && d->npal == 1u; const unsigned nw_ = bq_ ? blk_put(cb + coff, ndn_bq, d) : nd_emit_draw(cb + coff, &ndn_sh, d);`
   `GPUCMD_SetBufferOffset(coff + nw_); sc_gd[bq_]++; sc_gw[bq_] += nw_; } /* HC6 step 4; PERF0 */`
   After :4566 (pm_gpu_cnt_now): `void pm_gpu_gw_now(unsigned o[4]) { o[0] = sc_gd[0]; o[1] = sc_gw[0]; o[2] = sc_gd[1]; o[3] = sc_gw[1]; } /* PERF0: totals */`.
   drec_end after :638: `{ extern unsigned nd_altw[8]; pm_log_evt("altw f%u c1 m %u e %u c %u o %u c0 all %u m %u e %u gd %u gw %u bd %u bw %u", pm_frame_count(), nd_altw[0], nd_altw[1], nd_altw[2], nd_altw[3], nd_altw[4], nd_altw[5], nd_altw[6], sc_gd[0], sc_gw[0], sc_gd[1], sc_gw[1]); } /* PERF0 */`.
6. pm_phase.c:484 after the line add `static unsigned long long c1s_t[2], c1s_m[2]; static unsigned aw_m[8], gw_m[4]; extern unsigned nd_altw[8]; void pm_gpu_gw_now(unsigned o[4]); /* PERF0 */`.
   :486 pf_lg_mark: append `c1s_m[0] = c1s_t[0]; c1s_m[1] = c1s_t[1]; memcpy(aw_m, nd_altw, sizeof aw_m); pm_gpu_gw_now(gw_m); /* PERF0 */` inside the body.
   :1106-1127: in each of the 3 wraps put `const unsigned c0_ = pm_pf_on ? pm_ticks() : 0u;` before the `__real_` call and
   `if (pm_pf_on) c1s_t[k] += (unsigned)(pm_ticks() - c0_); /* PERF0 */` after it (k 0 in __wrap_step_game_loop, k 1 in the two gfx wraps).
   After :695 (the c1n line, inside its block) add the 2 lines. Divide once per window (cold). `d1 = dn ? dn : 1u`, `pw1 = pf_w ? pf_w : 1u`:
   `prof f%u w%u c1s logic %u.%02u dl %u.%02u`: (c1s_t[k] - c1s_m[k]) * 100ull / PF_TPM / d1 for k 0 and 1.
   `prof f%u w%u c1a m %u e %u c %u o %u c0 %u m %u e %u gpu d %u w %u bd %u bw %u`: nd_altw[0..3] minus aw_m over d1, nd_altw[4..6] minus aw_m over pw1, the 4 pm_gpu_gw_now values minus gw_m over pw1.
## Tests (output to build3ds/out/, read the last 20 lines; 1 build, at most 2 Azahar starts)
1. `sh 3ds/tools/nd_host_test.sh > build3ds/out/perf0_host.txt` (timeout 600000): PASS. Commit `PERF0 1: counters`.
2. `PM_LANE=PERF0 sh 3ds/build.sh`. `python 3ds/tools/c0gate.py build3ds/pm_3ds.elf`: calls 0 interp 0 state 1.
3. `PM_ROW=PERF0 PM_LANE=PERF0 python 3ds/tools/bench.py base spot_mac01_drec --commit $(git merge-base HEAD main)`, the same for entcape_kmr03_drec.
   `PM_ROW=PERF0 python 3ds/tools/bench.py run spot_mac01_drec,entcape_kmr03_drec --row PERF0 --why "PERF0: counters, no draw change"`.
   `python 3ds/tools/bench.py cmp <bench> --row PERF0 --commit $(git merge-base HEAD main)` for both: every shot byte-equal.
   Pass also: each run log has one `altw` line with c1 m > 0 and c0 all > 0.
## Fix rights: the edited lines, compile errors, a host test that counts lines. Any DIFF or a missing altw line: stop, send the output path to oracle NATIVE15.
Stop: last commit `PERF0 2: gate` deletes this page. One SubagentHandback: commit hash, host, c0gate, cmp results, the 2 altw lines verbatim.
