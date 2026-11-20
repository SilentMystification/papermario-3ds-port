# ws_NC0C (Sonnet): every tile bind but the glyph bind reads a recorded block (snap, entry or model snap), not the walker tiles of nd_c0
Designer: oracle NATIVE16 (plan_perf.md section 7). Start after NC0B is merged (entry record, NdGs). Parallel with NC0R (other lines). Branch from main.
Worktree build3ds/wt/NC0C, branch NC0C. PM_ROW=NC0C PM_LANE=NC0C. Lines: main 01e0780881; NC0B adds lines in nd_rec.c, nd_walk.c, nd_snap.h: grep the quoted text.
Why Sonnet: the entry record gains a bind block with a core 1 memo, 6 bind sites take 4 sources, and a slice inside a static call must void the entry block.
Objective: nd_tile_bind and dual_bind run with nd_bsrc set (ND_BTD and friends, nd_snap.h:33-36) at every site except the direct glyph (nd_direct.c:173, row NC0E).
Counter: the drec end line `c0src ... wbind <n>` (pm_gpu.c:628): tip = glyph binds plus the counted static fallbacks. c0gate 0 / 0 / 1; nd_c0 size unchanged.
Proxy (Azahar): drec byte-equal, 0 `bindsrc` lines. Console number: none; this row makes MCOPY1 exact (the miss draw binds from its model snap).
## Design (FACT code read)
- Today a bind reads the snap only when its triangle armed one (nd_geom.c:210 `nd_snap_rpend ? nd_snap_eb() : NULL`) or a rectangle has a live snap
  (nd_rect.c:248). Else ND_BTD falls back to nd_c0.td and tst1, and nd_tex_set.c:402, :406 set nd_c0w bit 4 (counted as wbind).
- Sources: armed snap (as now); else in a static call the entry block `nd_gs->b`; else `nd_ap->b` (no command since the last snap, so no tile change).
  A copy texture unit inside a static call voids the entry block (`nd_gs->b = NULL`): that bind keeps the walker tiles and is counted.
## Edits
1. nd_snap.h NdGs (NC0B): add `const NdSnapBind* b;`. NC0B's initializers: nd_ew gets `NULL`, nd_geo_units `loc` gets `nd_ap->b`.
2. nd_rec.c rec_ew (NC0B): 3 units; words 4 and 5 = the block pointer and 0. The block: `static NdSnapBind ew_lv[2]; static const NdSnapBind* ew_lb;` fill a local
   pair with `nd_snap_bind(&s->w.td[n], &s->w.tst1, n, &t[n])`; when `ew_lb && !memcmp(t, ew_lv, sizeof t)` reuse ew_lb, else copy t into
   `nd_rec_data_st(sizeof t)` and into ew_lv. `ew_lb = NULL;` next to nd_snap_ctx() (nd_rec.c:178). nd_ew_run (nd_walk.c) copies r[4] into nd_ew.b.
3. nd_snap.c: `const NdSnapBind* nd_snap_eb_of(const NdSnapBind* b)` copies b into sc_eb and returns sc_eb (NULL gives NULL). nd_snap_hold (:190) also copies
   nd_ew.b into a static pair and points nd_ew.b at it when nd_ew.b is not NULL (the chain can be rebuilt next).
4. nd_geom.c:210 before `nd_ep_bs = nd_snap_rpend ? nd_snap_eb() : NULL; /* NDR0G2 */`
   after `nd_ep_bs = nd_snap_rpend ? nd_snap_eb() : nd_snap_eb_of(nd_gs ? nd_gs->b : nd_ap->b); /* NC0C */`.
5. nd_rect.c:248 `nd_bsrc = rc_sn ? nd_snap_cur_b() : nd_ap->b;`. pm_gpu.c:4523-4524 (spr_build_rare): `nd_bsrc = nd_ap->b;` before, `nd_bsrc = NULL;` after.
6. nd_tex_set.c:27-32 pm_gbi_bind_tile1: `nd_bsrc = nd_msnap_b();` first; :31 reads `ND_BTD(1)->valid`; `nd_bsrc = NULL;` before return. nd_msnap.c:
   `const NdSnapBind* nd_msnap_b(void) { return ms_cur->b; }` (prototype in nd_msnapw.h with `struct NdSnapBind;`).
7. Void rule: first line of nd_tx_tszr (nd_tex_set.c:181) and of nd_sv_timg, nd_sv_tile, nd_sv_tsz, nd_sv_tlut, nd_sv_tlutn (:595-599): `if (nd_gs) nd_gs->b = NULL;`.
8. Census (drec frames only): nd_snap.c `void nd_bsrc_chk(const NdSnapBind* b, unsigned site)`: when b is not NULL and `nd_snap_bdiff(b, sc_wb(NULL, w))` is not 0,
   a cold log `bindsrc f<n> s<site>`, one line per site and frame (site 1 stream, 2 static, 3 rect, 4 sprite, 5 model tile 1). Call it at items 4 to 6. It stays.
## Checklist (hw_guidelines.md 2, 3, 7): the block is 2 x NdSnapBind in stream data, written only when it changes (memo); a hit costs one memcmp on core 1.
   PICA: the same nd_tile_bind calls in the same order; a block equal to the walker tiles gives the same loads, flushes and wraps.
## Tests (build3ds/out/, last 20 lines). Gate: 1 build, 2 Azahar runs (`bench.py base <b> --commit $(git merge-base HEAD main)`, `bench.py run`, `bench.py cmp`)
1. Host `sh 3ds/tools/nd_host_test.sh` (timeout 600000) PASS. Build; c0gate 0 / 0 / 1.
2. spot_mac01_drec (map shapes: entry blocks) and entcape_kmr03_drec (entity slices) byte-equal; `grep -c bindsrc` 0; note `c0src` wbind of base and tip logs.
Fix rights: compile errors; host test expectations that count the entry record units. A DIFF, a bindsrc line, a crash: stop, send the path to oracle NATIVE16.
Stop: last commit `NC0C 2: gate` deletes this page. Hand-back: one SubagentHandback: commit hash, host, c0gate, the 2 cmp lines, wbind base and tip, bindsrc count.
