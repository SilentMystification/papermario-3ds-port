# Row PBATCH2: a plan colour routed through the vertex slot goes into the ring vertices, so a per-particle prim change keeps the batch

(Model: Haiku. Base: main. Edits 3ds/src/nd_geom.c gv_tri (the params block :219-226) and one global; 3ds/src/pm_gpu.c
pm_gpu_set_plan (:3782) and two new functions after pm_gpu_gvtx_load (:3087). Disjoint from FILLA0 (nd_tmpl.c). Reads this file and CLAUDE.md.)

## User fact: pink_sparkles +4.7 ms, 44 draws per frame after PBATCH1 (the ring path).
## Cause (oracle code read)
- pink_sparkles sets gDPSetPrimColor per part, then the matrix and one of 8 lists (pink_sparkles.c:283-286). Its combine (sparkles gfx
  D_09000F20) is colour (1 - PRIM) * TEXEL0 + PRIM, alpha PRIM * TEXEL0: SHADE is not read, so the plan routes PRIM through the vertex
  colour slot (pm_comb.c:485 p->vtx = name_colour(vtxname), nd_geom.c:69-73 plan_vtx_ok) as a per-draw constant: nd_gv_params sets
  cm = 0, ca = vc (nd_model.c:51). A new prim makes a new plan, pm_gpu_set_plan flushes on any difference (pm_gpu.c:3783-3796; only the
  counters tell a constant-only change), and ca changes: one batch per part.
- The ring vertices are a raw copy of the N64 Vtx (pm_gpu_gvtx_load :3087-3094: 16 bytes, colour bytes 12-15). With the routed colour
  written into those bytes per triangle and cm = 1, ca = 0, the shader gives the same colour (vertex colour x 1 + 0 = vc, the same 8-bit
  values), and a plan that differs only in vtx needs no flush: the batch's parameters stay equal.

## Start
`sh 3ds/tools/wt_new.sh PBATCH2`. PM_ROW=PBATCH2 PM_LANE=PBATCH2 on every build and bench call.

## Exact edits
1. pm_gpu.c after pm_gpu_gvtx_load: `void pm_gpu_gvtx_colour(unsigned v, uint32_t rgba) { if (v >= GRING_VTX) return; u8* c = nb.gring + v * 16u + 12u;
   c[0] = (u8)(rgba >> 24); c[1] = (u8)(rgba >> 16); c[2] = (u8)(rgba >> 8); c[3] = (u8)rgba; } /* PBATCH2: the routed plan colour (0xRRGGBBAA) */`
   and `static int plan_vtx_only(const PmPlan* a, const PmPlan* b) { PmPlan x = *a, y = *b; x.vtx = y.vtx = 0; return memcmp(&x, &y, sizeof x) == 0; }`.
2. pm_gpu.c pm_gpu_set_plan, after the first line (the memcmp return): `extern int gv_vc_vtx; if (gs.combine == 8 && gv_vc_vtx && plan_vtx_only(&gs.plan, p)) { gs.plan.vtx = p->vtx; return; } /* PBATCH2: only the vertex-slot colour changed: it rides in the ring vertices */`.
3. nd_geom.c: global `int gv_vc_vtx; /* PBATCH2: the open ring batch takes the plan's routed colour from its vertices (cm 1, ca 0) */`.
   In gv_tri, replace the params block `gv_params_calc(tss, &ep_p); pm_gpu_set_rgb_scale(...); ep_pepoch = ...;` by: `gv_params_calc(tss, &ep_p);`
   `gv_vc_vtx = plan_active && cur_plan.vtxname != PM_NM_SHADE && cur_plan.vtxname != PM_NM_NONE && !ep_p.boost && !ep_p.fxon;`
   `if (gv_vc_vtx) for (int q = 0; q < 4; q++) { ep_p.cm[q] = 1.f; ep_p.ca[q] = 0.f; }` then the two old lines.
   After that block (before `if (ep_t1f) {`): `if (gv_vc_vtx) { extern void pm_gpu_gvtx_colour(unsigned v, uint32_t rgba); pm_gpu_gvtx_colour(vert_ring[a], cur_plan.vtx); pm_gpu_gvtx_colour(vert_ring[b], cur_plan.vtx); pm_gpu_gvtx_colour(vert_ring[c], cur_plan.vtx); }`.
   (cur_plan.vtx is 0xRRGGBBAA: unpack8, nd_tex_set.c:282-287, reads R from bits 24-31.)

## Gate (CLAUDE.md 2; Azahar budget 6 of 8)
- Build, memcheck OK; `bench.py run nd_host --row PBATCH2 --why "routed colour in ring vertices"`: PASS; hotset_size PASS.
- `bench.py base spot_mac01_det`, `bench.py base mdl5_iwa10_det`; one list `bench.py run spot_mac01_det,mdl5_iwa10_det,fxcen_c7 --row PBATCH2
  --why "same colours; pink_sparkles batches"`; `bench.py cmp` of both maps: 6/6 byte-equal each (pass gate).
- fxcen_report.py on the fxcen_c7 LOG: copy: pink_sparkles (124) ndraw per frame at most 1/4 of 49.8. Write the row in the commit.

## Step 2 (after 5dc61ce74: byte-equal, but pink_sparkles still 44 draws)
Cause: the prim op itself flushes: pm_gpu_set_prim (pm_gpu.c:3820-3825) flushes for combine 8 on any prim change (`reads && (rgba !=
prim ...) flush()`), before the plan compare runs. The 8 lists differ only in their vertex arrays (same texture and tile: the sparkle
frame is in the texcoords), the matrix rides in the palette: the prim flush is the remaining break.
Shared vertices: the colour is written per triangle into ring vertices; a vertex loaded before a colour change and used after it would be
rewritten while an earlier draw still reads it (the GPU reads the ring at draw time). pink_sparkles loads 4 new vertices per part, but
the rule must hold for every effect.
Edits (same worktree):
1. pm_gpu.c pm_gpu_set_prim: `if (reads && (rgba != prim || lod != prim_lod_g) && !(gs.combine == 8 && gv_vc_vtx)) flush(); /* PBATCH2: a
   routed prim rides in the vertices; the plan compare (pm_gpu_set_plan) flushes when a stage constant changes */` (extern gv_vc_vtx).
2. pm_gpu.c pm_gpu_set_plan, the PBATCH2 early return: before `return;` add `gv_vc_vbase = nb.gring_n;` and define
   `unsigned gv_vc_vbase; /* PBATCH2: ring vertices below this were loaded before the last routed colour change */` (export it).
3. nd_geom.c gv_tri, before the colour writes: `extern unsigned gv_vc_vbase; if (gv_vc_vtx && (vert_ring[a] < gv_vc_vbase || vert_ring[b] < gv_vc_vbase
   || vert_ring[c] < gv_vc_vbase)) { gv_vc_block = gv_epoch; pm_gpu_flush(); gv_params_calc(tss, &ep_p); pm_gpu_set_rgb_scale(ep_p.boost ? 2 : 1);
   ep_pepoch = gv_epoch; ep_ptss = tss; gv_vc_vtx = 0; } /* PBATCH2: an older vertex after a colour change: the uniform path */`
   with `static unsigned gv_vc_block = ~0u;`, and the gv_vc_vtx condition line gets `gv_vc_block != gv_epoch &&` first.
Gate as before (both maps 6/6 byte-equal, pink_sparkles ndraw at most 12.45).

## Stop and end
A failed gate, an edit that does not fit, or a tool fault: commit, hand back the exact output (CLAUDE.md 4-5). Delete this row file in
the completing commit. Hand back the hash, gates, memcheck line, the pink_sparkles row.
