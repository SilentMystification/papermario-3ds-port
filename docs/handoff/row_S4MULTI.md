# Row S4MULTI: nd_part_draw and parts_ok accept particle lists with several vertex loads (energy_shockwave's D_09000DC0)

(Model: Haiku: the exact code is below. Blocker: S4PDIR merged; it edits nd_rec.c nd_parts_end and nd_walk.c, while this row
edits nd_rec.c parts_ok and nd_geom.c nd_part_draw: different functions.)

## Why
D_09000DC0_3762D0 (energy_shockwave, assets/us/effects/gfx) is `VTX 32 at 0`, 8 TRI2, `VTX 28 at 0`, 7 TRI2, End. Two things
refuse it:
- parts_ok (3ds/src/nd_rec.c:283-299) returns 0 at the second NDO_VTX: its loop only allows NOP, TRI1, TRI2 and END;
- nd_part_draw (3ds/src/nd_geom.c:348) loads only the first load.

## Start
`sh 3ds/tools/wt_new.sh S4MULTI`; PM_ROW=S4MULTI PM_LANE=S4MULTI.

## Exact edits
1. nd_rec.c parts_ok. The first-load check stays: v0 must be 0 for the first load, because `((l[0] >> 1) & 0x7fu) != n` holds
   when the end slot equals n. Track `unsigned have = n;` (the slots loaded so far). In the loop, before `return 0;`:
   `if ((w0 >> 24) == NDO_VTX) { const unsigned n2 = (w0 >> 12) & 0xffu, e2 = (w0 >> 1) & 0x7fu; if (n2 < 1u || e2 > 64u || n2 > e2) return 0; if (e2 > have) have = e2; continue; }`
   and test the triangles with `tri_ok(..., have)` instead of `n`. Raise the loop bound from 40u to 64u: the list has 18 units.
2. nd_geom.c nd_part_draw:
   a. Before the room check (:353), sum the vertex counts of every NDO_VTX unit of the list into `tot`. Walk u, then `q = u + 2`
      until NDO_END, adding `(q[0] >> 12) & 0xffu` for NDO_VTX units. Use `tot + 64u` in both room calls instead of `n + 64u`,
      so no ring wrap happens between two loads of one list.
   b. In the triangle loop, before `if (q[0] == ND_NOP_A) continue;`, add:
      `if ((q[0] >> 24) == NDO_VTX) { const unsigned n2 = (q[0] >> 12) & 0xffu, v0 = ((q[0] >> 1) & 0x7fu) - n2; const Vtx* s2 = (const Vtx*)ptr_of(q[1]); if (!s2 || !is_host((u32)(uintptr_t)s2)) return 0; const int r2 = pm_gpu_gvtx_load(s2, n2); if (r2 < 0) return 0; PH_ADD(C_VTXLOAD, n2); PH_ADD(C_GVLOAD, n2); gv_load_n += n2; for (unsigned i = 0; i < n2; i++) rs[v0 + i] = (unsigned short)(r2 + (int)i); continue; }`
   c. Returning 0 after some triangles were emitted would draw them twice: the fallback runs the whole list. So everything that
      can refuse is checked before the first load:
      - check every later load's source pointer in step a (`ptr_of` plus is_host) and return 0 there;
      - after the room block (:353), add `if (!pm_gpu_gv_room(tot)) return 0;` (try 2, oracle 3). The room block alone does not
        guarantee room: ring_need returns 0 when no ring chunk is free (pm_gpu_rect.c:59-75).
      pm_gpu_gvtx_load (pm_gpu.c:3474-3475) refuses only on `!gpu_vtx_on || !gpu_ok || gring_n + n > GRING_VTX`, and
      pm_gpu_gv_room (:3473) tests the same three for tot. gring_n only grows by the loads of this list. The list's 30 triangles
      fit the 384 indices that ring_need reserved, so no ring wrap happens mid-list. So no later load can fail.
   d. Accepted from the try-1 code (54e17b37e): parts_ok requires each later load to start at v0 <= have (no slot gap); triangles
      are checked with tri_ok(..., have); the loop bound is 64. Keep them.
3. src/effects/energy_shockwave.c is not edited here: S4INST makes it a caller.

## Gate
- Build, memcheck OK; nd_host PASS.
- The merged S4 scenes must stay byte-equal (single-load lists take the same path): one list
  `bench.py run fxpic_63,fxpic_124 --row S4MULTI --why "multi-load particle lists"` (2 starts) after `bench.py base` of both;
  `bench.py cmp` both: every shot byte-equal.
- Budget 4. The first caller, and its byte gate, comes with S4INST (energy_shockwave).

## Stop
A byte difference or pm_gpu_gvtx_load able to fail after the room check: stop, commit, hand back. Delete this row file in the
completing commit.
