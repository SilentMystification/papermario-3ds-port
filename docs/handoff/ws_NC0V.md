# ws_NC0V (Sonnet): the fog factors and tex_on come from the records, not from nd_c0
Designer: oracle NATIVE17 (plan_perf.md section 7). WAIT NC0S: branch from main after the NC0S merge (shared lines nd_set.c:108, nd.h NdC0).
Worktree build3ds/wt/NC0V, branch NC0V. PM_ROW=NC0V PM_LANE=NC0V. Lines: main d0483da1ba; NC0S moves nd.h lines: grep the quoted text.
Why Sonnet: 3 record formats change (snap, model snap, entry record), about 20 sites in 12 files, host tests that parse stream records, a census step.
Objective: every reader of nd_c0.fog_fm, fog_fo, tex_on reads a recorded value. Step 2 deletes the 3 fields. Counter: nd_c0 876 to 864 B (GUESS). c0gate 0 / 0 / 1.
## Sources (NC0B pattern, FACT code read)
- Packed word fogf = `(uint32_t)(uint16_t)fm << 16 | (uint16_t)fo`, read back as `(int16_t)(f >> 16)`, `(int16_t)f` (nd_set.c:109 decodes r[1] this way).
- Snap: nd_snap.h:24 `uint32_t pad3;` becomes `uint32_t fogf; /* NC0V */` (size stays). nd_snap.c:57 add `r.fogf = <pack of s->fog_fm, s->fog_fo>;`. tex_on is in.tex_on.
- NdGs (nd_snap.h:103): add `uint32_t fogf; int tex_on;` at the end. Initializers: nd_walk.c:246 nd_ew add `, 0u, 0`; nd_geom.c:443 loc add `, nd_ap->fogf, nd_ap->in.tex_on`.
- Macros after nd_snap.h:107: `#define ND_FOGF() (nd_gs ? nd_gs->fogf : nd_ap->fogf)` and `#define ND_TEXON() (nd_gs ? nd_gs->tex_on : (int)nd_ap->in.tex_on)`.
- Entry record nd_rec.c:462-464 rec_ew: 4 units. `rec_take(8)` to `rec_take(10)`, `NDC_HDR(NDC_NAPI, 3, 1)` to `NDC_HDR(NDC_NAPI, 4, 1)`, p[7] = fogf of s, p[8] = s->w.tex_on, p[9] = 0.
  nd_walk.c:247 nd_ew_run: add `nd_ew.fogf = r[5]; nd_ew.tex_on = (int)r[6];`.
- Model snap: nd_msnap.h:12 next line `uint32_t d_fogf, d_texon; /* NC0V: core 1 after the record */`. nd_msnap.c:23 add the 2 stores from nd_ds (fog pack, w.tex_on).
- NdGvC (nd_apw.h:7): add `uint32_t fogf; int tex_on;`. nd_snap.c:127 nd_apw_gvc: `c->fogf = ND_FOGF(); c->tex_on = ND_TEXON();`. nd_msnap.c:37: from ms_cur d_fogf, d_texon.
- Static setters: nd_set.c:108 nd_sv_tex `if (nd_gs) { nd_gs->tss = r[1]; nd_gs->tex_on = (int)r[0]; }`; nd_set.c:109 nd_sv_fogf adds `if (nd_gs) nd_gs->fogf = r[1];`.
## Step 1: readers switch, nd_c0 writes stay, census (commit `NC0V 1`)
1. nd_geom.c:115 `in.tex_on = gc->tex_on;`. nd_geom.c:121 `in.fog_fm = (int16_t)(gc->fogf >> 16); in.fog_fo = (int16_t)gc->fogf;`.
2. nd_comb.c:67 `c.tex_on = ND_TEXON();`. nd_comb.c:195 `(uint32_t)ND_TEXON()`. nd_comb.c:199 `(uint32_t)(int32_t)(int16_t)(ND_FOGF() >> 16)` and `(uint32_t)(int32_t)(int16_t)ND_FOGF()`.
3. nd_comb.c:325-329 t1_fold_tri: `const int ton = ND_TEXON();` before the loop; the 3 `nd_c0.tex_on` become `ton`.
4. Census (drec frames only) in nd_snap.c, cold noinline: `void nd_tf_chk(uint32_t fogf, int ton, unsigned site)`. When `pm_drec_now()` and fogf differs
   from the pack of nd_c0.fog_fm, fog_fo, or ton differs from nd_c0.tex_on: `pm_log_evt("texfog f%u s%u", frame, site)`, once per site and frame.
   Sites: 1 nd_apw_gvc, 2 nd_msnap_gvc, 3 pm_gbi_gvkey, 4 const_rgb_tex_alpha, 5 t1_fold_tri. Call it with the new values at each site.
## Step 2: delete (commit `NC0V 2: gate`)
- nd.h NdC0: `int tex_on;` and `int fog_fm, fog_fo;`. nd_set.c:93 the ND_FS_MW_FOG arm. nd_set.c:108, :109 the nd_c0 stores. nd_snap.c:189 `nd_c0.tex_on = i->tex_on;`.
- nd_recs.c:153, :154 (hd_rare HD_TEX, HD_FOGF lines). nd_recs.c:131, :133 (hitst, debug): compare with ND_TEXON() and the ND_FOGF() halves.
- nd_set.c:94-97 nd_tex_run (grep `void nd_tex_run`) before: `const int same = nd_c0.tex_on == (int)on; if (same && !mk) return; ...; nd_c0.tex_on = (int)on; ...`
  after: `{ const int mk = (int)(r[0] >> 31); pm_gbi_cur_op = ND_FS_TEXTURE; if (mk) gv_epoch++; }` (the epoch rule is the same: it bumps only on mk).
- nd_tf_chk and its 5 calls.
## Tests (build3ds/out/, last 20 lines). Each gate: 1 build, 2 Azahar runs. Base: `bench.py base <b> --commit $(git merge-base HEAD main)`.
1. Host `sh 3ds/tools/nd_host_test.sh` (timeout 600000) PASS. Build; c0gate 0 / 0 / 1.
2. Step 1: spot_mac01_drec, entcape_kmr03_drec byte-equal; `grep -c "texfog f"` 0 in both logs. Step 2: hudfb_drec, fxparts_drec byte-equal.
Fix rights: compile errors; host test expectations that count entry record units or the model snap size. A DIFF, a texfog line, a crash: stop, send the path to oracle NATIVE17.
Stop: `NC0V 2: gate` deletes this page. Hand-back: one SubagentHandback: commit hashes, host, c0gate, nd_c0 size base and tip, the 4 cmp lines, texfog count.
