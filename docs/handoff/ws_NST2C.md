# ws_NST2C (Haiku): entity, sprite, compiled def and particle records feed nd_ds (stage S3; counter: dschk fbad lower than base)
Designer: oracle NSTO. WAIT: branch from main after the NST2B merge. Worktree build3ds/wt/NST2C, branch NST2C. PM_ROW=NST2C PM_LANE=NST2C.
Edit only: 3ds/src/nd_ds.c, 3ds/include/nd_ds.h, 3ds/src/nd_rec.c, 3ds/src/nd_scomp.c (1 line), src/sprite.c (spr_state_feed), 3ds/tools/nd_ds_test.c,
3ds/tests/host.d/nst1.sh, 3ds/tools/fxrest.d/dschk_base.txt.
## Facts (code read, main c8b0159131)
- nd_rec_ent_draw (nd_rec.c:241) records the list call in the payload, not through nd_rec_nr. Core 0 runs the list state (ent_run gaps or walk_call).
- spr_state_run (src/sprite.c:278) makes its units on core 0 (spr_flat_gfx into buf, nd_nr_exec). Core 1 never sees them.
- nd_scomp_rec (nd_scomp.c:152-161) records a compiled def as one NAPI record (:160). Core 0 runs its units with the values (sc_go :336).
- nd_parts_end (nd_rec.c:320-333) records the instances of nd_parts_add. Core 0 runs pre units, the list, post units (nd_walk.c:463-468). An NDG_F_PARTS list holds VTX and TRI only (nd_gtab.h:44): no state.
- nd_rec_spr holds no state op (nd_walk_spr nd_walk.c:165). dschk copies nd_ds at rec_take: feed core 1 after each record.
## Edits
1. nd_rec.c: rename `static void ds_unit` (NST2B) to `void nd_ds_unit` and update its 2 callers. Add
   `void nd_ds_feed(const uint32_t* u, unsigned n) { for (unsigned k = 0; k < n; k++) if (u[2u * k] != ND_NOP_A) nd_ds_unit(&nd_ds, u[2u * k], u[2u * k + 1u]); }` nd_ds.h: declare nd_ds_unit, nd_ds_feed, nd_ds_parts, nd_ds_scomp.
2. nd_ds.c (host-built):
   ```
   void nd_ds_parts(NdDs* s, const uint32_t* u, NdDsUnitFn unit) { /* nd_parts_run order: pre units, post units (the list has no state) */
       const unsigned n = u[0] & 0xffu, pre = (u[0] >> 8) & 0xffu, m = (u[0] >> 16) & 0xffu;
       const uint32_t* p = u + 2; for (unsigned k = 0; k < n; k++, p += 2u * m) for (unsigned j = 0; j < m; j++) if (j != pre) unit(s, p[2u * j], p[2u * j + 1u]);
   }
   void nd_ds_scomp(NdDs* s, const uint32_t* du, unsigned n, uint32_t vmask, const uint32_t* v, unsigned nv, NdDsUnitFn unit) { /* rec_ops order */
       unsigned j = 0; for (unsigned k = 0; k < n; k++) { uint32_t w0 = du[2u * k], w1 = du[2u * k + 1u];
           if (vmask >> k & 1u) { if (j >= nv) continue; w0 = v[2u * j]; w1 = v[2u * j + 1u]; j++; }
           if (w0 != ND_NOP_A) unit(s, w0, w1); }
   }
   ```
3. nd_rec.c: in nd_rec_ent_draw after `memcpy(p + 2, r, sizeof r);` (:252) add `nd_ds_list(&nd_ds, list, 0);`. In nd_parts_end after nd_rec_napi
   (:331), before `b->n = 0;`, add `nd_ds_parts(&nd_ds, b->u, nd_ds_unit);`.
4. nd_scomp.c nd_scomp_rec: after nd_rec_napi (:160) add `nd_ds_scomp(&nd_ds, (const uint32_t*)d->u, d->n, d->vmask, v, nv, nd_ds_unit);`
5. src/sprite.c: declare `void nd_ds_feed(const uint32_t* u, unsigned n); /* nd_rec.c */`. Add `static void spr_state_feed(const uint32_t* u)`:
   the body of spr_state_run (:278-284) with a local `uint32_t buf[128]` and `nd_ds_feed(buf, (unsigned)(nrp - buf) / 2u)` for nd_nr_exec.
   Before (:313) `nd_rec_napi(spr_state_run, st, 4u);` After `nd_rec_napi(spr_state_run, st, 4u); spr_state_feed(st);`
6. nd_ds_test.c: (a) a parts record of 2 instances {PRIM, CALL, ENV} (pre 1, m 3): prim and envc of instance 2. (b) a def {PRIM value slot,
   ENV} with vmask 1 and one value PRIM: prim from the value. Print `nst2c fields <n> bad <b>`. Fault: skip the post units in (a).
## Steps and tests (output to build3ds/out/, read the last 20 lines; 1 build, 4 Azahar starts)
1. Edits 1-6. Host PASS with `nst2c fields <n> bad 0` and the fault seen. Commit `NST2C 1: entity, sprite state, compiled def and particle records feed nd_ds`.
2. bench.py base spot_mac01_drec,wscmb_pause_drec (merge-base, not charged). rowloop --row NST2C --bench the same 2: LOOP PASS, byte-equal.
3. bench.py run the 2 dschk scenes spot_mac01_dschk and fxchk_a_dschk. Grep their dschk lines > build3ds/out/nst2c_dschk.txt. Pass: in each scene the last
   `dschk f`/`dschk end` fbad is not higher than dschk_base.txt of the merge-base, and the sum of the 2 is lower (GUESS). Write the grep into dschk_base.txt.
## Fix rights: the listed files; fix compile errors. A higher fbad, a crash, a DIFF: stop, send the output path to oracle NSTO. Wait. Stop: last
  commit `NST2C 2: gate. 2 benches byte-equal; dschk fbad <2> (base <2>)` deletes this page. One SubagentHandback.
