# ws_NC0A (Haiku: closed edits, byte-equal): the sprite known bits come from core 1; nd_c0 loses om_kh, om_kl, omode_l, fill, use_t1
Designer: oracle NATIVE15 (plan_perf.md section 7). Branch from main. Worktree build3ds/wt/NC0A, branch NC0A. PM_ROW=NC0A PM_LANE=NC0A.
Facts (main 0b1ba0f322):
- The only reader of nd_c0.om_kh and nd_c0.om_kl that changes a draw is the sprite build (pm_gpu.c:4582 known_h, known_l). nd_core.c:18 is a debug log.
- Core 1 holds the same bits: NdDs.w.om_kh, om_kl (ds_om sets them for every OMH, OML unit; nd_snap.c:101-102 for the direct pso snap).
- nd_c0.omode_l, fill and use_t1 have no reader after DUPMS (grep `nd_c0.omode_l`, `nd_c0.fill`, `nd_c0.use_t1`: writers only).
Objective: core 0 keeps 5 fewer state fields and stops its known-bit bookkeeping. c0gate stays 0 / 0 / 1 (nd_c0 still exists).
Proxy: the size of the nd_c0 symbol (`arm-none-eabi-nm -S build3ds/pm_3ds.elf | grep -w nd_c0`) falls by 20 bytes against the base build.
Edit only (line numbers main 0b1ba0f322): nd_snap.h:15-26, nd_apw.h:5, nd_snap.c:59, :129; pm_gpu.c:4582; nd.h:167, :170, :172, :180; nd_state.c:11;
nd_direct.c:57-58; nd_recs.c:148-149; nd_rect.c:323; nd_set.c:99, :103, :105, :107; nd_snap.c:213; nd_walk.c:209, :234, :260, :264; nd_core.c:18; 3ds/tools/nd_direct_test.c:24-27, :147, :158. Sources are CRLF.
## Edits
1. nd_snap.h NdSnapChk: after `NdSnapBind b[2];` add `uint32_t kh, kl; /* NC0A: core 1 om_kh, om_kl (the sprite known bits) */`. nd_snap.c:59 after
   `r.fill = s->w.fill; r.primz = s->w.prim_z;` add `r.kh = s->w.om_kh; r.kl = s->w.om_kl; /* NC0A */`. The new words sit in the compared region [plod, end).
2. nd_apw.h:5 append `, NDAPW_KH, NDAPW_KL` to the enum. nd_snap.c:129 nd_apw: add `case NDAPW_KH: return a->kh; case NDAPW_KL: return a->kl;` before `default:`.
3. pm_gpu.c:4582 `c.known_h = nd_c0.om_kh; c.known_l = nd_c0.om_kl;` becomes `c.known_h = nd_apw(NDAPW_KH); c.known_l = nd_apw(NDAPW_KL); /* NC0A */`.
4. Delete the fields from NdC0 (nd.h:165-190 only, NdWs keeps them): :167 `unsigned omode_h, omode_l;` becomes `unsigned omode_h;`; delete :170 (`fill`) and :172
   (`om_kh, om_kl`); :180 `int cur_tile, use_t1;` becomes `int cur_tile;`. nd_state.c:11: delete `.omode_l = 0x00000030u, ` and `.fill = 0xffffffffu, `.
5. Delete the writers:
   - nd_direct.c:57 `nd_c0.omode_h = mh; nd_c0.om_kh |= p->omh_m;` becomes `nd_c0.omode_h = mh;`. Delete :58.
   - nd_recs.c:148 keep only `nd_c0.omode_h = (nd_c0.omode_h & ~d->omh_m) | d->omh_v;`. :149 keep only `nd_c0.geom = (nd_c0.geom & d->g_and) | d->g_or;`.
   - nd_rect.c:323 delete the line (`if (nd_snap_live) nd_c0.fill = nd_snap_fill();`).
   - nd_set.c:99 `if (same) nd_c0.om_kh |= m;` goes (keep `const int same = ...;`). :103 delete `nd_c0.om_kh |= m;`. :105 nd_sv_oml body becomes `(void)r;`.
     :107 nd_sv_fill body becomes `(void)r;`.
   - nd_snap.c:213 delete `nd_c0.om_kl = c[7]; `.
   - nd_walk.c:260 nd_oml_run body becomes `(void)r; ep_mark = 0;`. :264 cp_vside keeps only its PRIM arm:
     `static void cp_vside(const uint32_t* u) { const unsigned id = u[0] & 0xffu; const uint32_t a = u[2], b = u[3]; if (id == ND_FS_PRIM && nd_ap->in.prim == b && (uint8_t)nd_ap->plod == (uint8_t)a) { pm_gpu_set_prim(nd_ap->in.prim, a); nd_c0.prim_lod = a; } } /* NC0A */`
   - nd_core.c:18 `nd_c0.om_kh, nd_c0.om_kl` becomes `nd_ap->kh, nd_ap->kl`.
6. Host test 3ds/tools/nd_direct_test.c: add `static unsigned t_oml; /* NC0A: the stub snap othermode L */` above ap_sync. :26 `ap_stub.oml = nd_c0.omode_l;` becomes
   `ap_stub.oml = t_oml;`. :27 delete `ap_stub.in.use_t1 = nd_c0.use_t1;`. :147 `nd_c0.omode_l = 0x5u;` becomes `t_oml = 0x5u;`. Delete :158 (core 0 keeps no othermode L).
7. Never-run fallback: nd_walk.c:209 delete nd_stat_calls and stat_call; :234 delete `else stat_call(next); /* NAT6E3 */`.
   FACT: every executed CALL unit carries bit 2 and its block (nd_rec.c:465-467 nd_alt_call, nd_ds.c:425); ent_dl_walk passes its own ab (nd_walk.c:291).
Checklist (hw_guidelines.md): core 0 does less per copy unit (cp_vside: 1 compare instead of 3) and per OMH, OML record. NdSnapChk grows 8 bytes (one more line of the 760, GUESS no extra line).
## Tests (output to build3ds/out/, read the last 20 lines; 1 build, at most 2 Azahar starts)
1. `grep -rn "nd_c0\.\(om_kh\|om_kl\|omode_l\|fill\|use_t1\)\|stat_call" 3ds/src > build3ds/out/nc0a_dead.txt`: empty.
2. `sh 3ds/tools/nd_host_test.sh > build3ds/out/nc0a_host.txt` (timeout 600000): PASS. If a host test defines NdC0 fields or nd_c0 initializers, edit it the same way.
   Commit `NC0A 1: sprite known bits from core 1, 5 nd_c0 fields go`.
3. `PM_LANE=NC0A sh 3ds/build.sh`; `python 3ds/tools/c0gate.py build3ds/pm_3ds.elf`: 0 / 0 / 1. Report the nd_c0 size of tip and base ELF (proxy).
4. Base, run and cmp (`bench.py base <bench> --commit $(git merge-base HEAD main)`, `bench.py run hudfb_drec,spot_mac01_drec --row NC0A`, `bench.py cmp`): every shot byte-equal.
Fix rights: the edited lines, compile errors, host test copies of NdC0. A DIFF (the core 0 bits were fewer than the core 1 bits): stop, send the cmp path to oracle NATIVE15.
Stop: last commit `NC0A 2: gate` deletes this page. One SubagentHandback: commit hash, host, c0gate, nd_c0 size base and tip, cmp.
