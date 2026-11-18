# ws_NST2E (Haiku): the units of the 6 direct-draw runners feed nd_ds (stage S3; counter: dschk fbad not higher)
Designer: oracle NSTO. WAIT: branch from main after the NST2D merge. Worktree build3ds/wt/NST2E, branch NST2E. PM_ROW=NST2E PM_LANE=NST2E.
Edit only: src/hud_element.c, src/msg_draw.c, src/screen_overlays.c, src/screen_render_util.c, src/battle/190B20.c, src/draw_img_util.c,
src/msg.c, src/state_title_screen.c (1 line each in the last 5), 3ds/src/nd_direct.c, 3ds/include/nd_api.h, 3ds/tools/fxrest.d/dschk_base.txt.
## Facts (code read, main 7ea18ce89e)
- 6 NAPI runners draw direct or, as fallback, write typed units into a local list and run them (nd_nr_exec). Core 1 never sees these units.
  | runner (core 0) | fallback units | record sites (core 1) |
  |---|---|---|
  | hud_rect_run hud_element.c:551 | hud_rect_emit(r, &nrp, 0) :555 | hud_element.c:850 |
  | cplx_quad_run hud_element.c:562 | :592-606 (FILT .. XF_POP) | hud_element.c:1891 |
  | pm_tsten_run screen_overlays.c:73 | :112-114 | screen_render_util.c:285 |
  | msg_glyph_run msg_draw.c:1711 | :1727-1739 | msg_draw.c:1846 |
  | nd_fill_run nd_direct.c:270 | nd_fill_walk :262-265 | screen_overlays.c:154, :163; battle/190B20.c:3017 |
  | nd_trect_run nd_direct.c:294 | :318-332 | draw_img_util.c:169, msg.c:2256, state_title_screen.c:522 |
- nd_ds_feed (nd_rec.c, NST2C) applies typed units to nd_ds. It skips ND_NOP_A. MTX, VTX, TRI and RECT units change no NdDs field.
## Edits (one shape for each runner; the units and their order do not change)
1. Move the fallback unit writes into `static void <x>_emit(const <Rec>* r, uint32_t** o)`: start `uint32_t* nrp = *o;`, end `*o = nrp;`.
   The runner calls `<x>_emit(r, &nrp);` where the writes were, then nd_nr_exec as before. hud_rect_emit exists: keep it.
2. Add `<x>_feed(const uint32_t* u)` next to the runner: a buffer of the runner's size (hud: `static uint32_t fb[1024]`, core 1 only; the
   others a local array), `<x>_emit`, then `nd_ds_feed(buf, (unsigned)(p - buf) / 2u);`. Static where the record site is in the same file.
   Exported, declared in nd_api.h next to nd_fill_run (:86): nd_fill_feed, nd_trect_feed. pm_tsten_feed: declare in screen_render_util.c:14.
3. At each record site add the feed right after nd_rec_napi, with the same record. Example (screen_render_util.c:285):
   Before `nd_rec_napi(pm_tsten_run, rec, 3u);` After `nd_rec_napi(pm_tsten_run, rec, 3u); pm_tsten_feed((const uint32_t*)rec);`
4. Example emit (screen_overlays.c:112-114, after): `static void tsten_emit(const NdTStenRec* r, uint32_t** o) { uint32_t* nrp = *o;
   const int full = (int)(r->full_and_id & 1u); if (full) NDST_CC(...); NDVAL_PRIM(...); NDTX_RECT_W(...); *o = nrp; }` (the same 3 lines).
5. nd_api.h: declare `void nd_ds_feed(const uint32_t* u, unsigned n); /* nd_rec.c, core 1 */`. Remove the local copy in src/sprite.c only
   when it conflicts (else leave it).
## Steps and tests (output to build3ds/out/, read the last 20 lines; 1 build, 4 Azahar starts)
1. Edits 1-5. `sh 3ds/tools/nd_host_test.sh` PASS (no new case: the edit moves unit writes). Commit `NST2E 1: the 6 direct-draw runners feed nd_ds`.
2. bench.py base spot_mac01_drec,wscmb_pause_drec (merge-base, not charged). rowloop --row NST2E --bench the same 2: LOOP PASS, byte-equal.
3. bench.py run the 2 dschk scenes spot_mac01_dschk and fxchk_a_dschk; grep their dschk lines > build3ds/out/nst2e_dschk.txt. Pass: each scene's last fbad is not higher than
   dschk_base.txt (the direct path still differs; row NST2F adopts it). Write the grep into dschk_base.txt.
## Fix rights: the listed files; compile errors. A DIFF (an emit changed a unit), a higher fbad, a crash: stop, send the output path to oracle
  NSTO. Wait. Stop: last commit `NST2E 2: gate. 2 benches byte-equal; dschk fbad <2> (base <2>)` deletes this page. One SubagentHandback.
