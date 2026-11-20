# ws_NC0B (Sonnet): geom and tss leave nd_c0. A vertex load reads its own VTX unit, a triangle reads the snap, a static call reads an entry record
Designer: oracle NATIVE16 (plan_perf.md section 7). Branch from main. Worktree build3ds/wt/NC0B, branch NC0B. PM_ROW=NC0B PM_LANE=NC0B. Lines: main 01e0780881.
Why Sonnet: a record format change (VTX unit 2 to 4 words) and a new record, 4 writers, about 35 readers in 12 files, a source pointer saved and restored around every static call. Haiku cannot hold these cross-file invariants.
Objective: NdC0 (nd.h:165-187) loses geom, tex_ss, tex_ts. nd_c0 size 912 to 900 (FACT 912: NC0A merge; -12 is arithmetic). c0gate stays 0 / 0 / 1. Proxy (Azahar): drec byte-equal, 0 `geodiff` lines. Console number: none by design; watch ndc kb (178, GUESS +2 to 6 KB) and lgms (44.4, GUESS +0.1 ms).
## Design (FACT code read)
- Core 0 sources: the VTX unit words (vertex load); `nd_ap` (stream, copy and list-table triangles, rects, direct draws, sprites, particles); `nd_gs`, set only
  inside a static call (nd_sg_try, ent_gdl, nd_ent_case, nd_geo_units). Core 1 writes an entry record just before each static call. Static list GEOM ops
  change `nd_gs->geom` in place (the static cache, class C). nd_snap.h after :98: `typedef struct NdGs { uint32_t geom, tss, oml, omh; } NdGs;`
  `extern NdGs* nd_gs; extern NdGs nd_ew; #define ND_GEOM() (nd_gs ? nd_gs->geom : nd_ap->geom)`, `#define ND_TSS() (nd_gs ? nd_gs->tss : nd_ap->tss)`. oml, omh: NC0R.
## Step 1 (writers and readers; nd_c0 is still written, for the census)
1. nd_rec.c cp_put (:404) gets the first parameter `const NdDs* s`: :411, :486, :487 pass `&nd_ds`, nd_alt_put :533 passes `alt_s` (FACT: alt_s is the applied
   state at every nd_alt_put: nd_rec.c:258, :276, :484, :510, :519, nd_gtab.c:143, :152). Before: `const NdNapiRun r = cp_run(w0, w1); if (!r) { left_unit(w0); return; }`
   After, first: `if ((w0 >> 24) == ND_FS_VTX) { xr_put(nd_vtx_run, w0, w1, 2u); xr_put(nd_vtx_run, s->w.geom, (s->w.tex_ss << 16) | (s->w.tex_ts & 0xffffu), 1u); return; }`
2. nd_walk.c:243 nd_vtx_run: a loop with stride 4 (as nd_cpy_run :270; keep r_pro, pm_ph_pop, r_end) calls `nd_geom_vtx(a, b, (const NdW2*)q, q[2], q[3])`.
3. nd_geom.c:453 `nd_geom_vtx(u32 w0, u32 w1, const NdW2* list, uint32_t geom, uint32_t tss)`: :480, :511 read geom, :508 reads tss. Callers: :450 (`g->geom, g->tss`, item 7), nd_walk.c:284 and nd_direct.c:337 (`nd_ap->geom, nd_ap->tss`).
4. nd_rec.c after rec_nat2 (:458): `static void rec_ew(const NdDs* s)` as rec_nat2 (NDC_NRUN, flag 1: not a command), 2 units, run `nd_ew_run`, words geom, tss,
   omode_l, omode_h of s; `void nd_rec_ew(const NdDs* s) { rec_open(); rec_ew(s); }`; a weak nd_ew_run as ND_WK (:418). Call it first in nd_alt_call (:538, alt_s),
   nd_rec_ent_draw (:261) and nd_rec_ent (:282) with &nd_ds, nd_gtab.c nd_rec_ntab before :140 and nd_rec_ndev before :150 with &nd_ds.
   nd_model.c:288 nd_r11_state: first in case NDC_NAPI `if (p[1] == (uint32_t)(uintptr_t)nd_ew_run) break;` (a skip, no i++).
