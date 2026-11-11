# Row S4P: native particle draws (NATIVEAPI S4 part 1: record builder and core 0 executor; caller in row S4SNOW)

(Model: Haiku. One coder does S4P, then S4SNOW, in one worktree; the orchestrator merges after the S4SNOW gates.)

## Design (oracle code read, main 6e171b39a)
XL xl4a: snowfall costs 18.1 ms of core 0 walk per frame for 117 parts (~155 us each). Per part today: gSPMatrix x2, gSPDisplayList,
gSPPopMatrix. The DL goes walk_call (nd_walk.c:221: ent_gdl lookup, PBATCH1 ring flag) -> nd_ops_run (:395: gv_epoch++, so every part
re-runs gv_epoch_enter and gv_params_calc) -> nd_geom_vtx (nd_geom.c:296: bbox class, per-vertex slot fields, clip transform) ->
draw_tri/gv_tri per triangle (:266, :198). API: `nd_parts_*` records one NDC_NAPI per effect draw: per instance its typed units
(state and matrix ops before the list, the NDO_CALL of the list, ops after). Core 0 (nd_parts_run) runs the pre units with
nd_walk_state (exact: same handlers), draws the list with nd_part_draw (ring copy, gtrip/gappend as gv_tri), runs the post units.
gv_epoch goes up once per record: only state ops change what gv_epoch_enter and the params read, and they bump gv_epoch themselves.
nd_part_draw has no CPU clip test: gv_tri only skipped triangles wholly outside one view plane, which draw no pixel on the GPU either.
Later (S4 colour and uv steps): prim as vertex colour, tile offset as uv offset; this row keeps them as ops (exact).

## Start
`sh 3ds/tools/wt_new.sh S4P`. PM_ROW=S4P PM_LANE=S4P on every build and bench call.

## Touch only
3ds/include/nd_api.h (end), 3ds/src/nd_rec.c (new functions after nd_rec_napi), 3ds/src/nd_walk.c (new nd_parts_run after
nd_ops_run), 3ds/src/nd_geom.c (new nd_part_draw after draw_tri).

## Exact edits
1. nd_api.h: `typedef struct NdParts { unsigned n, pre, m, ok; const void* list; uint32_t u[2u + 2u * 252u]; } NdParts;` and
   `void nd_parts_begin(NdParts* b, unsigned pre, unsigned post); void nd_parts_add(NdParts* b, const uint32_t* units);
   void nd_parts_end(NdParts* b); void nd_parts_run(const uint32_t* rec); int nd_part_draw(const uint32_t* list);`
   (units of one instance: pre ops, the NDO_CALL unit of its list, post ops; m = pre + 1 + post.)
2. nd_rec.c: `static int parts_ok(const uint32_t* l)`: 1 when l[0] is NDO_VTX with v0 0 (((l[0] >> 1) & 0x7f) == n, n = (l[0] >> 12)
   & 0xff, 1 <= n <= 64), then up to 40 units each NDO_TRI1, NDO_TRI2 or ND_NOP_A up to NDO_END, every index (byte / 2 of the a field,
   and of b for NDO_TRI2) < n. Memo the last 8 ok list addresses (array, round robin). nd_parts_begin: n 0, pre, m = pre + 1 + post.
   nd_parts_add (core 1: no ptr_of, it reads walker segments): a = units[2 * pre + 1]; if !is_host(a) or !parts_ok((const uint32_t*)a): nd_parts_end(b), then nd_rec_state(units[2k], units[2k+1])
   for k = 0 .. m-1 (the old stream form), return. Else copy the m units to b->u + 2 + 2 * m * n, n++; if 1 + (n + 1) * m > 255:
   nd_parts_end(b). nd_parts_end: if n: u[0] = n | pre << 8 | m << 16, u[1] = 0, nd_rec_napi(nd_parts_run, b->u, 1 + n * m), n = 0.
3. nd_walk.c nd_parts_run(u): `PM_WALK_OWN();` n, pre, m from u[0]; `pm_walk_side++; gv_epoch++; dlc_on = 1; gp_prev_op = -1;` then per instance p:
   `nd_walk_state(p, pre);` l = (const uint32_t*)ptr_of(p[2 * pre + 1]); `if (!nd_part_draw(l)) nd_ops_run(l, ND_LIST_MAX, 1);`
   `nd_walk_state(p + 2 * (pre + 1), m - pre - 1);`. After the loop: `pm_walk_side--; gv_streak_flush(); tex_streak_flush();`
4. nd_geom.c nd_part_draw(u): n and src from the VTX unit (ptr_of(u[1])); `memset(vert_live, 0, sizeof vert_live);`
   `if (!pm_gpu_gv_room(n + 64u)) ring_need(n + 64u, 384u);` gmi = gv_matrix_id(0); return 0 if (nd_ws.geom & G_LIGHTING), gmi ==
   GV_NONE, or r0 = pm_gpu_gvtx_load(src, n) < 0. `PH_ADD(C_VTXLOAD, n); PH_ADD(C_GVLOAD, n); gv_load_n += n;` rs[i] = r0 + i
   (unsigned short[64]); tss = nd_ws.tex_ss << 16 | (nd_ws.tex_ts & 0xffff). Per triangle (a, then b for NDO_TRI2; skip ND_NOP_A):
   indices i0..i2 as draw_tri (:267-269); `if (ep_osc != gv_epoch) { off_screen_check(); box_fold_sync(); ep_osc = gv_epoch; }`;
   `if (ep_epoch != gv_epoch) gv_epoch_enter();` `if (ep_why >= 0) { gv_fb[ep_why]++; tri_drop[1]++; n_drop++; continue; }`
   `if (ep_cullboth) { n_out++; continue; }` then gv_tri lines 222-246 verbatim with tss as above, vert_ring[a/b/c] -> rs[i0/i1/i2],
   vert_gmi[] -> gmi, and the ep_t1f block (:227-231) as `if (ep_t1f) { unsigned rt[3] = { rs[i0], rs[i1], rs[i2] }; t1_fold_tri(rt,
   tss); }` (no lighting: no texgen vertex); `why = GVR_REFUSE` becomes `gv_fb[GVR_REFUSE]++; tri_drop[1]++; n_drop++;`. At the end `tri_since_vtx = 1; return 1;`

## Gate
Build (`PM_ROW=S4P PM_LANE=S4P sh 3ds/build.sh > build3ds/out/build.txt 2>&1`, memcheck OK) and nd_host PASS. No Azahar run (no
caller yet). Commit; continue with docs/handoff/row_S4SNOW.md in the same worktree.

## Stop / At the end
A failed gate or an edit that does not fit: commit, hand back the output. Delete this row file in the S4P commit.
