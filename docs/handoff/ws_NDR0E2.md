# ws_NDR0E2 (Haiku): core 0 draws a mesh triangle with the snap combine decision; snapchk also compares at texture rectangles (stage S4; counter: snapchk bad)
Designer: oracle FIN. WAIT: branch from main after the NDR0E1 merge with snapchk bad 0 and after the NST2I merge (NST2H and NST2I edit nd_rec.c). Worktree build3ds/wt/NDR0E2,
branch NDR0E2. PM_ROW=NDR0E2 PM_LANE=NDR0E2. Edit only: 3ds/src/nd_rec.c (3 places), 3ds/include/nd_api.h (1 line), 3ds/src/nd_snap.c, 3ds/include/nd_snap.h, 3ds/src/nd_rect.c (2 lines).
## Facts (code read, main 7c8aeb0f21; NDR0E1 line numbers move)
- nd_snap_mesh (nd_snap.c:36) records only under PMDL("snapchk") with nd_rec_napi. A plain record runs rec_take without st_take: shadow reset (sh_all), the dschk
  record, ds_sclost = 1 (nd_rec.c rec_take). nd_rec_xf (:327-331) records with st_take = 1 (NDR5): the state run form, 1 unit only.
- stream_run disarms nd_snap_arm at every command but the state runs (nd_rec.c:655), so a TEXRECT command disarms before nd_texrect runs (nd_tex_set.c:234).
- NDR0E1 gives nd_comb_apply (nd_comb.c) and the coloured decision sc_cur.o. With snapchk bad 0 the GPU and core 0 already hold the snap values: the apply is a no-op.
## Edits
1. nd_rec.c after nd_rec_xf: `void nd_rec_st(NdNapiRun run, const void* rec, unsigned units) { st_take = 1; nd_rec_napi(run, rec, units); st_take = 0; }
   /* NDR0E2: a snap record taken as a state op (shadow, dschk and sc_lost stay) */`. nd_api.h after :12: declare it.
2. nd_rec.c :655: `op != NDC_JUMP &&` -> `op != NDC_JUMP && op != NDC_TEXRECT &&` (a texture rectangle keeps the snap armed for nd_texrect).
3. nd_rec.c nd_rec_texrect (:301) first line: `if (PMDL("snapchk")) nd_snap_mesh(); /* NDR0E2: the snap of a texture rectangle (compare only) */`. Next to the weak
   nd_snapchk_tick (:638): `__attribute__((weak)) void nd_snap_mesh(void) {}` (host units link nd_rec.c without nd_snap.c).
4. nd_snap.c nd_snap_mesh: delete `if (!PMDL("snapchk")) return;`. Record with `nd_rec_st(nd_snapchk_run, &r, sizeof r / 8u);` in place of nd_rec_napi.
5. nd_snap.c core 0: rename the body of nd_snapchk_tri after `nd_snap_arm = 0;` to `static void sc_compare(void)`. Then:
   ```
   void nd_snapchk_tri(void) {   /* NDR0E2: a mesh triangle: the compare (switch snapchk), then the combine of the snap */
       nd_snap_arm = 0;
       if (PMDL("snapchk")) sc_compare();
       nd_comb_apply(&sc_cur.o, sc_cur.in.w0, sc_cur.in.w1, sc_cur.in.use_t1, sc_cur.in.env);
   }
   void nd_snapchk_rect(void) { nd_snap_arm = 0; if (PMDL("snapchk")) sc_compare(); }   /* NDR0E2: a texture rectangle: compare only */
   ```
   nd_snap.h: declare nd_snapchk_rect. Update the file comments (the snap is recorded always; the compare stays behind the switch).
6. nd_rect.c: `#include "nd_snap.h"`; first line of nd_texrect (:219): `if (nd_snap_arm) nd_snapchk_rect(); /* NDR0E2 */`.
## Steps and tests (output to build3ds/out/, read the last 20 lines; 1 build, 4 Azahar starts)
1. Edits 1-6. `sh 3ds/tools/nd_host_test.sh` PASS. Commit `NDR0E2 1: mesh triangles take the snap combine, snapchk at texture rectangles`.
2. bench.py base nimgm_strip_drec,wscmb_pause_drec (merge-base, not charged). rowloop --row NDR0E2 --bench the same 2: LOOP PASS, byte-equal.
3. bench.py run nimgm_strip_snapchk and wscmb_pause_snapchk; grep -a the snapchk lines > build3ds/out/ndr0e2_snapchk.txt. Pass: recs above 0 in both. Record bad and
   the names (texture rectangles are new compares; GUESS: names from the box fold, which nd_texrect does not run). bad above 0 is data, not a fail: send the path.
Fix rights: the listed files; compile errors. A DIFF, a crash: stop, send the output path to the oracle. Wait.
Stop: last commit `NDR0E2 2: gate. 2 benches byte-equal; snapchk bad <2> fbad <2> miss <2>; names <list>` deletes this page. One SubagentHandback.