5. nd_walk.c: `NdGs nd_ew = { G_SHADE | G_SHADING_SMOOTH, 0xffffffffu, 0x30u, 0u }; NdGs* nd_gs;` and nd_ew_run copies r[0..3] into nd_ew.
   `NdGs* const gs0 = nd_gs; nd_gs = &nd_ew;` before the static tries, `nd_gs = gs0;` on every exit of them: nr_call :219-228, gt_run0 :369-376, nd_walk_ent :186.
   The copy runs (nr_call `if (ab)` :229, gt_body) run with the restored pointer. A slice inside a static call keeps nd_gs: nd_set.c:105 nd_sv_geom adds
   `if (nd_gs) nd_gs->geom = r[1];` and :108 nd_sv_tex adds `if (nd_gs) nd_gs->tss = r[1];` (FACT: ent_cut_run nd_ent.c:417 runs copy slices in nd_ent_case).
6. To ND_GEOM() / ND_TSS(): nd_geom.c:97, :130-132, :195, :198, :199, :251, :252, :373, :377; nd_ent.c:312, :313, :315; nd_shape.c:423, :591; nd_comb.c:208.
   To nd_ap->geom: nd_direct.c:54, :156; nd_rect.c:280, :339; nd_core.c:17. To nd_gs->geom: nd_shape.c:454, :497-499, :627, :880.
   Example nd_geom.c:195 before `unsigned cg = nd_c0.geom & G_CULL_BOTH;` after `unsigned cg = ND_GEOM() & G_CULL_BOTH;`.
7. nd_geo_units (nd_geom.c:441): `NdGs loc, * const s0 = nd_gs, * g = s0; if (!g) { loc = (NdGs){ nd_ap->geom, nd_ap->tss, nd_ap->oml, nd_ap->in.omode_h }; g = nd_gs = &loc; }`;
   the GEOM arm (:446-448) reads and writes g->geom; `nd_gs = s0;` at the end.
8. nd_ds.c:529 copy GEOM values are absolute: `case ND_FS_GEOM: v[0] = 0u; v[1] = s->w.geom; return nd_sv_geom;`.
9. Census: a cold `geo_note(unsigned site, uint32_t nw, uint32_t old)` logs `geodiff f<n> s<site> new %08x old %08x`, 8 lines per site. Site 1: gv_epoch_enter (:206),
   ND_GEOM() != nd_c0.geom. Site 2: nd_geom_vtx, geom or tss != nd_c0. Site 3: nd_ew_run, r[0] or r[1] != nd_c0.
## Step 2 (delete): nd.h:166, :179; nd_state.c:11 inits; nd_walk.c:447; nd_set.c:85-88 (keep `if (mk) gv_epoch++` and set_mode when `!pm_gpu_mode_is`), :94-97 and :108
   (keep tex_on), :105 nd_sv_geom (set_mode from r[1] only); nd_recs.c:139 (keep tex_on), :149; :112, :117 to nd_ap->geom, nd_ap->tss; nd_snap.c:234 the nd_c0.geom term; item 9.
## Checklist (hw_guidelines.md 1, 3, 5, 6): the VTX words are 2 sequential stores; ND_GEOM() is one load and one branch, no call; no new per-vertex work. PICA: pm_gpu_set_mode runs at the same places with the same shadow test (pm_gpu_mode_is); no new GPU command.
## Tests (build3ds/out/, last 20 lines). Per step: 1 build, 2 Azahar runs (`bench.py base <b> --commit $(git merge-base HEAD main)`, `bench.py run`, `bench.py cmp`)
1. Step 1: `sh 3ds/tools/nd_host_test.sh` (timeout 600000) PASS; build; c0gate 0 / 0 / 1; spot_mac01_drec, entcape_kmr03_drec byte-equal; `grep -c geodiff` 0 in both logs. Commit `NC0B 1: VTX unit and entry record carry geom and tss; readers switched`.
2. Step 2: host PASS; c0gate 0 / 0 / 1; nd_c0 size (`arm-none-eabi-nm -S`, as NC0A); hudfb_drec, fxparts_drec byte-equal.
Fix rights: compile errors; host test expectations that count VTX record units, the records of a call (entry record) or GEOM slot values (item 8).
A DIFF, a geodiff line or a crash: stop, send the output path to oracle NATIVE16. Stop: last commit `NC0B 2: gate` deletes this page. Hand-back: one SubagentHandback: commit hashes, host, c0gate, nd_c0 size base and tip, the 4 cmp lines, the geodiff count.
