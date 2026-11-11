# Row HCCUT3: two rare arms of class F and B/G hot functions cold (byte-equal)

(Model: Haiku. The agent reads only this file, CLAUDE.md and what this file names.)
## Facts (oracle code read, main 910dc2dd9)
- pm_gpu_gtrip (3ds/src/pm_gpu.c, `int pm_gpu_gtrip(`, class F): in its second loop `for (int i = 0; i < 3; i++) { unsigned r = ri[i];
  ...` the block `if (mvm & (1u << i)) { ... continue; }` (TRI-2 wide edge copy: wide_copy_vtx, ring word, rgen, rk, ro) runs only
  with widescreen on and a vertex on the game edge: rare, inline in the per-triangle loop.
- sync_render (3ds/src/nd_set.c:199-245, called per draw state change): the block `if (m == 0x00504B50u && PMDL("orbdbg")) { ... }`
  (:234-243, FX1-2b debug print) is inline; pm_gpu_dbg_mode(m) (:233) stays.
- Other rows: HC6B (pm_gpu.c model key/hit/submit, nd_emit.c, nd_recs.c, nd_model.c, nd_rec.c), HUDGM (pm_gpu.c:148 pcomp,
  nd_mtx_set.c, pm_gpu_int.h, nd_int.h, nd_cen.c, pm_gpu.h), HUDRING (nd_geom.c), HCCUT2 (nd_tmpl.c, nd_rect.c, hot_order.ld,
  hotset.json). This row edits pm_gpu_gtrip (pm_gpu.c, not a function of HC6B or HUDGM) and nd_set.c sync_render only.
## Start
`sh 3ds/tools/wt_new.sh HCCUT3`; PM_ROW=HCCUT3 PM_LANE=HCCUT3. `git merge main` before the completing commit.
## Exact edits
1. pm_gpu.c, directly above `int pm_gpu_gtrip(` add:
   `static __attribute__((cold, noinline)) int gtrip_edge_copy(const float (*tab)[4][4], unsigned gi, unsigned r, unsigned k, unsigned* ro) {`
   `    const int n = wide_copy_vtx(tab[gi], r);`
   `    if (n < 0) return 0;`
   `    *(unsigned short*)(nb.gring + 16u * (unsigned)n + 6) = (unsigned short)k;`
   `    rgen[n] = nb.pal_gen; rk[n] = (unsigned char)k;`
   `    *ro = (unsigned)n;`
   `    return 1;`
   `} /* HCCUT3: the TRI-2 wide edge copy of pm_gpu_gtrip, out of the per-triangle loop */`
   and in pm_gpu_gtrip replace the whole `if (mvm & (1u << i)) { ... continue; }` block with
   `if (mvm & (1u << i)) { if (!gtrip_edge_copy(tab, g[i], r, k3[i], &ro[i])) return 0; continue; } /* TRI-2 (HCCUT3: cold) */`
   (same statements in the same order; wide_copy_vtx is defined above pm_gpu_gtrip, check it; if not, move the new function below it).
2. nd_set.c: move the body of `if (m == 0x00504B50u && PMDL("orbdbg")) { ... }` (:234-243) into
   `static __attribute__((cold, noinline)) void sr_orb_log(unsigned m, const NdRender* rs)` above sync_render (rs.x -> rs->x), and call
   it as `if (m == 0x00504B50u && PMDL("orbdbg")) sr_orb_log(m, &rs); /* HCCUT3: cold */`.
## Gate (CLAUDE.md 2)
- build: memcheck OK. `bench.py run hotset_size --row HCCUT3 --lane HCCUT3 --why "HCCUT3 sizes"`: write M1hot F and G before (main
  line) and after; rc 1 only for the load_impl stale name while HCCUT2 is unmerged (write it; any other stale name: stop).
- `bench.py run nd_host --row HCCUT3 --why "HCCUT3"`: PASS.
- `bench.py base` mdl5_iwa10_det and spot_mac01_det; one `run mdl5_iwa10_det,spot_mac01_det` (the second warm); `cmp` each: 6/6.
## Stop and end
A failed gate, an edit that does not fit the code, or a test-tool fault: commit and hand back the exact output (CLAUDE.md 4-5).
At the end delete this row file in the completing commit. Hand back: the ready hash, the gate results, the memcheck line.
