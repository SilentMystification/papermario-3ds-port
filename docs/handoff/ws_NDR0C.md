# ws_NDR0C (Haiku): snapchk adds the target role and the render inputs (oml, geom, fog and blend colours) made on core 1 (stage S4; counter: snapchk names)
Designer: oracle FIN. Branch from main (NDR0A merged). Worktree build3ds/wt/NDR0C, branch NDR0C. PM_ROW=NDR0C PM_LANE=NDR0C.
Edit only: 3ds/include/nd_snap.h, 3ds/src/nd_snap.c, 3ds/src/nd_snap_in.c, 3ds/tools/nd_snap_test.c, 3ds/src/nd_geo.c (3 lines).
## Facts (code read, main 2548d439f4)
- The target role is nd_tgt_role(kind, siz, for_fill, oml, omh, zcol) (nd_tgt.h, pure). set_cimg (nd_target.c:87-99) makes the kind from address tests on
  static buffers. nd_target_beginw (nd_geo.c:16-19, core 1) records w0 = siz << 12 | width - 1, w1 = a host address (comment nd_geo.c:15).
- sync_render reads omode_l, omode_h, geom and the role (nd_set.c:211, :239), the mix reads fogc (:230). NdDs holds omode_l, geom, fogc, blendc.
- zcol is set by a texture rectangle (nd_rect.c:234). Core 1 passes 0: a mismatch shows as `role`.
## Edits
1. nd_snap.h: in NdSnapChk after plan_on: `int32_t role; uint32_t oml, geom, fogc, blendc, pad;` (keeps size % 8 == 0). Declare
   `void nd_snap_rnd(const NdDs* s, NdSnapChk* r);` (nd_snap_in.c) and `void nd_snap_tgt(uint32_t w0, uint32_t w1);` (nd_snap.c, core 1).
2. nd_snap_in.c: nd_snap_rnd sets r->oml = s->w.omode_l, r->geom = s->w.geom, r->fogc = s->fogc, r->blendc = s->blendc.
3. nd_snap.c core 1 (include nd_tgt.h, nd_int.h for DepthCopyBuffer :17, pm_is_cfb :201, pm_fb_native :418, nuGfxZBuffer :18):
   ```
   static NdTgt sn_tgt;
   void nd_snap_tgt(uint32_t w0, uint32_t w1) {   /* the target record of set_cimg (nd_target.c:87-99) without the GPU calls */
       void* nc = (void*)(uintptr_t)w1;
       sn_tgt.img = nc; sn_tgt.siz = (uint8_t)((w0 >> 12) & 3u); sn_tgt.stride = (uint16_t)((w0 & 0xfffu) + 1u);
       sn_tgt.kind = (uint8_t)((nc == NULL ? NDK_NULL : 0u) | (w1 == 0u ? NDK_ZERO : 0u) | (nc == (void*)DepthCopyBuffer ? NDK_DEPTHQ : 0u) |
           (pm_is_cfb(nc) ? NDK_CFB : 0u) |
           ((nuGfxZBuffer && (const unsigned short*)nc >= nuGfxZBuffer && (const unsigned short*)nc < nuGfxZBuffer + 320 * 240) ? NDK_INZ : 0u) |
           (pm_fb_native(nc) ? NDK_NATIVE : 0u));
   }
   ```
   nd_snap_mesh after nd_snap_in: `nd_snap_rnd(&nd_ds, &r); r.role = nd_tgt_role(sn_tgt.kind, sn_tgt.siz, 0, nd_ds.w.omode_l, nd_ds.w.omode_h, 0);`.
   The skip test also needs the same 5 fields: add `&& !memcmp(&r.role, &sn_last.role, 5u * 4u)` to it.
4. nd_snap.c core 0, nd_snapchk_tri after the tst line: `if (sc_cur.role != nd_img_role(0)) sc_add("role");` and the same for oml (nd_ws.omode_l),
   geom (nd_ws.geom), fogc (fogc), blendc (blendc). Declare `extern unsigned fogc, blendc;` (nd_set.c:41).
5. nd_geo.c: next to the weak nd_snap_mesh (:33) add `__attribute__((weak)) void nd_snap_tgt(uint32_t w0, uint32_t w1) { (void)w0; (void)w1; }`.
   In nd_target_beginw after the rec line: `nd_snap_tgt(rec[0], rec[1]);` (always, no switch: it writes only core 1 data).
6. nd_snap_test.c: an NdDs with omode_l, geom, fogc, blendc set; nd_snap_rnd; check the 4 fields. Print `ndr0c rnd bad <b>`. `--fault` expects blendc = fogc.
## Steps and tests (output to build3ds/out/, read the last 20 lines; 1 build, 4 Azahar starts)
1. Edits 1-6. `sh 3ds/tools/nd_host_test.sh` PASS with `ndr0c rnd bad 0` and the fault seen. Commit `NDR0C 1: snapchk target role and render inputs`.
2. bench.py base spot_mac01_drec,nimgm_anim_drec (merge-base, not charged). rowloop --row NDR0C --bench the same 2: LOOP PASS, byte-equal.
3. bench.py run nimgm_strip_snapchk and wscmb_pause_snapchk; grep the snapchk lines > build3ds/out/ndr0c_snapchk.txt. Pass: recs above 0 in both, and
   no `snapchk bad` line names role, oml, geom, fogc or blendc. Else: commit the grep, send the output path to the oracle, wait.
## Fix rights: the listed files; compile errors. A DIFF, a crash: stop, send the output path to the oracle. Wait.
Stop: last commit `NDR0C 2: gate. 2 benches byte-equal; snapchk bad <2> fbad <2> miss <2>; names <list>` deletes this page. One SubagentHandback.
